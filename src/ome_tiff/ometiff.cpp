#include "..\micro_tiff\micro_tiff.h"
//#include "jpeg_handler.h"
#include "ometiff.h"
#include "ometiff_info.h"
#include <filesystem>
#include <sstream>

using namespace std;
using namespace ome;
namespace fs = filesystem;

#define CHECK_OPENMODE(mode) \
if (mode==OpenMode::READ_ONLY_MODE) return ErrorCode::ERR_MODIFY_NOT_ALLOWED

OmeTiff::OmeTiff(void )
{
	_open_mode = OpenMode::READ_ONLY_MODE;
	_compression_mode = CompressionMode::COMPRESSIONMODE_NONE;
	_images.clear();
	_plates.clear();
	_raw_file_containers.clear();

	_tiff_file_full_name = L"";
	_tiff_file_dir = L"";
	_tiff_file_name = L"";
	_tiff_header_ext = "";

	_is_in_parsing = false;
}

OmeTiff::~OmeTiff(void)
{
	if (_open_mode != OpenMode::READ_ONLY_MODE) {
		RewriteOMEHeader();
	}

	for (auto it = _plates.begin(); it != _plates.end(); it++)
	{
		if (it->second)
		{
			delete it->second;
			it->second = nullptr;
		}
	}

	for (auto it = _images.begin(); it != _images.end(); it++)
	{
		if (it->second)
		{
			delete it->second;
			it->second = nullptr;
		}
	}

	for (auto it = _raw_file_containers.begin(); it != _raw_file_containers.end(); it++)
	{
		if (it->second)
		{
			delete it->second;
			it->second = nullptr;
		}
	}
}

int32_t OmeTiff::Init(const wchar_t* file_name, OpenMode mode, CompressionMode cm)
{
	_open_mode = mode;
	_compression_mode = cm;

	wchar_t full_file_name[_MAX_PATH] = L"";
	wchar_t* __ = _wfullpath(full_file_name, file_name, _MAX_PATH);

	fs::path p{ full_file_name };
	string extension = p.extension().u8string();
	_tiff_header_ext = extension;

	wstring name = p.filename().replace_extension().wstring();
	if (name.size() == 0 || extension.size() == 0)
		return ErrorCode::ERR_FILE_PATH_ERROR;

	_tiff_file_name = name;
	wstring dir = p.parent_path().wstring();
	_tiff_file_dir = dir;

	if (0 != _waccess(_tiff_file_dir.c_str(), 0))
	{
		if (_open_mode == OpenMode::CREATE_MODE) {
			int ret = _wmkdir(_tiff_file_dir.c_str());
			if (ret != 0)
				return ErrorCode::ERR_FILE_PATH_ERROR;
		}
		else {
			return ErrorCode::ERR_FILE_PATH_ERROR;
		}
	}

	_tiff_file_full_name = p.wstring();

	if (_open_mode != OpenMode::CREATE_MODE)
	{
		int32_t hdl = micro_tiff_Open(_tiff_file_full_name.c_str(), ((_open_mode != OpenMode::READ_ONLY_MODE) * OPENFLAG_WRITE));
		if (hdl < 0) return hdl;
		uint16_t type = 0;
		uint32_t size = 0;
		int32_t status = micro_tiff_GetTagInfo(hdl, 0, TIFFTAG_IMAGEDESCRIPTION, type, size);
		if (status == ErrorCode::TIFF_ERR_TAG_NOT_FOUND && strcmp(extension.c_str(), "tid") == 0)
		{
			wchar_t new_file_name[_MAX_PATH] = { 0 };
			wcscpy_s(new_file_name, full_file_name);
			bool num_flag = false;
			for (size_t i = wcslen(new_file_name); i > 0; --i)
			{
				if (num_flag && new_file_name[i] == L'_')
				{
					wchar_t ex[] = L".btf";
					wmemcpy(&new_file_name[i], ex, wcslen(ex));
					return Init(new_file_name, mode, cm);
				}
				if (new_file_name[i] >= '0' && new_file_name[i] <= '9')
					num_flag = true;
			}
			micro_tiff_Close(hdl);
			return ErrorCode::ERR_FILE_PATH_ERROR;
		}
		if (status == ErrorCode::STATUS_OK && size > 0)
		{
			unique_ptr<uint8_t[]> auto_xml = make_unique<uint8_t[]>(size);
			uint8_t* xml = auto_xml.get();
			if (xml == nullptr)
				return ErrorCode::ERR_BUFFER_IS_NULL;
			status = micro_tiff_GetTag(hdl, 0, TIFFTAG_IMAGEDESCRIPTION, xml);
			if (status != ErrorCode::STATUS_OK)
			{
				micro_tiff_Close(hdl);
				return ErrorCode::ERR_GET_OMEXML_FAILED;
			}
			_is_in_parsing = true;
			status = parse_ome_xml((char*)xml, size, this);
			_is_in_parsing = false;			
		}
		micro_tiff_Close(hdl);
		return status;
	}
	return ErrorCode::STATUS_OK;
}

int32_t OmeTiff::SaveTileData(void* image_data, uint32_t stride, FrameInfo frame, uint32_t row, uint32_t column)
{
	CHECK_OPENMODE(_open_mode);
	TiffContainer* container = nullptr;
	uint32_t ifd_no;
	int32_t status = GetRawContainer(frame, &container, ifd_no, false);
	if (status != ErrorCode::STATUS_OK)
		return status;

	return container->SaveTileData(image_data, stride, ifd_no, row, column);
}

int32_t OmeTiff::PurgeFrame(FrameInfo frame)
{
	CHECK_OPENMODE(_open_mode);
	TiffContainer* container = nullptr;
	uint32_t ifd_no;
	int32_t status = GetRawContainer(frame, &container, ifd_no, false);
	if (status != ErrorCode::STATUS_OK)
		return status;

	return container->CloseIFD(ifd_no);
}

int32_t OmeTiff::LoadRawData(FrameInfo frame, OmeSize dst_size, OmeRect src_rect, void* buffer, uint32_t stride)
{
	TiffContainer* container = nullptr;
	uint32_t ifd_no;
	int32_t status = GetRawContainer(frame, &container, ifd_no, true);
	if (status != ErrorCode::STATUS_OK)
		return status;

	return container->LoadRectData(ifd_no, dst_size, src_rect, buffer, stride);
}

int32_t OmeTiff::AddPlate(PlateInfo& plate_info)
{
	if (!_is_in_parsing)
		CHECK_OPENMODE(_open_mode);

	uint32_t count = (uint32_t)_plates.size();
	if (count > 0)
		return ErrorCode::ERR_ONLY_SUPPORT_ONE_PLATE;
	auto it = _plates.find(plate_info.id);
	if (it != _plates.end())
		return ErrorCode::ERR_PLATE_ID_EXIST;
	Plate* plate = new Plate();
	plate->_info = plate_info;
	_plates[plate_info.id] = plate;
	return ErrorCode::STATUS_OK;
}

int32_t OmeTiff::GetPlates(PlateInfo* plates_info)
{
	int32_t index = 0;
	for (auto it = _plates.begin(); it != _plates.end(); it++, index++)
	{
		plates_info[index] = it->second->_info;
	}
	return ErrorCode::STATUS_OK;
}

int32_t OmeTiff::GetPlatesSize()
{
	return (int32_t)_plates.size();
}

int32_t OmeTiff::AddWell(uint32_t plate_id, WellInfo& well_info)
{
	if (!_is_in_parsing)
		CHECK_OPENMODE(_open_mode);

	auto it = _plates.find(plate_id);
	if (it == _plates.end())
		return ErrorCode::ERR_PLATE_NOT_EXIST;

	if (well_info.column_index >= it->second->_info.column_size)
		return ErrorCode::ERR_COLUMN_OUT_OF_RANGE;
	if (well_info.row_index >= it->second->_info.row_size)
		return ErrorCode::ERR_ROW_OUT_OF_RANGE;

	return it->second->add_well(well_info);
}

int32_t OmeTiff::GetWells(uint32_t plate_id, WellInfo* wells_info)
{
	auto it = _plates.find(plate_id);
	if (it == _plates.end())
		return ErrorCode::ERR_PLATE_NOT_EXIST;

	return it->second->get_wells(wells_info);
}

int32_t OmeTiff::GetWellsSize(uint32_t plate_id)
{
	auto it = _plates.find(plate_id);
	if (it == _plates.end())
		return ErrorCode::ERR_PLATE_NOT_EXIST;

	return (int32_t)it->second->_wells_array.size();
}

int32_t OmeTiff::AddScan(uint32_t plate_id, ScanInfo& scan_info)
{
	if (!_is_in_parsing)
		CHECK_OPENMODE(_open_mode);

	auto it = _plates.find(plate_id);
	if (it == _plates.end())
		return ErrorCode::ERR_PLATE_NOT_EXIST;

	memcpy_s(scan_info.dimension_order, NAME_LEN, "XYZTC", 5);

	int32_t status = it->second->add_scan(scan_info);
	if (status != ErrorCode::STATUS_OK)
		return status;

	PlateAcquisition plate_acquisition;
	plate_acquisition._id = "PlateAcquisition:" + to_string(plate_id) + "." + to_string(scan_info.id);

	auto it_plate_acquisition = it->second->_plate_acquisition_array.find(scan_info.id);
	if (it_plate_acquisition != it->second->_plate_acquisition_array.end())
		return ErrorCode::ERR_PLATE_ACQUISITION_ID_EXIST;

	it->second->_plate_acquisition_array[scan_info.id] = plate_acquisition;
	return status;
}


int32_t OmeTiff::GetScans(uint32_t plate_id, ScanInfo* scans_info)
{
	auto it = _plates.find(plate_id);
	if (it == _plates.end())
		return ErrorCode::ERR_PLATE_NOT_EXIST;

	return it->second->get_scans(scans_info);
}


int32_t OmeTiff::GetScansSize(uint32_t plate_id)
{
	auto it = _plates.find(plate_id);
	if (it == _plates.end())
		return ErrorCode::ERR_PLATE_NOT_EXIST;

	return (int32_t)it->second->_scans_array.size();
}

int32_t OmeTiff::AddChannel(uint32_t plate_id, uint32_t scan_id, ChannelInfo& channel_info)
{
	if (!_is_in_parsing)
		CHECK_OPENMODE(_open_mode);

	auto it_plate = _plates.find(plate_id);
	if (it_plate == _plates.end())
		return ErrorCode::ERR_PLATE_NOT_EXIST;

	auto it_scan = it_plate->second->_scans_array.find(scan_id);
	if (it_scan == it_plate->second->_scans_array.end())
		return ErrorCode::ERR_SCAN_NOT_EXIST;

	auto it_plate_acquisition = it_plate->second->_plate_acquisition_array.find(scan_id);
	if (it_plate_acquisition == it_plate->second->_plate_acquisition_array.end())
		return ErrorCode::ERR_PLATE_ACQUISITION_NOT_EXIST;

	PlateAcquisition plate_acquisition = it_plate_acquisition->second;

	for (auto it_ref = plate_acquisition._well_sample_ref.begin(); it_ref != plate_acquisition._well_sample_ref.end(); it_ref++)
	{
		uint32_t well_id = 0, well_sample_id = 0;
		int result_sscanf = sscanf_s(it_ref->second.c_str(), "WellSample:%*u.%u.%u", &well_id, &well_sample_id);
		if (result_sscanf <= 0)
			return ErrorCode::ERR_SCANF_ASSIGNED_ERROR;

		auto it_well = it_plate->second->_wells_array.find(well_id);
		if (it_well == it_plate->second->_wells_array.end())
			return ErrorCode::ERR_WELL_NOT_EXIST;

	 	auto it_well_sample = it_well->second._well_sample_array.find(well_sample_id);
		if (it_well_sample == it_well->second._well_sample_array.end())
			return ErrorCode::ERR_WELL_SAMPLE_NOT_EXIST;

		uint32_t image_id = 0;
		result_sscanf = sscanf_s(it_well_sample->second.image_ref_id.c_str(), "Image:%u", &image_id);
		if (result_sscanf <= 0)
			return ErrorCode::ERR_SCANF_ASSIGNED_ERROR;

		auto it_image = _images.find(image_id);
		if (it_image != _images.end())
		{
			int32_t status = it_image->second->_pixels.add_channel(channel_info);
			if (ErrorCode::STATUS_OK != status)
				return status;
		}
	}

	return it_scan->second.add_channel(channel_info);
}

int32_t OmeTiff::GetChannels(uint32_t plate_id, uint32_t scan_id, ChannelInfo* channel_info)
{
	auto it_plate = _plates.find(plate_id);
	if (it_plate == _plates.end())
		return ErrorCode::ERR_PLATE_NOT_EXIST;

	auto it_scan = it_plate->second->_scans_array.find(scan_id);
	if (it_scan == it_plate->second->_scans_array.end())
		return ErrorCode::ERR_SCAN_NOT_EXIST;

	return it_scan->second.get_channels(channel_info);
}

int32_t OmeTiff::GetChannelsSize(uint32_t plate_id, uint32_t scan_id)
{
	auto it_plate = _plates.find(plate_id);
	if (it_plate == _plates.end())
		return ErrorCode::ERR_PLATE_NOT_EXIST;

	auto it_scan = it_plate->second->_scans_array.find(scan_id);
	if (it_scan == it_plate->second->_scans_array.end())
		return ErrorCode::ERR_SCAN_NOT_EXIST;

	return (int32_t)it_scan->second._channel_array.size();
}

int32_t OmeTiff::RemoveChannel(uint32_t plate_id, uint32_t scan_id, uint32_t channel_id)
{
	if (!_is_in_parsing)
		CHECK_OPENMODE(_open_mode);

	auto it_plate = _plates.find(plate_id);
	if (it_plate == _plates.end())
		return ErrorCode::ERR_PLATE_NOT_EXIST;

	auto it_scan = it_plate->second->_scans_array.find(scan_id);
	if (it_scan == it_plate->second->_scans_array.end())
		return ErrorCode::ERR_SCAN_NOT_EXIST;

	auto it_plate_acquisition = it_plate->second->_plate_acquisition_array.find(scan_id);
	if (it_plate_acquisition == it_plate->second->_plate_acquisition_array.end())
		return ErrorCode::ERR_PLATE_ACQUISITION_NOT_EXIST;

	PlateAcquisition plate_acquisition = it_plate_acquisition->second;

	for (auto it_ref = plate_acquisition._well_sample_ref.begin(); it_ref != plate_acquisition._well_sample_ref.end(); it_ref++)
	{
		uint32_t well_id = 0, well_sample_id = 0;
		int result_sscanf = sscanf_s(it_ref->second.c_str(), "WellSample:%*u.%u.%u", &well_id, &well_sample_id);
		if (result_sscanf <= 0)
			return ErrorCode::ERR_SCANF_ASSIGNED_ERROR;

		auto it_well = it_plate->second->_wells_array.find(well_id);
		if (it_well == it_plate->second->_wells_array.end())
			return ErrorCode::ERR_WELL_NOT_EXIST;

		Well well = it_well->second;

		auto it_well_sample = well._well_sample_array.find(well_sample_id);
		if (it_well_sample == well._well_sample_array.end())
			return ErrorCode::ERR_WELL_SAMPLE_NOT_EXIST;

		WellSample well_sample = it_well_sample->second;

		uint32_t image_id;
		result_sscanf = sscanf_s(well_sample.image_ref_id.c_str(), "Image:%u", &image_id);
		if (result_sscanf <= 0)
			return ErrorCode::ERR_SCANF_ASSIGNED_ERROR;

		auto it_image = _images.find(image_id);
		if (it_image != _images.end())
		{
			int32_t status = it_image->second->_pixels.remove_channel(channel_id);
			if (ErrorCode::STATUS_OK != status)
				return status;
		}
	}

	return it_scan->second.remove_channel(channel_id);
}

int32_t OmeTiff::AddScanRegion(uint32_t plate_id, uint32_t scan_id, uint32_t well_id, ScanRegionInfo& scan_region_info, string well_sample_id, string image_ref_id)
{
	if (!_is_in_parsing)
		CHECK_OPENMODE(_open_mode);

	auto it_plate = _plates.find(plate_id);
	if (it_plate == _plates.end())
		return ErrorCode::ERR_PLATE_NOT_EXIST;

	auto it_scan = it_plate->second->_scans_array.find(scan_id);
	if (it_scan == it_plate->second->_scans_array.end())
		return ErrorCode::ERR_SCAN_NOT_EXIST;

	auto it_plate_acquisition = it_plate->second->_plate_acquisition_array.find(scan_id);
	if (it_plate_acquisition == it_plate->second->_plate_acquisition_array.end())
		return ErrorCode::ERR_PLATE_ACQUISITION_NOT_EXIST;

	auto it_well = it_plate->second->_wells_array.find(well_id);
	if (it_well == it_plate->second->_wells_array.end())
		return ErrorCode::ERR_WELL_NOT_EXIST;

	int32_t result = it_scan->second.add_region(scan_region_info);
	if (result != ErrorCode::STATUS_OK)
		return result;

	if (_is_in_parsing)
	{
		if (well_sample_id.empty() || image_ref_id.empty())
			ErrorCode::ERR_PARAMETER_NOT_VALIE;

		result = it_well->second.add_well_sample(scan_region_info, well_sample_id, image_ref_id);
		if (result != ErrorCode::STATUS_OK)
			return result;

		return it_plate_acquisition->second.add_well_sample_ref(scan_region_info.id, well_sample_id);
	}

	//Image:0 is the boot file
	uint32_t index = (uint32_t)_images.size() + 1;
	while(true)
	{
		auto it_image = _images.find(index);
		if (it_image == _images.end())
			break;
		index++;
	}
	string image_id = "Image:" + to_string(index);

	string well_sample_ref_id = "";
	result = it_well->second.add_well_sample(plate_id, scan_region_info, well_sample_ref_id, image_id);
	if (result != ErrorCode::STATUS_OK)
		return result;

	result = it_plate_acquisition->second.add_well_sample_ref(scan_region_info.id, well_sample_ref_id);
	if (result != ErrorCode::STATUS_OK)
		return result;

	Image* image = new Image();
	image->_id = image_id;
	image->_name = to_string(index);

	image->_pixels._info.id = string("Pixels:0");
	image->_pixels._info.dimension_order = string(it_scan->second._info.dimension_order);
	image->_pixels._info.physical_size_per_pixel_x = it_scan->second._info.pixel_physical_size_x;
	image->_pixels._info.physical_size_per_pixel_y = it_scan->second._info.pixel_physical_size_y;
	image->_pixels._info.physical_size_per_pixel_z = it_scan->second._info.pixel_physical_size_z;
	image->_pixels._info.time_increment = it_scan->second._info.time_increment;
	image->_pixels._info.physical_x_unit = it_scan->second._info.pixel_physical_uint_x;
	image->_pixels._info.physical_y_unit = it_scan->second._info.pixel_physical_uint_y;
	image->_pixels._info.physical_z_unit = it_scan->second._info.pixel_physical_uint_z;
	image->_pixels._info.time_increment_unit = it_scan->second._info.time_increment_unit;
	image->_pixels._info.tile_pixel_width = it_scan->second._info.tile_pixel_size_width;
	image->_pixels._info.tile_pixel_height = it_scan->second._info.tile_pixel_size_height;
	image->_pixels._info.significant_bits = it_scan->second._info.significant_bits;
	image->_pixels._info.pixel_type = it_scan->second._info.pixel_type;
	image->_pixels._info.size_x = scan_region_info.pixel_size_x;
	image->_pixels._info.size_y = scan_region_info.pixel_size_y;
	image->_pixels._info.size_z = scan_region_info.pixel_size_z;
	image->_pixels._info.size_t = 1;

	for (auto it_channel = it_scan->second._channel_array.begin(); it_channel != it_scan->second._channel_array.end(); it_channel++)
	{
		image->_pixels.add_channel(it_channel->second._info);
	}

	_images[index] = image;

	return ErrorCode::STATUS_OK;
}

int32_t OmeTiff::GetScanRegions(uint32_t plate_id, uint32_t scan_id, uint32_t well_id, ScanRegionInfo* scan_regions_info)
{
	auto it_plate = _plates.find(plate_id);
	if (it_plate == _plates.end())
		return ErrorCode::ERR_PLATE_NOT_EXIST;

	auto it_scan = it_plate->second->_scans_array.find(scan_id);
	if (it_scan == it_plate->second->_scans_array.end())
		return ErrorCode::ERR_SCAN_NOT_EXIST;

	auto it_plate_acquisition = it_plate->second->_plate_acquisition_array.find(scan_id);
	if (it_plate_acquisition == it_plate->second->_plate_acquisition_array.end())
		return ErrorCode::ERR_PLATE_ACQUISITION_NOT_EXIST;

	PlateAcquisition plate_acquisition = it_plate_acquisition->second;

	uint32_t index = 0;
	for (auto it_well_sample_ref = plate_acquisition._well_sample_ref.begin(); it_well_sample_ref != plate_acquisition._well_sample_ref.end(); it_well_sample_ref++)
	{
		uint32_t parse_well_id = 0;
		int result_sscanf = sscanf_s(it_well_sample_ref->second.c_str(), "WellSample:%*u.%u.%*u", &parse_well_id);
		if (result_sscanf <= 0)
			return ErrorCode::ERR_SCANF_ASSIGNED_ERROR;

		if (parse_well_id == well_id)
		{
			int32_t result = it_scan->second.get_region(it_well_sample_ref->first, scan_regions_info[index]);
			if (result != ErrorCode::STATUS_OK)
				return result;

			index++;
		}
	}

	return ErrorCode::STATUS_OK;
}

int32_t OmeTiff::GetScanRegionsSize(uint32_t plate_id, uint32_t scan_id, uint32_t well_id)
{
	auto it_plate = _plates.find(plate_id);
	if (it_plate == _plates.end())
		return ErrorCode::ERR_PLATE_NOT_EXIST;

	auto it_plate_acquisition = it_plate->second->_plate_acquisition_array.find(scan_id);
	if (it_plate_acquisition == it_plate->second->_plate_acquisition_array.end())
		return ErrorCode::ERR_PLATE_ACQUISITION_NOT_EXIST;

	PlateAcquisition plate_acquisition = it_plate_acquisition->second;

	int32_t count = 0;
	for (auto it_well_sample_ref = plate_acquisition._well_sample_ref.begin(); it_well_sample_ref != plate_acquisition._well_sample_ref.end(); it_well_sample_ref++)
	{
		uint32_t parse_well_id;
		int result_sscanf = sscanf_s(it_well_sample_ref->second.c_str(), "WellSample:%*u.%u.%*u", &parse_well_id);
		if (result_sscanf <= 0)
			return ErrorCode::ERR_SCANF_ASSIGNED_ERROR;

		if (parse_well_id == well_id)
			count++;
	}

	return count;
}

string OmeTiff::GetUTF8FileName()
{
	fs::path p = _tiff_file_name;
	string str_utf8 = p.make_preferred().u8string();
	string utf8_file_name = str_utf8 + _tiff_header_ext;
	return utf8_file_name;
}

string OmeTiff::GetFullPathWithFileName(string utf8_file_name)
{
	fs::path p = _tiff_file_dir;
	p /= utf8_file_name;
	return p.make_preferred().u8string();
}

int32_t OmeTiff::RewriteOMEHeader()
{
	int32_t status = ErrorCode::STATUS_OK;
	int32_t hdl = micro_tiff_Open(_tiff_file_full_name.c_str(), OPENFLAG_WRITE | OPENFLAG_CREATE | OPENFLAG_BIGTIFF);
	if (hdl < 0)
		return hdl;

	ImageInfo info = { 0 };
	info.bits_per_sample = 8;
	info.compression = COMPRESSION_NONE;
	info.planarconfig = PLANARCONFIG_CONTIG;
	info.photometric = PHOTOMETRIC_MINISBLACK;
	info.image_byte_count = 1;
	info.samples_per_pixel = 1;
	info.block_width = 256;
	info.block_height = 256;
	info.image_width = 256;
	info.image_height = 256;

	uint32_t buf_size = info.image_width * info.image_height;

	unique_ptr<uint8_t[]> auto_buf = make_unique<uint8_t[]>(buf_size);

	void* buf = auto_buf.get();
	if (buf == nullptr) {
		return ErrorCode::TIFF_ERR_ALLOC_MEMORY_FAILED;
	}

	char* xml_data = nullptr;
	uint32_t xml_size = 0;

	do {
		status = generate_ome_xml(&xml_data, this, info.image_width, info.image_height);
		if (status < 0) {
			if (xml_data != nullptr)
				free(xml_data);
			break;
		}
		xml_size = (uint32_t)strlen(xml_data) + 1;
	} while (0);

	if (status != ErrorCode::STATUS_OK)
		return status;

	do {
		int32_t ifd_no = micro_tiff_CreateIFD(hdl, info);
		if (ifd_no < 0) {
			status = ifd_no;
			break;
		}
		status = micro_tiff_SetTag(hdl, ifd_no, TIFFTAG_IMAGEDESCRIPTION, TagDataType::TIFF_ASCII, xml_size, xml_data);
		if (status != ErrorCode::STATUS_OK) {
			break;
		}
		status = micro_tiff_SaveBlock(hdl, ifd_no, 0, buf_size, buf);
		if (status != ErrorCode::STATUS_OK) {
			break;
		}
		micro_tiff_CloseIFD(hdl, (int32_t)ifd_no);
		status = micro_tiff_Close(hdl);
		if (status != ErrorCode::STATUS_OK) {
			break;
		}
	} while (0);
	free(xml_data);
	return status;
}

int32_t OmeTiff::SetCustomTag(FrameInfo frame, uint16_t tag_id, uint16_t tag_type, uint32_t tag_count, void* tag_value)
{
	TiffContainer* container = nullptr;
	uint32_t ifd_no;
	int32_t status = GetRawContainer(frame, &container, ifd_no, true);
	if (status != ErrorCode::STATUS_OK)
		return status;

	return container->SetCustomTag(ifd_no, tag_id, tag_type, tag_count, tag_value);
}

int32_t OmeTiff::GetCustomTag(FrameInfo frame, uint16_t tag_id, uint32_t tag_size, void* tag_value)
{
	TiffContainer* container = nullptr;
	uint32_t ifd_no;
	int32_t status = GetRawContainer(frame, &container, ifd_no, true);
	if (status != ErrorCode::STATUS_OK)
		return status;

	return container->GetCustomTag(ifd_no, tag_id, tag_size, tag_value);
}

int32_t OmeTiff::GetRawContainer(FrameInfo frame, TiffContainer** container, uint32_t& ifd_no, bool is_read)
{
	auto it_plate = _plates.find(frame.plate_id);
	if (it_plate == _plates.end())
		return ErrorCode::ERR_PLATE_NOT_EXIST;

	auto it_scan = it_plate->second->_scans_array.find(frame.scan_id);
	if (it_scan == it_plate->second->_scans_array.end())
		return ErrorCode::ERR_SCAN_NOT_EXIST;

	Scan scan = it_scan->second;

	auto it_channel = scan._channel_array.find(frame.c_id);
	if (it_channel == scan._channel_array.end())
		return ErrorCode::ERR_CHANNEL_NOT_EXIST;

	auto it_plate_acquisition = it_plate->second->_plate_acquisition_array.find(frame.scan_id);
	if (it_plate_acquisition == it_plate->second->_plate_acquisition_array.end())
		return ErrorCode::ERR_PLATE_ACQUISITION_NOT_EXIST;

	PlateAcquisition plate_acquisition = it_plate_acquisition->second;

	auto it_well_sample_ref = plate_acquisition._well_sample_ref.find(frame.region_id);
	if (it_well_sample_ref == plate_acquisition._well_sample_ref.end())
		return ErrorCode::ERR_NO_WELL_SAMPLE_MATCHS_REGION_ID;

	string well_sample_ref_id = it_well_sample_ref->second;

	uint32_t well_id = 0, well_sample_id = 0;
	int result_sscanf = sscanf_s(well_sample_ref_id.c_str(), "WellSample:%*u.%u.%u", &well_id, &well_sample_id);
	if (result_sscanf <= 0)
		return ErrorCode::ERR_SCANF_ASSIGNED_ERROR;

	auto it_well = it_plate->second->_wells_array.find(well_id);
	if (it_well == it_plate->second->_wells_array.end())
		return ErrorCode::ERR_WELL_NOT_EXIST;

	Well well = it_well->second;

	auto it_well_sample = well._well_sample_array.find(well_sample_id);
	if (it_well_sample == well._well_sample_array.end())
		return ErrorCode::ERR_WELL_SAMPLE_NOT_EXIST;

	WellSample well_sample = it_well_sample->second;

	ContainerInfo info = { 0 };
	info.tile_pixel_width = scan._info.tile_pixel_size_width;
	info.tile_pixel_height = scan._info.tile_pixel_size_height;
	info.samples_per_pixel = it_channel->second._info.sample_per_pixel;
	info.pixel_type = scan._info.pixel_type;
	info.open_mode = _open_mode;
	info.compress_mode = _compression_mode;

	uint32_t image_id;
	result_sscanf = sscanf_s(well_sample.image_ref_id.c_str(), "Image:%u", &image_id);
	if (result_sscanf <= 0)
		return ErrorCode::ERR_SCANF_ASSIGNED_ERROR;
	auto it = _images.find(image_id);
	if (it == _images.end())
		return ErrorCode::ERR_CANNOT_FIND_IMAGE;

	Image* image = it->second;

	TiffData tiff_data;
	int32_t result = image->_pixels.get_tiff_data(frame.c_id, frame.z_id, frame.t_id, tiff_data);
	if (result != ErrorCode::STATUS_OK)
	{
		if (is_read)
			return result;

		wstringstream stream_file_name;
		stream_file_name << _tiff_file_name << "_s" << to_wstring(frame.scan_id) << "c" << to_wstring(frame.c_id) << ".tid";
		wstring w_file_name = stream_file_name.str();
		fs::path p_full{ _tiff_file_dir };
		p_full /= w_file_name;
		wstring container_full_path = p_full.wstring();

		fs::path p{ w_file_name };
		string utf8_container_file_name = p.u8string();

		{
			unique_lock<mutex> lock(_mutex_raw);
			auto it_find = _raw_file_containers.find(utf8_container_file_name);
			if (it_find == _raw_file_containers.end())
			{
				*container = new TiffContainer();
				result = (*container)->Init(info, container_full_path);
				if (result != ErrorCode::STATUS_OK)
					return result;
				_raw_file_containers.insert(make_pair(utf8_container_file_name, *container));
			}
			else
			{
				*container = it_find->second;
			}
		}

		ScanRegionInfo scan_region_info;
		result = scan.get_region(frame.region_id, scan_region_info);
		if (result != ErrorCode::STATUS_OK)
			return result;

		int32_t created_ifd_no = (*container)->CreateIFD(scan_region_info.pixel_size_x, scan_region_info.pixel_size_y);
		if (created_ifd_no < 0)
			return created_ifd_no;

		ifd_no = created_ifd_no;
		TiffData tiff_data;
		tiff_data.FirstC = frame.c_id;
		tiff_data.FirstT = frame.t_id;
		tiff_data.FirstZ = frame.z_id;
		tiff_data.IFD = created_ifd_no;
		tiff_data.FileName = utf8_container_file_name;

		result = image->_pixels.add_tiff_data(tiff_data);
		if (result != ErrorCode::STATUS_OK)
			return result;
	}
	else
	{
		fs::path p_full{ _tiff_file_dir };
		p_full /= tiff_data.FileName;
		wstring container_full_path = p_full.wstring();

		{
			unique_lock<mutex> lock(_mutex_raw);
			auto it_find = _raw_file_containers.find(tiff_data.FileName);
			if (it_find == _raw_file_containers.end())
			{
				*container = new TiffContainer();
				result = (*container)->Init(info, container_full_path);
				if (result != ErrorCode::STATUS_OK)
					return result;
				_raw_file_containers.insert(make_pair(tiff_data.FileName, *container));
			}
			else
			{
				*container = it_find->second;
			}
		}

		ifd_no = tiff_data.IFD;
	}

	return result;
}
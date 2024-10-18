#include "ome_tiff_library.h"
#include "ometiff.h"
#include <vector>

static std::vector<OmeTiff*> vecOmeTiff;
using namespace ome;

#ifndef CHECK_HANDLE
#define CHECK_HANDLE(handle) \
	if (handle < 0 || (size_t)handle >= vecOmeTiff.size() || vecOmeTiff[handle] == nullptr) \
		return ErrorCode::ERR_HANDLE_NOT_EXIST;
#endif // !CHECK_HANDLE
#ifndef CHECK_BUFFER
#define CHECK_BUFFER(buffer) if(buffer == nullptr) return ErrorCode::ERR_BUFFER_IS_NULL;
#endif // !CHECK_BUFFER

int32_t ome_open_file(const wchar_t* file_name, OpenMode mode, CompressionMode cm)
{
	wchar_t full_file_name[_MAX_PATH] = L"";
	wchar_t* __ = _wfullpath(full_file_name, file_name, _MAX_PATH);
	std::wstring full_path_string = std::wstring(full_file_name);

	size_t size = vecOmeTiff.size();
	OmeTiff* omeTiff = nullptr;
	for (size_t i = 0; i < size; i++) {
		omeTiff = vecOmeTiff.at(i);
		if (omeTiff != nullptr) {
			if (omeTiff->GetFileFullName() == full_path_string) {
				if (omeTiff->GetOpenMode() != OpenMode::READ_ONLY_MODE)
					return ErrorCode::TIFF_ERR_WRONG_OPEN_MODE;
				break;
			}
		}
	}

	omeTiff = new(std::nothrow) OmeTiff();
	if (omeTiff == nullptr) {
		return ErrorCode::TIFF_ERR_ALLOC_MEMORY_FAILED;
	}
	int32_t ret = omeTiff->Init(full_file_name, mode, cm);
	if (ret == ErrorCode::STATUS_OK) {
		for (size_t i = 0; i < vecOmeTiff.size(); i++)
		{
			if (vecOmeTiff[i] == nullptr) {
				vecOmeTiff[i] = omeTiff;
				return (int32_t)i;
			}
		}
		vecOmeTiff.emplace_back(omeTiff);
		return (int32_t)vecOmeTiff.size() - 1;
	}

	delete omeTiff;
	return ret;
}

int32_t ome_close_file(int32_t handle)
{
	CHECK_HANDLE(handle);
	OmeTiff* tiff = vecOmeTiff[handle];
	int32_t status = tiff->CreateOMEHeader();
	delete tiff;
	vecOmeTiff[handle] = nullptr;
	return status;
}

int32_t ome_add_plate(int32_t handle, PlateInfo plates_info)
{
	CHECK_HANDLE(handle);
	return vecOmeTiff[handle]->AddPlate(plates_info);
}

int32_t ome_get_plates(int32_t handle, PlateInfo* plates_info)
{
	CHECK_HANDLE(handle);
	CHECK_BUFFER(plates_info);
	return vecOmeTiff[handle]->GetPlates(plates_info);
}

int32_t ome_get_plates_num(int32_t handle)
{
	CHECK_HANDLE(handle);
	return vecOmeTiff[handle]->GetPlatesSize();
}

int32_t ome_add_well(int32_t handle, uint32_t plate_id, WellInfo well_info)
{
	CHECK_HANDLE(handle);
	return vecOmeTiff[handle]->AddWell(plate_id, well_info);
}

int32_t ome_get_wells(int32_t handle, uint32_t plate_id, WellInfo* wells_info)
{
	CHECK_HANDLE(handle);
	CHECK_BUFFER(wells_info);
	return vecOmeTiff[handle]->GetWells(plate_id, wells_info);
}

int32_t ome_get_wells_num(int32_t handle, uint32_t plate_id)
{
	CHECK_HANDLE(handle);
	return vecOmeTiff[handle]->GetWellsSize(plate_id);
}

int32_t ome_add_scan(int32_t handle, uint32_t plate_id, ScanInfo scan_info)
{
	CHECK_HANDLE(handle);
	return vecOmeTiff[handle]->AddScan(plate_id, scan_info);
}

int32_t ome_get_scans(int32_t handle, uint32_t plate_id, ScanInfo* scans_info)
{
	CHECK_HANDLE(handle);
	CHECK_BUFFER(scans_info);
	return vecOmeTiff[handle]->GetScans(plate_id, scans_info);
}

int32_t ome_get_scans_num(int32_t handle, uint32_t plate_id)
{
	CHECK_HANDLE(handle);
	return vecOmeTiff[handle]->GetScansSize(plate_id);
}

int32_t ome_add_channel(int32_t handle, uint32_t plate_id, uint32_t scan_id, ChannelInfo channel_info)
{
	CHECK_HANDLE(handle);
	return vecOmeTiff[handle]->AddChannel(plate_id, scan_id, channel_info);
}

int32_t ome_get_channels(int32_t handle, uint32_t plate_id, uint32_t scan_id, ChannelInfo* channels_info)
{
	CHECK_HANDLE(handle);
	CHECK_BUFFER(channels_info);
	return vecOmeTiff[handle]->GetChannels(plate_id, scan_id, channels_info);
}

int32_t ome_get_channels_num(int32_t handle, uint32_t plate_id, uint32_t scan_id)
{
	CHECK_HANDLE(handle);
	return vecOmeTiff[handle]->GetChannelsSize(plate_id, scan_id);
}

int32_t ome_remove_channel(int32_t handle, uint32_t plate_id, uint32_t scan_id, uint32_t channel_id)
{
	CHECK_HANDLE(handle);
	return vecOmeTiff[handle]->RemoveChannel(plate_id, scan_id, channel_id);
}

int32_t ome_add_scan_region(int32_t handle, uint32_t plate_id, uint32_t scan_id, uint32_t well_id, ScanRegionInfo scan_region_info)
{
	CHECK_HANDLE(handle);
	return vecOmeTiff[handle]->AddScanRegion(plate_id, scan_id, well_id, scan_region_info);
}

int32_t ome_get_scan_regions(int32_t handle, uint32_t plate_id, uint32_t scan_id, uint32_t well_id, ScanRegionInfo* scan_regions_info)
{
	CHECK_HANDLE(handle);
	CHECK_BUFFER(scan_regions_info);
	return vecOmeTiff[handle]->GetScanRegions(plate_id, scan_id, well_id, scan_regions_info);
}

int32_t ome_get_scan_regions_num(int32_t handle, uint32_t plate_id, uint32_t scan_id, uint32_t well_id)
{
	CHECK_HANDLE(handle);
	return vecOmeTiff[handle]->GetScanRegionsSize(plate_id, scan_id, well_id);
}

int32_t ome_save_tile_data(int32_t handle, void* image_data, FrameInfo frame, uint32_t row, uint32_t column, uint32_t stride)
{
	CHECK_HANDLE(handle);
	CHECK_BUFFER(image_data);
	return vecOmeTiff[handle]->SaveTileData(frame, row, column, image_data, stride);
}

int32_t ome_purge_frame(int32_t handle, FrameInfo frame)
{
	CHECK_HANDLE(handle);
	return vecOmeTiff[handle]->PurgeFrame(frame);
}

int32_t ome_get_raw_data(int32_t handle, FrameInfo frame, OmeRect src_rect, void* image_data, uint32_t stride)
{
	CHECK_HANDLE(handle);
	CHECK_BUFFER(image_data);
	OmeSize dst_size = { src_rect.width,src_rect.height };
	return vecOmeTiff[handle]->LoadRawData(frame, dst_size, src_rect, image_data, stride);
}

int32_t ome_get_raw_tile_data(int32_t handle, ome::FrameInfo frame, uint32_t row, uint32_t column, void* image_data, uint32_t stride)
{
	CHECK_HANDLE(handle);
	CHECK_BUFFER(image_data);
	return vecOmeTiff[handle]->LoadRawData(frame, row, column, image_data, stride);
}

int32_t ome_get_tag(int32_t handle, FrameInfo frame, uint16_t tag_id, uint32_t* tag_size, void* tag_value)
{
	CHECK_HANDLE(handle);
	return vecOmeTiff[handle]->GetTag(frame, tag_id, *tag_size, tag_value);
}

int32_t ome_set_tag(int32_t handle, FrameInfo frame, uint16_t tag_id, TiffTagDataType tag_type, uint32_t tag_count, void* tag_value)
{
	CHECK_HANDLE(handle);
	CHECK_BUFFER(tag_value);
	return vecOmeTiff[handle]->SetTag(frame, tag_id, (uint16_t)tag_type, tag_count, tag_value);
}

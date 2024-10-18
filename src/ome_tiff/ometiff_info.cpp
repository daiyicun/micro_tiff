#include "ometiff_info.h"
#include "..\tinyxml2\tinyxml2.h"

#include <filesystem>

using namespace std;
using namespace tinyxml2;
using namespace ome;
namespace fs = filesystem;

#define XML_SCANF(buffer,format, ...)\
	if(buffer == nullptr)\
		return ErrorCode::ERR_BUFFER_IS_NULL;\
	else\
	{\
		int result = sscanf_s(buffer,format,__VA_ARGS__);\
		if (result <= 0)\
			return ErrorCode::ERR_SCANF_ASSIGNED_ERROR;\
	}\

const char um[4] = { (char)194,(char)181,(char)109,(char)0 };
const char us[4] = { (char)194,(char)181,(char)115,(char)0 };

static char* convert_double_to_string(double value, char* str)
{
	char str2[_MAX_PATH] = { 0 };
	sprintf_s(str2, "%.3f", value);
	strcpy_s(str, _MAX_PATH, str2);
	return str;
}

static const char* convert_distance_unit_to_string(DistanceUnit unit)
{
	switch (unit)
	{
	case DistanceUnit::DISTANCE_KILOMETER:
		return "km";
	case DistanceUnit::DISTANCE_METER:
		return "m";
	case DistanceUnit::DISTANCE_MILLIMETER:
		return "mm";
	case DistanceUnit::DISTANCE_MICROMETER:
		return um;
	case DistanceUnit::DISTANCE_NANOMETER:
		return "nm";
	case DistanceUnit::DISTANCE_PICOMETER:
		return "pm";
	default:
		return "mm";
	}
}

static const char* convert_shape_to_string(Shape shape)
{
	switch (shape)
	{
	case Shape::SHAPE_RECTANGLE:
		return "Rectangle";
	case Shape::SHAPE_ELLIPSE:
		return "Ellipse";
	default:
		return "Rectangle";
	}
}

static const char* convert_pixel_type_to_string(PixelType pixelType)
{
	switch (pixelType)
	{
	case PixelType::PIXEL_INT8:
		return "int8";
	case PixelType::PIXEL_INT16:
		return "int16";
	case PixelType::PIXEL_UINT8:
		return "uint8";
	case PixelType::PIXEL_UINT16:
		return "uint16";
	case PixelType::PIXEL_FLOAT32:
		return "float";
	default:
		return "uint16";
	}
}

static Shape convert_string_to_shape(const char* str)
{
	if (str == nullptr)
		return Shape::SHAPE_UNDEFINED;
	if (memcmp(str, "Rectangle", strlen(str)) == 0)
		return Shape::SHAPE_RECTANGLE;
	if (memcmp(str, "Ellipse", strlen(str)) == 0)
		return Shape::SHAPE_ELLIPSE;
	return Shape::SHAPE_UNDEFINED;
}

static const char* convert_timeunit_to_string(TimeUnit unit)
{
	switch (unit)
	{
	case TimeUnit::TIME_SECOND:
		return "s";
	case TimeUnit::TIME_MILLISECOND:
		return "ms";
	case TimeUnit::TIME_MICROSECOND:
		return us;
	case TimeUnit::TIME_NANOSECOND:
		return "ns";
	case TimeUnit::TIME_PICOSECOND:
		return "ps";
	case TimeUnit::TIME_FEMTOSECOND:
		return "fs";
	default:
		return "s";
	}
}

static DistanceUnit convert_string_to_distance_unit(const char* str)
{
	if (str == nullptr)
		return DistanceUnit::DISTANCE_UNDEFINED;
	string s_distance(str);
	transform(s_distance.begin(), s_distance.end(), s_distance.begin(), ::tolower);
	if (s_distance.compare("m") == 0)
		return DistanceUnit::DISTANCE_METER;
	if (s_distance.compare("km") == 0)
		return DistanceUnit::DISTANCE_KILOMETER;
	if (s_distance.compare("mm") == 0)
		return DistanceUnit::DISTANCE_MILLIMETER;
	if (s_distance.compare(um) == 0)
		return DistanceUnit::DISTANCE_MICROMETER;
	if (s_distance.compare("nm") == 0)
		return DistanceUnit::DISTANCE_NANOMETER;
	if (s_distance.compare("pm") == 0)
		return DistanceUnit::DISTANCE_PICOMETER;
	return DistanceUnit::DISTANCE_UNDEFINED;
}

static PixelType convert_string_pixel_type(const char* type)
{
	if (type == nullptr)
		return PixelType::PIXEL_UNDEFINED;
	string s_type(type);
	transform(s_type.begin(), s_type.end(), s_type.begin(), ::tolower);
	if (s_type.compare("uint16") == 0)
		return PixelType::PIXEL_UINT16;
	if (s_type.compare("int16") == 0)
		return PixelType::PIXEL_INT16;
	if (s_type.compare("uint8") == 0)
		return PixelType::PIXEL_UINT8;
	if (s_type.compare("int8") == 0)
		return PixelType::PIXEL_INT8;
	if (s_type.compare("float") == 0)
		return PixelType::PIXEL_FLOAT32;
	return PixelType::PIXEL_UNDEFINED;
}

static TimeUnit convert_string_to_time_unit(const char* str)
{
	if (str == nullptr)
		return TimeUnit::TIME_UNDEFINED;
	string s_time(str);
	transform(s_time.begin(), s_time.end(), s_time.begin(), ::tolower);
	if (s_time.compare("s") == 0)
		return TimeUnit::TIME_SECOND;
	if (s_time.compare("ms") == 0)
		return TimeUnit::TIME_MILLISECOND;
	if (s_time.compare(us) == 0)
		return TimeUnit::TIME_MICROSECOND;
	if (s_time.compare("ns") == 0)
		return TimeUnit::TIME_NANOSECOND;
	if (s_time.compare("ps") == 0)
		return TimeUnit::TIME_PICOSECOND;
	if (s_time.compare("fs") == 0)
		return TimeUnit::TIME_FEMTOSECOND;
	return TimeUnit::TIME_UNDEFINED;
}

int32_t parse_ome_xml(const string& xml, OmeTiff* tiff_obj)
{
	tinyxml2::XMLDocument doc;
	if (doc.Parse(xml.c_str(), xml.size()) != XMLError::XML_SUCCESS)
		return ErrorCode::ERR_READ_OME_XML_FAILED;

	const XMLElement* element_ome = doc.FirstChildElement("OME");
	if (element_ome == nullptr)
		return ErrorCode::ERR_XML_PARSE_FAILED;

	int32_t status = ErrorCode::STATUS_OK;

	const XMLElement* element_image = element_ome->FirstChildElement("Image");
	while (element_image)
	{
		const char* image_id_str = element_image->Attribute("ID");
		uint32_t image_id = 0;
		XML_SCANF(image_id_str, "Image:%u", &image_id);
		//skip boot image which ID="Image:0"
		if (image_id == 0)
		{
			element_image = element_image->NextSiblingElement("Image");
			continue;
		}

		Image* image = new Image();
		image->_id = string(image_id_str);
		if (element_image->Attribute("Name") != nullptr) {
			image->_name = string(element_image->Attribute("Name"));
		}

		const XMLElement* element_pixels = element_image->FirstChildElement("Pixels");
		if (element_pixels == nullptr)
			return ErrorCode::ERR_NO_PIXELS_IN_IMAGE;

		image->_pixels._info.id = string(element_pixels->Attribute("ID"));
		image->_pixels._info.dimension_order = string(element_pixels->Attribute("DimensionOrder"));

		XML_SCANF(element_pixels->Attribute("SizeX"), "%u", &image->_pixels._info.size_x);
		XML_SCANF(element_pixels->Attribute("SizeY"), "%u", &image->_pixels._info.size_y);
		XML_SCANF(element_pixels->Attribute("SizeZ"), "%u", &image->_pixels._info.size_z);
		XML_SCANF(element_pixels->Attribute("SizeT"), "%u", &image->_pixels._info.size_t);

		XML_SCANF(element_pixels->Attribute("PhysicalSizeX"), "%f", &image->_pixels._info.physical_size_per_pixel_x);
		XML_SCANF(element_pixels->Attribute("PhysicalSizeY"), "%f", &image->_pixels._info.physical_size_per_pixel_y);
		XML_SCANF(element_pixels->Attribute("PhysicalSizeZ"), "%f", &image->_pixels._info.physical_size_per_pixel_z);
		XML_SCANF(element_pixels->Attribute("TimeIncrement"), "%f", &image->_pixels._info.time_increment);

		image->_pixels._info.physical_x_unit = convert_string_to_distance_unit(element_pixels->Attribute("PhysicalSizeXUnit"));
		image->_pixels._info.physical_y_unit = convert_string_to_distance_unit(element_pixels->Attribute("PhysicalSizeYUnit"));
		image->_pixels._info.physical_z_unit = convert_string_to_distance_unit(element_pixels->Attribute("PhysicalSizeZUnit"));
		if (element_pixels->Attribute("TimeIncrementUnit") != nullptr) {
			image->_pixels._info.time_increment_unit = convert_string_to_time_unit(element_pixels->Attribute("TimeIncrementUnit"));
		}
		else {
			image->_pixels._info.time_increment_unit = TimeUnit::TIME_UNDEFINED;
		}

		XML_SCANF(element_pixels->Attribute("TileWidth"), "%u", &image->_pixels._info.tile_pixel_width);
		XML_SCANF(element_pixels->Attribute("TileHeight"), "%u", &image->_pixels._info.tile_pixel_height);
		image->_pixels._info.pixel_type = convert_string_pixel_type(element_pixels->Attribute("Type"));

		const char* attri_sig = element_pixels->Attribute("SignificantBits");
		if (attri_sig == nullptr) {
			if (image->_pixels._info.pixel_type == PixelType::PIXEL_INT8 || image->_pixels._info.pixel_type == PixelType::PIXEL_UINT8)
				image->_pixels._info.significant_bits = 8;
			else if (image->_pixels._info.pixel_type == PixelType::PIXEL_INT16 || image->_pixels._info.pixel_type == PixelType::PIXEL_UINT16)
				image->_pixels._info.significant_bits = 16;
			else if (image->_pixels._info.pixel_type == PixelType::PIXEL_FLOAT32)
				image->_pixels._info.significant_bits = 32;
			else
				return ErrorCode::ERR_SIGNIFICATION_BITS;
		}
		else {
			XML_SCANF(attri_sig, "%u", &image->_pixels._info.significant_bits);
		}

		vector<uint32_t> channel_ids;
		const XMLElement* element_channel = element_pixels->FirstChildElement("Channel");
		while (element_channel)
		{
			ChannelInfo channel_info = { 0 };
			const char* channel_id = element_channel->Attribute("ID");
			XML_SCANF(channel_id, "Channel:%u", &channel_info.id);
			const char* channel_name = element_channel->Attribute("Name");
			if (channel_name)
			{
				fs::path p = fs::u8path(channel_name);
				wstring wstr = p.wstring();
				wmemcpy_s(channel_info.name, NAME_LEN, wstr.c_str(), wstr.size());
			}
			else
			{
				fs::path p = fs::u8path(channel_id);
				wstring wstr = p.wstring();
				wmemcpy_s(channel_info.name, NAME_LEN, wstr.c_str(), wstr.size());
			}

			XML_SCANF(element_channel->Attribute("SamplesPerPixel"), "%u", &channel_info.sample_per_pixel);
			if (element_channel->Attribute("BinSize") != nullptr) {
				XML_SCANF(element_channel->Attribute("BinSize"), "%u", &channel_info.bin_size);
			}
			else {
				channel_info.bin_size = 1;
			}
			status = image->_pixels.add_channel(channel_info);
			if (status != ErrorCode::STATUS_OK)
				return status;

			channel_ids.push_back(channel_info.id);
			element_channel = element_channel->NextSiblingElement("Channel");
		}

		vector<TiffData> tiff_datas;
		const XMLElement* element_tiff_data = element_pixels->FirstChildElement("TiffData");
		while (element_tiff_data)
		{
			const XMLElement* element_UUID = element_tiff_data->FirstChildElement("UUID");
			if (element_UUID == nullptr)
				return ErrorCode::ERR_TIFF_DATA_HAS_NO_UUID;

			const char* UUID_file = element_UUID->Attribute("FileName");

			TiffData tiff_data = { 0 };
			XML_SCANF(element_tiff_data->Attribute("FirstC"), "%u", &tiff_data.FirstC);
			XML_SCANF(element_tiff_data->Attribute("FirstT"), "%u", &tiff_data.FirstT);
			XML_SCANF(element_tiff_data->Attribute("FirstZ"), "%u", &tiff_data.FirstZ);
			XML_SCANF(element_tiff_data->Attribute("IFD"), "%u", &tiff_data.IFD);
			tiff_data.FileName = UUID_file;

			string full_path = tiff_obj->GetFullPathWithFileName(tiff_data.FileName);
			fs::path p_full{ full_path };
			if (!fs::exists(p_full))
			{
				image->_pixels.remove_channel(tiff_data.FirstC);
				auto it_channel = find(channel_ids.begin(), channel_ids.end(), tiff_data.FirstC);
				if (it_channel != channel_ids.end())
					channel_ids.erase(it_channel);
				element_tiff_data = element_tiff_data->NextSiblingElement("TiffData");
				continue;
			}
			else
			{
				auto it_channel = find(channel_ids.begin(), channel_ids.end(), tiff_data.FirstC);
				if (it_channel == channel_ids.end())
				{
					element_tiff_data = element_tiff_data->NextSiblingElement("TiffData");
					continue;
				}
			}

			status = image->_pixels.add_tiff_data(tiff_data);
			if (status != ErrorCode::STATUS_OK)
				return status;

			tiff_datas.push_back(tiff_data);
			element_tiff_data = element_tiff_data->NextSiblingElement("TiffData");
		}

		uint32_t channel_size = (uint32_t)(channel_ids.size());
		uint32_t total_tiff_count = image->_pixels._info.size_t * image->_pixels._info.size_z * channel_size;
		uint32_t tiff_data_size = (uint32_t)tiff_datas.size();
		if (tiff_data_size != total_tiff_count)
		{
			if (tiff_data_size == 1)
			{
				TiffData first_tiff = tiff_datas.front();
				uint32_t ifd_no = first_tiff.IFD;
				for (size_t i = 0; i < channel_ids.size(); i++)
				{
					uint32_t c = channel_ids[i];
					for (uint32_t t = first_tiff.FirstT; t < image->_pixels._info.size_t; t++)
					{
						for (uint32_t z = first_tiff.FirstZ; z < image->_pixels._info.size_z; z++)
						{
							if (first_tiff.FirstC == c && t == first_tiff.FirstT && z == first_tiff.FirstZ)
								continue;

							TiffData tiff_data = { 0 };
							tiff_data.FirstC = c;
							tiff_data.FirstT = t;
							tiff_data.FirstZ = z;
							tiff_data.IFD = ++ifd_no;
							tiff_data.FileName = first_tiff.FileName;

							status = image->_pixels.add_tiff_data(tiff_data);
							if (status != ErrorCode::STATUS_OK)
								return status;
						}
					}
				}
			}
			else if (tiff_data_size == channel_size)
			{
				for (size_t i = 0; i < channel_ids.size(); i++)
				{
					uint32_t c = channel_ids[i];
					auto it = find_if(tiff_datas.begin(), tiff_datas.end(), [&](const TiffData& data) { return data.FirstC == c; });
					if (it != tiff_datas.end())
					{
						TiffData tiff_data_c = *it;
						uint32_t ifd_no = tiff_data_c.IFD;
						for (uint32_t t = tiff_data_c.FirstT; t < image->_pixels._info.size_t; t++)
						{
							for (uint32_t z = tiff_data_c.FirstZ; z < image->_pixels._info.size_z; z++)
							{
								if (tiff_data_c.FirstC == c && t == tiff_data_c.FirstT && z == tiff_data_c.FirstZ)
									continue;

								TiffData tiff_data = { 0 };
								tiff_data.FirstC = c;
								tiff_data.FirstT = t;
								tiff_data.FirstZ = z;
								tiff_data.IFD = ++ifd_no;
								tiff_data.FileName = tiff_data_c.FileName;

								status = image->_pixels.add_tiff_data(tiff_data);
								if (status != ErrorCode::STATUS_OK)
									return status;
							}
						}
					}
				}
			}
			else
				return ErrorCode::ERR_TIFF_DATA_CANNOT_PREDICT;
		}

		auto it_image = tiff_obj->_images.find(image_id);
		if (it_image != tiff_obj->_images.end())
			return ErrorCode::ERR_IMAGE_ID_EXIST;

		tiff_obj->_images[image_id] = image;
		element_image = element_image->NextSiblingElement("Image");
	}

	map<string, ScanRegionInfo> scan_region_infos;
	map<string, string> well_sample_id_image_ref_map;

	const XMLElement* element_plate = element_ome->FirstChildElement("Plate");
	while (element_plate)
	{
		PlateInfo plate_info = { 0 };
		XML_SCANF(element_plate->Attribute("ID"), "Plate:%u", &plate_info.id);
		const char* name = element_plate->Attribute("Name");
		fs::path p = fs::u8path(name);
		wstring wstr = p.wstring();
		wmemcpy_s(plate_info.name, NAME_LEN, wstr.c_str(), wstr.size());
		XML_SCANF(element_plate->Attribute("Width"), "%f", &plate_info.width);
		XML_SCANF(element_plate->Attribute("Height"), "%f", &plate_info.height);
		plate_info.physicalsize_unit_x = convert_string_to_distance_unit(element_plate->Attribute("PhysicalSizeXUnit"));
		plate_info.physicalsize_unit_y = convert_string_to_distance_unit(element_plate->Attribute("PhysicalSizeYUnit"));
		if (element_plate->Attribute("Rows") != nullptr) {
			XML_SCANF(element_plate->Attribute("Rows"), "%hu", &plate_info.row_size);
			XML_SCANF(element_plate->Attribute("Columns"), "%hu", &plate_info.column_size);
		}

		status = tiff_obj->AddPlate(plate_info);
		if (status != ErrorCode::STATUS_OK)
			return status;

		const XMLElement* element_well = element_plate->FirstChildElement("Well");
		while (element_well)
		{
			WellInfo well_info = { 0 };
			XML_SCANF(element_well->Attribute("ID"), "Well:%*u.%u", &well_info.id);
			XML_SCANF(element_well->Attribute("PositionX"), "%f", &well_info.position_x);
			XML_SCANF(element_well->Attribute("PositionY"), "%f", &well_info.position_y);
			XML_SCANF(element_well->Attribute("Width"), "%f", &well_info.width);
			XML_SCANF(element_well->Attribute("Height"), "%f", &well_info.height);
			well_info.well_shape = convert_string_to_shape(element_well->Attribute("Shape"));
			if (element_well->Attribute("Row") != nullptr) {
				XML_SCANF(element_well->Attribute("Row"), "%hu", &well_info.row_index);
				XML_SCANF(element_well->Attribute("Column"), "%hu", &well_info.column_index);
			}

			status = tiff_obj->AddWell(plate_info.id, well_info);
			if (status != ErrorCode::STATUS_OK)
				return status;

			const XMLElement* element_well_sample = element_well->FirstChildElement("WellSample");
			while (element_well_sample)
			{
				WellSample well_sample = { 0 };
				uint32_t scan_id = 0;
				const char* well_sample_id = element_well_sample->Attribute("ID");
				well_sample.id = string(well_sample_id);
				XML_SCANF(well_sample_id, "WellSample:%*u.%*u.%u", &scan_id);
				XML_SCANF(element_well_sample->Attribute("PositionX"), "%f", &well_sample.position_x);
				XML_SCANF(element_well_sample->Attribute("PositionY"), "%f", &well_sample.position_y);
				XML_SCANF(element_well_sample->Attribute("PositionZ"), "%f", &well_sample.position_z);
				well_sample.physicalsize_unit_x = convert_string_to_distance_unit(element_well_sample->Attribute("PositionXUnit"));
				well_sample.physicalsize_unit_y = convert_string_to_distance_unit(element_well_sample->Attribute("PositionYUnit"));
				well_sample.physicalsize_unit_z = convert_string_to_distance_unit(element_well_sample->Attribute("PositionZUnit"));

				const XMLElement* element_image_ref = element_well_sample->FirstChildElement("ImageRef");
				if (element_image_ref == nullptr)
					return ErrorCode::ERR_NO_IMAGE_REF_IN_WELL_SAMPLE;

				const char* image_ref_id = element_image_ref->Attribute("ID");
				well_sample.image_ref_id = string(image_ref_id);

				uint32_t image_id_u;
				XML_SCANF(image_ref_id, "Image:%u", &image_id_u);

				auto it_image = tiff_obj->_images.find(image_id_u);
				if (it_image == tiff_obj->_images.end())
					return ErrorCode::ERR_CANNOT_FIND_IMAGE;

				Image* image = it_image->second;

				ScanRegionInfo info = { 0 };
				info.start_physical_x = well_sample.position_x;
				info.start_physical_y = well_sample.position_y;
				info.start_physical_z = well_sample.position_z;
				info.start_unit_x = well_sample.physicalsize_unit_x;
				info.start_unit_y = well_sample.physicalsize_unit_y;
				info.start_unit_z = well_sample.physicalsize_unit_z;
				info.pixel_size_x = image->_pixels._info.size_x;
				info.pixel_size_y = image->_pixels._info.size_y;
				info.pixel_size_z = image->_pixels._info.size_z;
				info.size_t = image->_pixels._info.size_t;
				scan_region_infos.insert(make_pair(well_sample_id, info));
				well_sample_id_image_ref_map.insert(make_pair(well_sample_id, image_ref_id));

				element_well_sample = element_well_sample->NextSiblingElement("WellSample");
			}

			element_well = element_well->NextSiblingElement("Well");
		}

		const XMLElement* element_plate_acquisition = element_plate->FirstChildElement("PlateAcquisition");
		while (element_plate_acquisition)
		{
			const char* plate_acquisition_id = element_plate_acquisition->Attribute("ID");
			uint32_t inner_plate_id = 0;
			uint32_t acquisition_id = 0;
			XML_SCANF(plate_acquisition_id, "PlateAcquisition:%u.%u", &inner_plate_id, &acquisition_id);
			if (inner_plate_id != plate_info.id)
				return ErrorCode::ERR_PLATE_ID_NOT_MATCHED;

			bool is_scan_set = false;
			const XMLElement* element_well_sample_ref = element_plate_acquisition->FirstChildElement("WellSampleRef");
			while (element_well_sample_ref)
			{
				uint32_t region_id = 0;
				XML_SCANF(element_well_sample_ref->Attribute("RegionID"), "%u", &region_id);
				const char* well_sample_ref_id = element_well_sample_ref->Attribute("ID");
				if (well_sample_ref_id == nullptr)
					return ErrorCode::ERR_WELL_SAMPLE_REF_ID_EMPTY;

				uint32_t well_id = 0;
				XML_SCANF(well_sample_ref_id, "WellSample:%*u.%u.%*u", &well_id);
				auto it_image_ref = well_sample_id_image_ref_map.find(well_sample_ref_id);

				if (!is_scan_set && it_image_ref != well_sample_id_image_ref_map.end())
				{
					uint32_t image_id_u;
					XML_SCANF(it_image_ref->second.c_str(), "Image:%u", &image_id_u);

					auto it_image = tiff_obj->_images.find(image_id_u);
					if (it_image == tiff_obj->_images.end())
						return ErrorCode::ERR_CANNOT_FIND_IMAGE;

					Image* image = it_image->second;

					ScanInfo scan_info = { 0 };
					scan_info.id = acquisition_id;
					memcpy_s(scan_info.dimension_order, NAME_LEN, image->_pixels._info.dimension_order.c_str(), image->_pixels._info.dimension_order.size());
					scan_info.pixel_physical_size_x = image->_pixels._info.physical_size_per_pixel_x;
					scan_info.pixel_physical_size_y = image->_pixels._info.physical_size_per_pixel_y;
					scan_info.pixel_physical_size_z = image->_pixels._info.physical_size_per_pixel_z;
					scan_info.time_increment = image->_pixels._info.time_increment;
					scan_info.pixel_physical_uint_x = image->_pixels._info.physical_x_unit;
					scan_info.pixel_physical_uint_y = image->_pixels._info.physical_y_unit;
					scan_info.pixel_physical_uint_z = image->_pixels._info.physical_z_unit;
					scan_info.time_increment_unit = image->_pixels._info.time_increment_unit;
					scan_info.tile_pixel_size_width = image->_pixels._info.tile_pixel_width;
					scan_info.tile_pixel_size_height = image->_pixels._info.tile_pixel_height;
					scan_info.significant_bits = image->_pixels._info.significant_bits;
					scan_info.pixel_type = image->_pixels._info.pixel_type;

					status = tiff_obj->AddScan(inner_plate_id, scan_info);
					if (status != ErrorCode::STATUS_OK)
						return status;

					for (auto it_channel = image->_pixels._channels.begin(); it_channel != image->_pixels._channels.end(); it_channel++)
					{
						status = tiff_obj->AddChannel(plate_info.id, acquisition_id, it_channel->second._info);
						if (status != ErrorCode::STATUS_OK)
							return status;
					}

					is_scan_set = true;
				}

				auto it_scan_region_info = scan_region_infos.find(well_sample_ref_id);
				if (it_image_ref != well_sample_id_image_ref_map.end() && it_scan_region_info != scan_region_infos.end())
				{
					it_scan_region_info->second.id = region_id;

					status = tiff_obj->AddScanRegion(plate_info.id, acquisition_id, well_id, it_scan_region_info->second, it_image_ref->first, it_image_ref->second);
					if (status != ErrorCode::STATUS_OK)
						return status;
				}

				element_well_sample_ref = element_well_sample_ref->NextSiblingElement("WellSampleRef");
			}
			element_plate_acquisition = element_plate_acquisition->NextSiblingElement("PlateAcquisition");
		}

		element_plate = element_plate->NextSiblingElement("Plate");
	}

	return ErrorCode::STATUS_OK;
}

int32_t generate_ome_xml(string& xml, const OmeTiff* tiff_obj)
{
	tinyxml2::XMLDocument doc;
	XMLDeclaration* bom = doc.NewDeclaration();
	doc.InsertFirstChild(bom);
	const char* comment = "Warning: this comment is an OME-XML metadata block, which contains crucial dimensional parameters and other important metadata. Please edit cautiously (if at all), and back up the original data before doing so. For more information, see the OME-TIFF web site: http://www.openmicroscopy.org/site/support/ome-model/ome-tiff/.";
	doc.InsertEndChild(doc.NewComment(comment));

	XMLElement* element_OME = doc.NewElement("OME");
	element_OME->SetAttribute("xmlns", "http://www.openmicroscopy.org/Schemas/OME/2016-06");
	element_OME->SetAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
	element_OME->SetAttribute("xsi:schemaLocation", "http://www.openmicroscopy.org/Schemas/OME/2016-06 http://www.openmicroscopy.org/Schemas/OME/2016-06/ome.xsd");
	doc.InsertEndChild(element_OME);

	XMLElement* element_boot_Image = doc.NewElement("Image");
	element_OME->InsertEndChild(element_boot_Image);
	element_boot_Image->SetAttribute("ID", "Image:0");
	XMLElement* element_boot_Pixels = doc.NewElement("Pixels");
	element_boot_Image->InsertEndChild(element_boot_Pixels);
	element_boot_Pixels->SetAttribute("ID", "Pixels:0");
	element_boot_Pixels->SetAttribute("DimensionOrder", "XYCZT");
	element_boot_Pixels->SetAttribute("Type", "uint8");
	element_boot_Pixels->SetAttribute("SizeC", 1);
	element_boot_Pixels->SetAttribute("SizeT", 1);
	element_boot_Pixels->SetAttribute("SizeX", HEADERIMAGESIZE);
	element_boot_Pixels->SetAttribute("SizeY", HEADERIMAGESIZE);
	element_boot_Pixels->SetAttribute("SizeZ", 1);
	element_boot_Pixels->SetAttribute("SizeS", 1);
	element_boot_Pixels->SetAttribute("TileWidth", HEADERIMAGESIZE);
	element_boot_Pixels->SetAttribute("TileHeight", HEADERIMAGESIZE);
	XMLElement* element_boot_channel = doc.NewElement("Channel");
	element_boot_Pixels->InsertEndChild(element_boot_channel);
	element_boot_channel->SetAttribute("ID", "Channel:0");
	element_boot_channel->SetAttribute("SamplesPerPixel", 1);
	XMLElement* element_boot_TiffData = doc.NewElement("TiffData");
	element_boot_Pixels->InsertEndChild(element_boot_TiffData);
	element_boot_TiffData->SetAttribute("FirstC", 0);
	element_boot_TiffData->SetAttribute("FirstT", 0);
	element_boot_TiffData->SetAttribute("FirstZ", 0);
	element_boot_TiffData->SetAttribute("FirstS", 0);
	element_boot_TiffData->SetAttribute("IFD", 0);
	XMLElement* element_boot_UUID = doc.NewElement("UUID");
	string file_name = tiff_obj->GetUTF8FileName();
	element_boot_UUID->SetAttribute("FileName", file_name.c_str());
	element_boot_TiffData->InsertEndChild(element_boot_UUID);

	int32_t status = ErrorCode::STATUS_OK;
	char tmp[_MAX_PATH] = { 0 };
	for (auto it = tiff_obj->_plates.begin(); it != tiff_obj->_plates.end(); it++)
	{
		Plate* plate = it->second;
		PlateInfo plate_info = plate->_info;

		XMLElement* element_Plate = doc.NewElement("Plate");
		element_OME->InsertEndChild(element_Plate);
		memset(tmp, 0, _MAX_PATH);
		sprintf_s(tmp, "Plate:%hd", plate_info.id);
		element_Plate->SetAttribute("ID", tmp);
		fs::path p(plate_info.name);
		string plate_name_str = p.u8string();
		element_Plate->SetAttribute("Name", plate_name_str.c_str());
		element_Plate->SetAttribute("Width", convert_double_to_string(plate_info.width, tmp));
		element_Plate->SetAttribute("Height", convert_double_to_string(plate_info.height, tmp));
		element_Plate->SetAttribute("PhysicalSizeXUnit", convert_distance_unit_to_string(plate_info.physicalsize_unit_x));
		element_Plate->SetAttribute("PhysicalSizeYUnit", convert_distance_unit_to_string(plate_info.physicalsize_unit_y));
		element_Plate->SetAttribute("Rows", plate_info.row_size);
		element_Plate->SetAttribute("Columns", plate_info.column_size);

		for (auto it_well = plate->_wells_array.begin(); it_well != plate->_wells_array.end(); it_well++)
		{
			WellInfo well_info = it_well->second._info;
			XMLElement* element_Well = doc.NewElement("Well");
			element_Plate->InsertEndChild(element_Well);
			memset(tmp, 0, _MAX_PATH);
			sprintf_s(tmp, "Well:%u.%u", plate_info.id, well_info.id);
			element_Well->SetAttribute("ID", tmp);
			element_Well->SetAttribute("PositionX", convert_double_to_string(well_info.position_x, tmp));
			element_Well->SetAttribute("PositionY", convert_double_to_string(well_info.position_y, tmp));
			element_Well->SetAttribute("Width", convert_double_to_string(well_info.width, tmp));
			element_Well->SetAttribute("Height", convert_double_to_string(well_info.height, tmp));
			element_Well->SetAttribute("Shape", convert_shape_to_string(well_info.well_shape));
			element_Well->SetAttribute("Row", well_info.row_index);
			element_Well->SetAttribute("Column", well_info.column_index);

			for (auto it_well_sample = it_well->second._well_sample_array.begin(); it_well_sample != it_well->second._well_sample_array.end(); it_well_sample++)
			{
				WellSample wellSample = it_well_sample->second;
				XMLElement* element_WellSample = doc.NewElement("WellSample");
				element_Well->InsertEndChild(element_WellSample);
				element_WellSample->SetAttribute("ID", wellSample.id.c_str());
				element_WellSample->SetAttribute("PositionX", convert_double_to_string(wellSample.position_x, tmp));
				element_WellSample->SetAttribute("PositionY", convert_double_to_string(wellSample.position_y, tmp));
				element_WellSample->SetAttribute("PositionZ", convert_double_to_string(wellSample.position_z, tmp));
				element_WellSample->SetAttribute("PositionXUnit", convert_distance_unit_to_string(wellSample.physicalsize_unit_x));
				element_WellSample->SetAttribute("PositionYUnit", convert_distance_unit_to_string(wellSample.physicalsize_unit_y));
				element_WellSample->SetAttribute("PositionZUnit", convert_distance_unit_to_string(wellSample.physicalsize_unit_z));

				XMLElement* element_imageref = doc.NewElement("ImageRef");
				element_WellSample->InsertEndChild(element_imageref);
				element_imageref->SetAttribute("ID", wellSample.image_ref_id.c_str());
			}
		}

		for (auto it_plate_acquisition = plate->_plate_acquisition_array.begin(); it_plate_acquisition != plate->_plate_acquisition_array.end(); it_plate_acquisition++)
		{
			PlateAcquisition acquisition = it_plate_acquisition->second;

			XMLElement* element_plate_acquisition = doc.NewElement("PlateAcquisition");
			element_Plate->InsertEndChild(element_plate_acquisition);
			element_plate_acquisition->SetAttribute("ID", acquisition._id.c_str());

			for (auto it_ref = acquisition._well_sample_ref.begin(); it_ref != acquisition._well_sample_ref.end(); it_ref++)
			{
				XMLElement* element_wellsampleref = doc.NewElement("WellSampleRef");
				element_wellsampleref->SetAttribute("ID", it_ref->second.c_str());
				element_wellsampleref->SetAttribute("RegionID", it_ref->first);
				element_plate_acquisition->InsertEndChild(element_wellsampleref);
			}
		}
	}

	for (auto it_image = tiff_obj->_images.begin(); it_image != tiff_obj->_images.end(); it_image++)
	{
		Image* image = it_image->second;
		XMLElement* element_Image = doc.NewElement("Image");
		element_OME->InsertEndChild(element_Image);
		element_Image->SetAttribute("Name", image->_name.c_str());
		element_Image->SetAttribute("ID", image->_id.c_str());
		XMLElement* element_Pixels = doc.NewElement("Pixels");
		element_Image->InsertEndChild(element_Pixels);

		Pixels pixels = image->_pixels;
		PixelsInfo pix = pixels._info;

		uint32_t channel_size = (uint32_t)pixels._channels.size();
		if (channel_size == 0)
			return ErrorCode::ERR_NO_CHANNELS;

		element_Pixels->SetAttribute("ID", pix.id.c_str());
		element_Pixels->SetAttribute("DimensionOrder", pix.dimension_order.c_str());
		element_Pixels->SetAttribute("PhysicalSizeX", convert_double_to_string(pix.physical_size_per_pixel_x != 0 ? pix.physical_size_per_pixel_x : 1, tmp));
		element_Pixels->SetAttribute("PhysicalSizeY", convert_double_to_string(pix.physical_size_per_pixel_y != 0 ? pix.physical_size_per_pixel_y : 1, tmp));
		element_Pixels->SetAttribute("PhysicalSizeZ", convert_double_to_string(pix.physical_size_per_pixel_z != 0 ? pix.physical_size_per_pixel_z : 1, tmp));
		element_Pixels->SetAttribute("PhysicalSizeXUnit", convert_distance_unit_to_string(pix.physical_x_unit));
		element_Pixels->SetAttribute("PhysicalSizeYUnit", convert_distance_unit_to_string(pix.physical_y_unit));
		element_Pixels->SetAttribute("PhysicalSizeZUnit", convert_distance_unit_to_string(pix.physical_z_unit));
		element_Pixels->SetAttribute("TimeIncrement", convert_double_to_string(pix.time_increment, tmp));
		element_Pixels->SetAttribute("TimeIncrementUnit", convert_timeunit_to_string(pix.time_increment_unit));
		element_Pixels->SetAttribute("Type", convert_pixel_type_to_string(pix.pixel_type));
		element_Pixels->SetAttribute("SizeC", channel_size);
		element_Pixels->SetAttribute("SizeT", pixels.get_t_size());
		element_Pixels->SetAttribute("SizeX", pix.size_x);
		element_Pixels->SetAttribute("SizeY", pix.size_y);
		element_Pixels->SetAttribute("SizeZ", pix.size_z);
		element_Pixels->SetAttribute("TileWidth", pix.tile_pixel_width);
		element_Pixels->SetAttribute("TileHeight", pix.tile_pixel_height);
		element_Pixels->SetAttribute("SignificantBits", pix.significant_bits);

		for (auto it_channel = pixels._channels.begin(); it_channel != pixels._channels.end(); it_channel++)
		{
			ChannelInfo channel_info = it_channel->second._info;
			XMLElement* element_Channel = doc.NewElement("Channel");
			element_Pixels->InsertEndChild(element_Channel);
			memset(tmp, 0, _MAX_PATH);
			sprintf_s(tmp, "Channel:%u", channel_info.id);
			element_Channel->SetAttribute("ID", tmp);
			fs::path p(channel_info.name);
			string channel_name_str = p.u8string();
			element_Channel->SetAttribute("Name", channel_name_str.c_str());
			element_Channel->SetAttribute("SamplesPerPixel", channel_info.sample_per_pixel);
			element_Channel->SetAttribute("BinSize", channel_info.bin_size);
		}

		bool is_ordered = false;
		//Check if the sequence of data is ordered
		if (pix.dimension_order == "XYZTC")
		{
			is_ordered = true;
			for (auto it_channel = pixels._channels.begin(); it_channel != pixels._channels.end(); it_channel++)
			{
				uint32_t c_id = it_channel->second._info.id;
				
				TiffData tiff_data;
				status = pixels.get_tiff_data(c_id, 0, 0, tiff_data);
				if (status != ErrorCode::STATUS_OK)
					return status;

				uint32_t start_ifd_no = tiff_data.IFD;
				for (uint32_t t = 0; t < pixels.get_t_size(); t++)
				{
					for (uint32_t z = 0; z < pixels._info.size_z; z++)
					{
						if (z == 0 && t == 0)
							continue;

						status = pixels.get_tiff_data(c_id, z, t, tiff_data);
						if (status != ErrorCode::STATUS_OK)
							return status;

						if (tiff_data.IFD != ++start_ifd_no)
						{
							is_ordered = false;
							break;
						}
					}
					if (!is_ordered)
						break;
				}
				if (!is_ordered)
					break;
			}
		}

		auto insertTiffDataFunc = [&doc, &element_Pixels](const TiffData& tiff_data)
			{
				XMLElement* element_TiffData = doc.NewElement("TiffData");
				element_Pixels->InsertEndChild(element_TiffData);
				element_TiffData->SetAttribute("FirstC", tiff_data.FirstC);
				element_TiffData->SetAttribute("FirstT", tiff_data.FirstT);
				element_TiffData->SetAttribute("FirstZ", tiff_data.FirstZ);
				element_TiffData->SetAttribute("IFD", tiff_data.IFD);

				XMLElement* element_UUID = doc.NewElement("UUID");
				element_TiffData->InsertEndChild(element_UUID);
				element_UUID->SetAttribute("FileName", tiff_data.FileName.c_str());
			};

		if (is_ordered)
		{
			for (auto it_channel = pixels._channels.begin(); it_channel != pixels._channels.end(); it_channel++)
			{
				uint32_t c_id = it_channel->second._info.id;
				TiffData tiff_data;
				status = pixels.get_tiff_data(c_id, 0, 0, tiff_data);
				if (status != ErrorCode::STATUS_OK)
					return status;

				insertTiffDataFunc(tiff_data);
			}
		}
		else
		{
			for (auto it_tiff_data = pixels._tiff_datas.begin(); it_tiff_data != pixels._tiff_datas.end(); it_tiff_data++)
			{
				insertTiffDataFunc(it_tiff_data->second);
			}
		}
	}

	XMLPrinter printer;
	doc.Print(&printer);
	//int dataSize = printer.CStrSize();
	//*xml = (char*)calloc(dataSize, 1);
	//if (*xml == nullptr)
	//	return ErrorCode::TIFF_ERR_ALLOC_MEMORY_FAILED;
	//memcpy(*xml, printer.CStr(), dataSize);

	xml = printer.CStr();

	return ErrorCode::STATUS_OK;
}
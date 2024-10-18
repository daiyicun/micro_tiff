#include "ome_struct.h"

using namespace std;

namespace ome
{
	Well::Well()
	{
		memset(&_info, 0, sizeof(WellInfo));		
		_info.well_shape = Shape::SHAPE_UNDEFINED;
		_well_sample_array.clear();
		_well_sample_id_index = 1;
	}

	Well::~Well()
	{
		_well_sample_array.clear();
	}

	Well& Well::operator=(const Well& well)
	{
		if (this != &well)
		{
			this->_info = well._info;
			this->_well_sample_array = well._well_sample_array;
		}
		return *this;
	}

	int32_t Well::add_well_sample(uint32_t plate_id, ScanRegionInfo& scan_region_info, string& well_sample_id, string& image_ref_id)
	{
		well_sample_id = "WellSample:" + to_string(plate_id) + "." + to_string(_info.id) + "." + to_string(_well_sample_id_index);
		WellSample well_sample;
		well_sample.position_x = scan_region_info.start_physical_x;
		well_sample.position_y = scan_region_info.start_physical_y;
		well_sample.position_z = scan_region_info.start_physical_z;
		well_sample.physicalsize_unit_x = scan_region_info.start_unit_x;
		well_sample.physicalsize_unit_y = scan_region_info.start_unit_y;
		well_sample.physicalsize_unit_z = scan_region_info.start_unit_z;
		well_sample.id = well_sample_id;
		well_sample.image_ref_id = image_ref_id;
		_well_sample_array[_well_sample_id_index] = well_sample;
		_well_sample_id_index++;
		return ErrorCode::STATUS_OK;
	}

	int32_t Well::add_well_sample(ScanRegionInfo& scan_region_info, const string& well_sample_id, const string& image_ref_id)
	{
		uint32_t well_sample_pure_id = 0;
		int result_sscanf = sscanf_s(well_sample_id.c_str(), "WellSample:%*u.%*u.%u", &well_sample_pure_id);
		if (result_sscanf <= 0)
			return ErrorCode::ERR_SCANF_ASSIGNED_ERROR;

		auto it = _well_sample_array.find(well_sample_pure_id);
		if (it != _well_sample_array.end())
			return ErrorCode::ERR_WELL_SAMPLE_ID_EXIST;

		WellSample well_sample;
		well_sample.position_x = scan_region_info.start_physical_x;
		well_sample.position_y = scan_region_info.start_physical_y;
		well_sample.position_z = scan_region_info.start_physical_z;
		well_sample.physicalsize_unit_x = scan_region_info.start_unit_x;
		well_sample.physicalsize_unit_y = scan_region_info.start_unit_y;
		well_sample.physicalsize_unit_z = scan_region_info.start_unit_z;
		well_sample.id = well_sample_id;
		well_sample.image_ref_id = image_ref_id;
		_well_sample_array[well_sample_pure_id] = well_sample;
		if (_well_sample_id_index <= well_sample_pure_id)
			_well_sample_id_index = well_sample_pure_id + 1;
		return ErrorCode::STATUS_OK;
	}

	PlateAcquisition::PlateAcquisition()
	{
		_id = "";
		_well_sample_ref.clear();
	}

	PlateAcquisition::~PlateAcquisition()
	{
		_id = "";
		_well_sample_ref.clear();
	}

	PlateAcquisition& PlateAcquisition::operator=(const PlateAcquisition& plate_acquisition)
	{
		if (this != &plate_acquisition)
		{
			this->_id = plate_acquisition._id;
			this->_well_sample_ref = plate_acquisition._well_sample_ref;
		}
		return *this;
	}

	int32_t PlateAcquisition::add_well_sample_ref(uint32_t region_id, const string& well_sample_ref_id)
	{
		auto it = _well_sample_ref.find(region_id);
		if (it != _well_sample_ref.end())
			return ErrorCode::ERR_SCAN_REGION_ID_EXIST;

		_well_sample_ref[region_id] = well_sample_ref_id;
		return ErrorCode::STATUS_OK;
	}

	Plate::Plate()
	{
		memset(&_info, 0, sizeof(PlateInfo));
		_info.physicalsize_unit_x = DistanceUnit::DISTANCE_UNDEFINED;
		_info.physicalsize_unit_y = DistanceUnit::DISTANCE_UNDEFINED;
		_wells_array.clear();
		_scans_array.clear();
	}

	Plate::~Plate()
	{
		_wells_array.clear();
		_scans_array.clear();
	}

	int32_t Plate::add_well(WellInfo& info)
	{
		auto it = _wells_array.find(info.id);
		if (it != _wells_array.end())
			return ErrorCode::ERR_WELL_ID_EXIST;
		Well well;
		well._info = info;
		_wells_array[info.id] = well;
		return ErrorCode::STATUS_OK;
	}

	int32_t Plate::get_wells(WellInfo* infos)
	{
		uint32_t index = 0;
		for (auto it = _wells_array.begin(); it != _wells_array.end(); it++, index++)
		{
			infos[index] = it->second._info;
		}
		return ErrorCode::STATUS_OK;
	}

	int32_t Plate::add_scan(ScanInfo& info)
	{
		auto it = _scans_array.find(info.id);
		if (it != _scans_array.end())
			return ErrorCode::ERR_SCAN_ID_EXIST;

		Scan scan;
		scan._info = info;
		_scans_array[info.id] = scan;
		return ErrorCode::STATUS_OK;
	}

	int32_t Plate::get_scans(ScanInfo* infos)
	{
		uint32_t index = 0;
		for (auto it = _scans_array.begin(); it != _scans_array.end(); it++, index++)
		{
			infos[index] = it->second._info;
		}
		return ErrorCode::STATUS_OK;
	}

	Plate& Plate::operator=(const Plate& plate)
	{
		if (this != &plate)
		{
			this->_info = plate._info;
			this->_wells_array = plate._wells_array;
			this->_scans_array = plate._scans_array;
			this->_plate_acquisition_array = plate._plate_acquisition_array;
		}
		return *this;
	}

	Scan::Scan()
	{
		memset(&_info, 0, sizeof(ScanInfo));
		_info.pixel_physical_uint_x = DistanceUnit::DISTANCE_UNDEFINED;
		_info.pixel_physical_uint_y = DistanceUnit::DISTANCE_UNDEFINED;
		_info.pixel_physical_uint_z = DistanceUnit::DISTANCE_UNDEFINED;
		_info.time_increment_unit = TimeUnit::TIME_UNDEFINED;
		_info.pixel_type = PixelType::PIXEL_UNDEFINED;
		_channel_array.clear();
		_regions_array.clear();
	}

	Scan::~Scan()
	{
		_channel_array.clear();
		_regions_array.clear();
	}

	int32_t Scan::add_channel(ChannelInfo& info)
	{
		auto it = _channel_array.find(info.id);
		if (it != _channel_array.end())
		{
			bool check_result = false;
			if (it->second._info.sample_per_pixel == info.sample_per_pixel)
			{
				wstring str1 = wstring(it->second._info.name);
				wstring str2 = wstring(info.name);
				if (str1 == str2)
					check_result = true;
			}
			if (check_result)
				return ErrorCode::STATUS_OK;
			return ErrorCode::ERR_CHANNEL_ID_EXIST;
		}
		Channel channel;
		channel._info = info;
		_channel_array[info.id] = channel;
		return ErrorCode::STATUS_OK;
	}

	int32_t Scan::get_channels(ChannelInfo* channels)
	{
		uint32_t index = 0;
		for (auto it = _channel_array.begin(); it != _channel_array.end(); it++, index++)
		{
			channels[index] = it->second._info;
		}
		return ErrorCode::STATUS_OK;
	}

	int32_t Scan::remove_channel(uint32_t channel_id)
	{
		auto it = _channel_array.find(channel_id);
		if (it != _channel_array.end())
			_channel_array.erase(it);
		return ErrorCode::STATUS_OK;
	}

	int32_t Scan::add_region(ScanRegionInfo& info)
	{
		auto it = _regions_array.find(info.id);
		if (it != _regions_array.end())
			return ErrorCode::ERR_SCAN_REGION_ID_EXIST;
		ScanRegion region;
		region._info = info;
		_regions_array[info.id] = region;
		return ErrorCode::STATUS_OK;
	}

	int32_t Scan::get_region(uint32_t region_id, ScanRegionInfo& info)
	{
		auto it = _regions_array.find(region_id);
		if (it == _regions_array.end())
			return ErrorCode::ERR_WELL_NOT_EXIST;

		info = it->second._info;
		return ErrorCode::STATUS_OK;
	}

	Scan& Scan::operator=(const Scan& scan)
	{
		if (this != &scan)
		{
			this->_info = scan._info;
			this->_channel_array = scan._channel_array;
			this->_regions_array = scan._regions_array;
		}
		return *this;
	}

	ScanRegion::ScanRegion()
	{
		memset(&_info, 0, sizeof(ScanRegionInfo));
	}

	ScanRegion::~ScanRegion()
	{
		memset(&_info, 0, sizeof(ScanRegionInfo));
	}

	ScanRegion& ScanRegion::operator=(const ScanRegion& region)
	{
		if (this != &region)
		{
			this->_info = region._info;
		}
		return *this;
	}

	Pixels::Pixels()
	{
		memset(&_info, 0, sizeof(PixelsInfo));
		_tiff_datas.clear();
		_channels.clear();
		_t_max = 0;
	}

	int32_t Pixels::add_channel(ChannelInfo& info)
	{
		auto it = _channels.find(info.id);
		if (it != _channels.end())
		{
			bool check_result = false;
			if (it->second._info.sample_per_pixel == info.sample_per_pixel)
			{
				wstring str1 = wstring(it->second._info.name);
				wstring str2 = wstring(info.name);
				if (str1 == str2)
					check_result = true;
			}
			if (check_result)
				return ErrorCode::STATUS_OK;
			return ErrorCode::ERR_CHANNEL_ID_EXIST;
		}
		Channel channel;
		channel._info = info;
		_channels[info.id] = channel;
		return ErrorCode::STATUS_OK;
	}

	int32_t Pixels::remove_channel(uint32_t channel_id)
	{
		auto it = _channels.find(channel_id);
		if (it != _channels.end())
			_channels.erase(it);
		return ErrorCode::STATUS_OK;
	}

	int32_t Pixels::add_tiff_data(TiffData& tiff_data)
	{
		uint64_t key = ((uint64_t)tiff_data.FirstC << 48) + ((uint64_t)tiff_data.FirstT << 16) + (uint64_t)tiff_data.FirstZ;
		auto it = _tiff_datas.find(key);
		if (it != _tiff_datas.end())
			return ErrorCode::ERR_ADD_TIFF_DATA_REPEATE;

		_tiff_datas[key] = tiff_data;
		if (tiff_data.FirstT > _t_max)
			_t_max = tiff_data.FirstT;

		return ErrorCode::STATUS_OK;
	}

	int32_t Pixels::get_tiff_data(uint32_t c, uint32_t z, uint32_t t, TiffData& tiff_data)
	{
		uint64_t key = ((uint64_t)c << 48) + ((uint64_t)t << 16) + (uint64_t)z;
		auto it = _tiff_datas.find(key);
		if (it == _tiff_datas.end())
			return ErrorCode::ERR_CANNOT_FIND_TIFF_DATA_WITH_ZTC;

		tiff_data = it->second;
		return ErrorCode::STATUS_OK;
	}

	uint32_t Pixels::get_t_size() const
	{
		return _t_max + 1;
	}

	Pixels& Pixels::operator=(const Pixels& pixels)
	{
		if (this != &pixels)
		{
			this->_info = pixels._info;
			this->_channels = pixels._channels;
			this->_tiff_datas = pixels._tiff_datas;
			this->_t_max = pixels._t_max;
			this->_info.id = pixels._info.id;
			this->_info.dimension_order = pixels._info.dimension_order;
		}
		return *this;
	}

	Image& Image::operator=(const Image& image)
	{
		if (this != &image)
		{
			this->_id = image._id;
			this->_name = image._name;
			this->_pixels = image._pixels;
		}
		return *this;
	}

	Channel& Channel::operator=(const Channel& channel)
	{
		if (this != &channel)
		{
			this->_info = channel._info;
		}
		return *this;
	}
}

#pragma once
#include "ome_def.h"
#include <string>
#include <map>

namespace ome
{
	struct WellSample {
		float position_x;
		float position_y;
		float position_z;
		DistanceUnit physicalsize_unit_x;
		DistanceUnit physicalsize_unit_y;
		DistanceUnit physicalsize_unit_z;
		std::string id;
		std::string image_ref_id;
	};

	class Well
	{
	public:
		WellInfo _info;
		//key "WellSample ID"
		std::map<uint32_t, WellSample> _well_sample_array;

		Well();
		~Well();
		Well& operator=(const Well& well);

		int32_t add_well_sample(uint32_t plate_id, ScanRegionInfo& scan_region_info, std::string& well_sample_id, std::string& image_ref_id);
		int32_t add_well_sample(ScanRegionInfo& scan_region_info, const std::string& well_sample_id, const std::string& image_ref_id);

	private:
		uint32_t _well_sample_id_index;
	};

	class PlateAcquisition
	{
	public:
		std::string _id;
		//<WellSampleRef ID="WellSample:1.1.1" RegionID="0" / >
		//key "RegionID", value "ID"
		std::map<uint32_t, std::string> _well_sample_ref;

		PlateAcquisition();
		~PlateAcquisition();
		PlateAcquisition& operator=(const PlateAcquisition& plate_acquisition);

		int32_t add_well_sample_ref(uint32_t region_id, const std::string& well_sample_ref_id);
	private:
	};

	class Channel
	{
	public:
		ChannelInfo _info;
		Channel& operator=(const Channel& channel);
	};

	class ScanRegion
	{
	public:
		ScanRegionInfo _info;

		ScanRegion();
		~ScanRegion();
		ScanRegion& operator=(const ScanRegion& region);

	private:
	
	};

	class Scan
	{
	public:
		ScanInfo _info;
		//key: channel id
		std::map<uint32_t, Channel> _channel_array;

		Scan();
		~Scan();
		Scan& operator=(const Scan& scan);

		int32_t add_channel(ChannelInfo& info);
		int32_t get_channels(ChannelInfo* channels);
		int32_t remove_channel(uint32_t channel_id);

		int32_t add_region(ScanRegionInfo& info);
		int32_t get_region(uint32_t region_id, ScanRegionInfo& info);

	private:
		//key: region id
		std::map<uint32_t, ScanRegion> _regions_array;
	};

	class Plate
	{
	public:
		PlateInfo  _info;
		//key: scan_id
		std::map<uint32_t, PlateAcquisition> _plate_acquisition_array;
		//key: well id
		std::map<uint32_t, Well> _wells_array;
		//key: scan_id
		std::map<uint32_t, Scan> _scans_array;

		Plate();
		~Plate();
		Plate& operator=(const Plate& plate);

		int32_t add_well(WellInfo& info);
		int32_t get_wells(WellInfo* infos);

		int32_t add_scan(ScanInfo& info);
		int32_t get_scans(ScanInfo* infos);
	private:
	};

	struct PixelsInfo
	{
		float physical_size_per_pixel_x;
		float physical_size_per_pixel_y;
		float physical_size_per_pixel_z;
		float time_increment;

		ome::DistanceUnit physical_x_unit;
		ome::DistanceUnit physical_y_unit;
		ome::DistanceUnit physical_z_unit;
		ome::TimeUnit time_increment_unit;

		ome::PixelType pixel_type;
		uint32_t significant_bits;

		uint32_t size_x;
		uint32_t size_y;
		uint32_t size_z;
		uint32_t size_t;

		uint32_t tile_pixel_width;
		uint32_t tile_pixel_height;

		std::string id;
		std::string dimension_order;
	};

	struct TiffData
	{
		uint32_t FirstC;
		uint32_t FirstT;
		uint32_t FirstZ;
		uint32_t IFD;
		std::string FileName;
	};

	class Pixels
	{
	public:
		PixelsInfo _info;
		std::map<uint32_t, Channel> _channels;
		//<TiffData FirstC="0" FirstT="0" FirstZ="0" FirstS="0" IFD="0">
		//	<UUID FileName = "xxx.tif" / >
		//</TiffData>
		//key "FirstC<<48, FirstT<<16, FirstZ"
		//value TiffData
		std::map<uint64_t, TiffData> _tiff_datas;

		Pixels();
		Pixels& operator=(const Pixels& pixels);

		int32_t add_channel(ChannelInfo& info);
		int32_t remove_channel(uint32_t channel_id);

		int32_t add_tiff_data(TiffData& tiff_data);
		int32_t get_tiff_data(uint32_t c, uint32_t z, uint32_t t, TiffData& tiff_data);

		uint32_t get_t_size() const;

	private:
		uint32_t _t_max;
	};

	class Image
	{
	public:
		std::string _id;
		std::string _name;
		Pixels _pixels;
		Image& operator=(const Image& image);
	};
}

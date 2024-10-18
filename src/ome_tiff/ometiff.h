#pragma once
#include "ome_struct.h"
#include "ometiff_container.h"
#include <mutex>

#ifndef HEADERIMAGESIZE
#define HEADERIMAGESIZE 64
#endif // !HEADERIMAGESIZE

class OmeTiff
{
public:
	OmeTiff(void);
	~OmeTiff(void);

	int32_t Init(const wchar_t* file_name, ome::OpenMode mode, ome::CompressionMode cm);

	int32_t SaveTileData(ome::FrameInfo frame, uint32_t row, uint32_t column, void* image_data, uint32_t stride);
	int32_t PurgeFrame(ome::FrameInfo frame);

	int32_t LoadRawData(ome::FrameInfo frame, ome::OmeSize dst_size, ome::OmeRect src_rect, void* image_data, uint32_t stride);
	int32_t LoadRawData(ome::FrameInfo frame, uint32_t row, uint32_t column, void* image_data, uint32_t stride);

	int32_t AddPlate(ome::PlateInfo& plate_info);
	int32_t GetPlates(ome::PlateInfo* plates_info);
	int32_t GetPlatesSize();

	int32_t AddWell(uint32_t plate_id, ome::WellInfo& well_info);
	int32_t GetWells(uint32_t plate_id, ome::WellInfo* wells_info);
	int32_t GetWellsSize(uint32_t plate_id);

	int32_t AddScan(uint32_t plate_id, ome::ScanInfo& scan_info);
	int32_t GetScans(uint32_t plate_id, ome::ScanInfo* scans_info);
	int32_t GetScansSize(uint32_t plate_id);

	int32_t AddChannel(uint32_t plate_id, uint32_t scan_id, ome::ChannelInfo& channel_info);
	int32_t GetChannels(uint32_t plate_id, uint32_t scan_id, ome::ChannelInfo* channel_info);
	int32_t GetChannelsSize(uint32_t plate_id, uint32_t scan_id);

	int32_t RemoveChannel(uint32_t plate_id, uint32_t scan_id, uint32_t channel_id);

	int32_t AddScanRegion(uint32_t plate_id, uint32_t scan_id, uint32_t well_id, ome::ScanRegionInfo& scan_region_info, 
						  const std::string& well_sample_id = "", const std::string& image_ref_id = "");
	int32_t GetScanRegions(uint32_t plate_id, uint32_t scan_id, uint32_t well_id, ome::ScanRegionInfo* scan_regions_info);
	int32_t GetScanRegionsSize(uint32_t plate_id, uint32_t scan_id, uint32_t well_id);

	int32_t SetTag(ome::FrameInfo frame, uint16_t tag_id, uint16_t tag_type, uint32_t tag_count, void* tag_value);
	int32_t GetTag(ome::FrameInfo frame, uint16_t tag_id, uint32_t& tag_size, void* tag_value);

	int32_t CreateOMEHeader();

	std::wstring GetFileFullName() const { return _tiff_file_full_name; }
	std::string GetUTF8FileName() const;
	std::string GetFullPathWithFileName(std::string& utf8_file_name);
	ome::OpenMode GetOpenMode() const { return _open_mode; }

	std::map<uint32_t, ome::Image*> _images;
	std::map<uint32_t, ome::Plate*> _plates;

private:
	ome::CompressionMode _compression_mode;
	ome::OpenMode _open_mode;

	std::mutex _mutex_raw;
	std::map<std::string, TiffContainer*> _raw_file_containers;

	std::wstring _tiff_file_full_name;
	std::wstring _tiff_file_dir;
	std::wstring _tiff_file_name;
	std::string _tiff_header_ext;

	bool _is_in_parsing;

	int32_t GetRawContainer(ome::FrameInfo frame, TiffContainer** container, uint32_t& ifd_no, bool is_read);
};


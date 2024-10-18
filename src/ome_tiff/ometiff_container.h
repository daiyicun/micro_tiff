#pragma once
#include "ome_struct.h"
#include "..\micro_tiff\micro_tiff.h"

class TiffContainer
{
private:
	int32_t _hdl;
	ome::OpenMode _open_mode;
	uint32_t _bin_size;

	std::wstring _file_full_path;
	std::string _utf8_short_name;

	//int32_t SaveTileJpeg(void* image_data, uint32_t ifd_no, uint32_t block_no, uint64_t block_size, int32_t image_width, int32_t image_height);
	int32_t SaveTileLZW(void* image_data, uint32_t ifd_no, uint32_t block_no, uint64_t block_size);
	//int32_t SaveTileLZ4(void* image_data, uint32_t ifd_no, uint32_t block_no, uint64_t block_size);
	//int32_t SaveTileZlib(void* image_data, uint32_t ifd_no, uint32_t block_no, uint64_t block_size);

	int32_t GetOneBlockData(uint32_t ifd_no, ome::OmeRect rect, const ImageInfo& image_info, void* image_data, uint32_t stride, ome::OmeSize copy_start);

	int32_t GetRectData(uint32_t ifd_no, ome::OmeRect rect, void* image_data, uint32_t stride);
	int32_t SaveTileData(uint32_t ifd_no, ome::OmeRect rect, const ImageInfo& image_info, void* image_data, uint32_t stride);

public:
	TiffContainer();
	~TiffContainer();

	int32_t Init(ome::OpenMode open_mode, const std::wstring& file_full_path, uint32_t bin_size);

	std::string GetFileName() const { return _utf8_short_name; }
	bool IsAvailable() const { return _hdl >= 0; }

	int32_t CloseFile();
	int32_t RemoveFile();

	int32_t SaveTileData(uint32_t ifd_no, uint32_t row, uint32_t column, void* image_data, uint32_t stride);

	int32_t LoadRectData(uint32_t ifd_no, ome::OmeSize dst_size, ome::OmeRect src_rect, void* image_data, uint32_t stride);
	int32_t LoadTileData(uint32_t ifd_no, uint32_t row, uint32_t column, void* image_data, uint32_t stride);

	int32_t CreateIFD(uint32_t width, uint32_t height, uint32_t block_width, uint32_t block_height,
		ome::PixelType pixel_type, uint16_t samples_per_pixel, ome::CompressionMode compress_mode);
	int32_t CloseIFD(uint32_t ifd_no);

	int32_t SetTag(uint32_t ifd_no, uint16_t tag_id, uint16_t tag_type, uint32_t tag_count, void* tag_value);
	int32_t GetTag(uint32_t ifd_no, uint16_t tag_id, uint32_t& tag_size, void* tag_value);
};
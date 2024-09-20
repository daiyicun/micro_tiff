#pragma once
#include "ome_struct.h"
#include "..\micro_tiff\micro_tiff.h"

struct ContainerInfo
{
	uint32_t tile_pixel_width;
	uint32_t tile_pixel_height;

	uint32_t samples_per_pixel;

	ome::PixelType pixel_type;

	ome::OpenMode open_mode;
	ome::CompressionMode compress_mode;
};

class TiffContainer
{
private:
	int32_t _hdl;
	ContainerInfo _info;
	uint8_t _image_bytes;

	std::wstring _file_full_path;
	std::string _utf8_short_name;

	uint32_t GetBlockId(uint32_t x, uint32_t y, uint32_t block_width, uint32_t block_height, uint32_t image_width);

	//int32_t SaveTileJpeg(void* image_data, uint32_t ifd_no, uint32_t block_no, uint64_t block_size, int32_t image_width, int32_t image_height);
	int32_t SaveTileLZW(void* image_data, uint32_t ifd_no, uint32_t block_no, uint64_t block_size);
	//int32_t SaveTileLZ4(void* image_data, uint32_t ifd_no, uint32_t block_no, uint64_t block_size);
	//int32_t SaveTileZlib(void* image_data, uint32_t ifd_no, uint32_t block_no, uint64_t block_size);
	int32_t SaveTileRaw(void* image_data, uint32_t ifd_no, uint32_t block_no, uint64_t block_size);

	//long TransformFloatToU8(void* buf, void* image, uint32_t ifd_no, ome::OmeRect* rect, uint32_t block_width);

	int32_t GetOneBlockData(uint32_t ifd_no, ome::OmeRect* rect, ImageInfo* imageInfo, void* image, uint32_t stride);
	int32_t GetMultiBlockData(uint32_t ifd_no, ome::OmeRect* rect, ImageInfo* imageInfo, void* image, uint32_t stride);

	int32_t GetRectData(uint32_t ifd_no, ome::OmeRect* rect, void* image, uint32_t stride);
public:
	TiffContainer();
	~TiffContainer();

	int32_t Init(ContainerInfo info, std::wstring file_full_path);

	std::string GetFileName() { return _utf8_short_name; }
	bool IsAvailable() { return _hdl >= 0; }

	int32_t CloseFile();
	int32_t RemoveFile();

	int32_t SaveTileData(void* image_data, uint32_t stride, uint32_t ifd_no, uint32_t row, uint32_t column);
	int32_t SaveTileData(void* image_data, uint32_t stride, uint32_t ifd_no, ome::OmeSize image_size, ome::OmeRect rect);

	int32_t LoadRectData(uint32_t ifd_no, ome::OmeSize dst_size, ome::OmeRect src_rect, void* buffer, uint32_t stride);

	int32_t CreateIFD(const uint32_t width, const uint32_t height);
	int32_t CloseIFD(uint32_t ifd_no);

	int32_t SetCustomTag(uint32_t ifd_no, uint16_t tag_id, uint16_t tag_type, uint32_t tag_count, void* tag_value);
	int32_t GetCustomTag(uint32_t ifd_no, uint16_t tag_id, uint32_t tag_size, void* tag_value);
};
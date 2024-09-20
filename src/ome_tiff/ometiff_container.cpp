#include <fstream>
#include <functional>
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <omp.h>
#include "ometiff_container.h"
//#include "jpeg_handler.h"
//#include "..\lz4-1.9.2\lz4.h"
#include "..\lzw\lzw.h"
//#include "..\p2d\p2d_lib.h"
//#include "..\p2d\img.h"
//#include "..\p2d\p2d_basic.h"
//#include "..\zlib-1.2.11\zlib.h"

using namespace std;
using namespace ome;

namespace fs = filesystem;

inline static uint8_t GetBits(PixelType type) {
	switch (type)
	{
	case PixelType::PIXEL_UINT8:
	case PixelType::PIXEL_INT8:
		return 8;
	case PixelType::PIXEL_INT16:
	case PixelType::PIXEL_UINT16:
		return 16;
	case PixelType::PIXEL_FLOAT32:
		return 32;
	}
	return 8;
}

inline static uint8_t GetBytes(PixelType type) { return GetBits(type) / 8; }

static int32_t convert_tileIndex_to_rect(const uint32_t row, const uint32_t column, const OmeSize full_size, const OmeSize tile_size, OmeRect& rect)
{
	uint32_t multiHeight = row * tile_size.height;
	uint32_t multiWidth = column * tile_size.width;
	if (full_size.height <= multiHeight)
		return ErrorCode::ERR_COLUMN_OUT_OF_RANGE;
	if (full_size.width <= multiWidth)
		return ErrorCode::ERR_ROW_OUT_OF_RANGE;

	uint32_t tempHeight = full_size.height - multiHeight;
	uint32_t tempWidth = full_size.width - multiWidth;

	rect.x = multiWidth;
	rect.y = multiHeight;
	rect.width = tempWidth < tile_size.width ? tempWidth : tile_size.width;
	rect.height = tempHeight < tile_size.height ? tempHeight : tile_size.height;
	return ErrorCode::STATUS_OK;
}

TiffContainer::TiffContainer()
{
	_hdl = -1;
	_info = { 0 };
	_info.open_mode = OpenMode::READ_ONLY_MODE;
	_info.pixel_type = PixelType::PIXEL_UNDEFINED;
	_image_bytes = 1;
	_file_full_path = L"";
	_utf8_short_name = "";
}

TiffContainer::~TiffContainer()
{
	if (_hdl < 0)
		return;
	CloseFile();
}

int32_t TiffContainer::Init(ContainerInfo info, wstring file_full_path)
{
	_info = info;
	_image_bytes = GetBytes(info.pixel_type);
	_file_full_path = file_full_path;
	int32_t hdl = micro_tiff_Open(_file_full_path.c_str(), ((info.open_mode != OpenMode::READ_ONLY_MODE) * OPENFLAG_WRITE) | ((info.open_mode == OpenMode::CREATE_MODE) * OPENFLAG_CREATE) | OPENFLAG_BIGTIFF);
	if (hdl < 0)
		return hdl;

	_hdl = hdl;

	fs::path p{ _file_full_path };
	string str_utf8 = p.u8string();
	_utf8_short_name = p.filename().u8string().c_str();

	return ErrorCode::STATUS_OK;
}

int32_t TiffContainer::CloseFile()
{
	if (_info.open_mode != OpenMode::READ_ONLY_MODE)
	{
		int32_t ifd_size = micro_tiff_GetIFDSize(_hdl);
		for (int32_t i = 0; i < ifd_size; i++)
		{
			micro_tiff_CloseIFD(_hdl, i);
		}
	}
	int32_t status = micro_tiff_Close(_hdl);
	_hdl = -1;
	return status;
}

int32_t TiffContainer::SaveTileData(void* image_data, uint32_t stride, uint32_t ifd_no, uint32_t row, uint32_t column)
{
	ImageInfo info = { 0 };
	int32_t status = micro_tiff_GetImageInfo(_hdl, ifd_no, info);
	if (status != ErrorCode::STATUS_OK)
		return status;

	if (info.block_width != _info.tile_pixel_width || info.block_height != _info.tile_pixel_height)
		return ErrorCode::ERR_BLOCK_SIZE_NOT_MATCHED;

	OmeSize image_size = { info.image_width, info.image_height };
	OmeSize tile_size = { info.block_width, info.block_height };
	OmeRect rect = { 0 };
	status = convert_tileIndex_to_rect(row, column, image_size, tile_size, rect);
	if (status != ErrorCode::STATUS_OK)
		return status;

	return SaveTileData(image_data, stride, ifd_no, image_size, rect);
}

static int32_t BufferCopy(void* src_buffer, uint32_t src_stride, void* dst_buffer, uint32_t dst_stride, uint32_t bytes_per_pixel, OmeRect src_copy_rect, OmeSize dst_paste_position)
{
	uint32_t src_copy_stride = src_copy_rect.width * bytes_per_pixel;
	for (uint32_t h = 0; h < src_copy_rect.height; h++)
	{
		uint8_t* src_ptr = (uint8_t*)src_buffer + (h + src_copy_rect.y) * src_stride + src_copy_rect.x * bytes_per_pixel;
		uint8_t* dst_ptr = (uint8_t*)dst_buffer + (h + dst_paste_position.height) * dst_stride + dst_paste_position.width * bytes_per_pixel;
		memcpy_s(dst_ptr, src_copy_stride, src_ptr, src_copy_stride);
	}

	return ErrorCode::STATUS_OK;
}

int32_t TiffContainer::SaveTileData(void* image_data, uint32_t stride, uint32_t ifd_no, OmeSize image_size, OmeRect rect)
{
	uint32_t tile_width = _info.tile_pixel_width;
	uint32_t tile_height = _info.tile_pixel_height;

	uint32_t bytes_per_pixel = _image_bytes * _info.samples_per_pixel;

	uint32_t block_no = GetBlockId(rect.x, rect.y, tile_width, tile_height, image_size.width);
	uint32_t buf_width = stride == 0 ? rect.width : stride / bytes_per_pixel;

	uint64_t block_byte_size = rect.height * tile_width * bytes_per_pixel;
	//when rect.width != tileWidth, expand to a whole tile width

	void* buf = nullptr;
	unique_ptr<uint8_t> auto_buf(new uint8_t[0]);
	if (buf_width != tile_width)
	{
		auto_buf.reset(new uint8_t[block_byte_size]);
		buf = auto_buf.get();
		if (buf == nullptr)
			return ErrorCode::ERR_BUFFER_IS_NULL;
		uint32_t src_stride = stride == 0 ? buf_width * bytes_per_pixel : stride;
		OmeRect copy_rect = { 0 };
		copy_rect.x = 0;
		copy_rect.y = 0;
		copy_rect.width = buf_width;
		copy_rect.height = rect.height;
		OmeSize dst_pos = { 0 };
		dst_pos.width = 0;
		dst_pos.height = 0;
		BufferCopy(image_data, src_stride, buf, tile_width * bytes_per_pixel, bytes_per_pixel, copy_rect, dst_pos);
	}
	else
	{
		buf = image_data;
	}

	//OmeRect mm_rect = { rect.x, rect.y, (int32_t)buf_width, rect.height };
	//SaveMinMax(ifd_no, image_data, mm_rect);

	int32_t status = ErrorCode::ERR_COMPRESS_TYPE_NOTSUPPORT;
	switch (_info.compress_mode)
	{
	case CompressionMode::COMPRESSIONMODE_NONE:
		status = SaveTileRaw(buf, ifd_no, block_no, block_byte_size);
		break;
	case CompressionMode::COMPRESSIONMODE_LZW:
		status = SaveTileLZW(buf, ifd_no, block_no, block_byte_size);
		break;
	//case COMPRESSION_LZ4:
	//	status = SaveTileLZ4(buf, ifd_no, block_no, block_byte_size);
	//	break;
	//case COMPRESSION_DEFLATE:
	//	status = SaveTileZlib(buf, ifd_no, block_no, block_byte_size);
	//	break;
	//case COMPRESSION_JPEG:
	//	//for saving not uint8_t data, shift to uint8_t
	//	void* shift_buf = nullptr;
	//	if (_info.pixel_type == PixelType::PIXEL_UINT16)
	//	{
	//		shift_buf = calloc((size_t)tile_width * rect.height, 1);
	//		if (shift_buf == nullptr) {
	//			status = ErrorCode::ERR_BUFFER_IS_NULL;
	//			break;
	//		}
	//		uint32_t source_stride = stride == 0 ? tile_width * 2 : stride;
	//		p2d_region region = { (int32_t)tile_width, rect.height };
	//		int32_t p2d_status = p2d_img_shift(buf, source_stride, _info.significant_bits, shift_buf, tile_width, 8, region);
	//		if (p2d_status != ErrorCode::STATUS_OK)
	//		{
	//			free(shift_buf);
	//			status = ErrorCode::ERR_P2D_SHIFT_FAILED;
	//			break;
	//		}
	//	}
	//	else
	//	{
	//		shift_buf = buf;
	//	}
	//	status = SaveTileJpeg(shift_buf, ifd_no, block_no, block_byte_size, tile_width, rect.height);
	//	if (shift_buf != buf)
	//		free(shift_buf);
	//	break;
	}

	return status;
}

//in previous version, functions "LoadPyramidalRectData", "LoadRawRectData", "LoadScaledRawRectData" have same code
//in this version, these 3 functions are combined
int32_t TiffContainer::LoadRectData(uint32_t ifd_no, OmeSize dst_size, OmeRect src_rect, void* buffer, uint32_t stride)
{
	uint32_t bytes_per_pixel = _image_bytes * _info.samples_per_pixel;

	if (stride != 0 && (int32_t)(stride / bytes_per_pixel) < dst_size.width)
		return ErrorCode::ERR_STRIDE_NOT_CORRECT;

	if (src_rect.width == dst_size.width && src_rect.height == dst_size.height)
	{
		return GetRectData(ifd_no, &src_rect, buffer, stride);
	}
	else
	{
		//size_t buf_size = (size_t)src_rect.height * src_rect.width;
		//uint32_t src_stride = src_rect.width * _image_bytes;
		//void* imageBuf = calloc((size_t)src_rect.height * src_rect.width, _image_bytes);
		//if (imageBuf == nullptr)
		//	return ErrorCode::ERR_BUFFER_IS_NULL;
		//int32_t status = GetRectData(ifd_no, &src_rect, imageBuf, src_stride);
		//if (status != ErrorCode::STATUS_OK)
		//{
		//	free(imageBuf);
		//	return status;
		//}

		//p2d_region srcRegion = { src_rect.width, src_rect.height };
		//p2d_region dstRegion = { dst_size.width, dst_size.height };
		//uint32_t dst_stride = stride == 0 ? dst_size.width * _image_bytes : stride;
		//int32_t p2d_status = p2d_img_resize(imageBuf, srcRegion, src_stride, buffer, dstRegion, dst_stride, (p2d_data_format)(_info.pixel_type));
		//free(imageBuf);
		//if (p2d_status != ErrorCode::STATUS_OK)
		//	return ErrorCode::ERR_P2D_RESIZE_FAILED;
		return ErrorCode::ERR_RESIZE_NOT_SUPPORT_NOW;
	}
	return ErrorCode::STATUS_OK;
}

int32_t DecompressLZWData(void* encode_data, uint64_t encode_size, void* decode_data, uint64_t* decode_size, uint64_t max_decode_size);
//int32_t DecompressLZ4Data(void* encode_data, uint64_t encode_size, void* decode_data, uint64_t* decode_size, uint64_t max_decode_size);
//int32_t DecompressJPEGData(void* encode_data, uint64_t encode_size, void* decode_data, uint64_t* decode_size, int32_t* width);
//int32_t DecompressZlibData(void* encode_data, uint64_t encode_size, void* decode_data, uint64_t* decode_size);

int32_t TiffContainer::GetOneBlockData(uint32_t ifd_no, OmeRect* rect, ImageInfo* imageInfo, void* image, uint32_t stride)
{
	if (rect->y + rect->height > (int32_t)imageInfo->image_height || rect->x + rect->width > (int32_t)imageInfo->image_width)
		return ErrorCode::TIFF_ERR_BAD_PARAMETER_VALUE;

	uint32_t bytes_per_pixel = _image_bytes * _info.samples_per_pixel;

	uint32_t block_no = GetBlockId(rect->x, rect->y, imageInfo->block_width, imageInfo->block_height, imageInfo->image_width);

	uint32_t height = imageInfo->block_height;
	uint32_t row_tail = imageInfo->image_height % imageInfo->block_height;
	uint32_t row_count = imageInfo->image_height / imageInfo->block_height;
	if (row_tail > 0)
	{
		double count = (double)(rect->y + rect->height) / imageInfo->block_height;
		if (count > (double)row_count && count < (double)row_count + 1)
			height = row_tail;
	}

	uint32_t block_actual_byte_size = height * imageInfo->block_width * bytes_per_pixel;
	uint32_t block_full_byte_size = imageInfo->block_height * imageInfo->block_width * bytes_per_pixel;

	//in special case with old sample data, tiff data save with LZW and compressed data take more space than raw data, so malloc more buffer to hold data to prevent memory error.
	unique_ptr<uint8_t> auto_load_block_buf(new uint8_t[(size_t)(block_full_byte_size * 1.5)]);
	uint8_t* load_block_buf = auto_load_block_buf.get();
	if (load_block_buf == nullptr)
		return ErrorCode::ERR_BUFFER_IS_NULL;

	//Load compressed data
	uint64_t count;
	int32_t status = micro_tiff_LoadBlock(_hdl, ifd_no, block_no, count, load_block_buf);
	if (status != ErrorCode::STATUS_OK || count == 0)
	{
		return status;
	}

	//decompress data information
	int32_t decompress_width = imageInfo->block_width;

	uint8_t* decompress_buf = nullptr;
	unique_ptr<uint8_t> auto_decompress_buf(new uint8_t[0]);

	if (imageInfo->compression == COMPRESSION_NONE) {
		decompress_buf = load_block_buf;
	}
	else
	{
		auto_decompress_buf.reset(new uint8_t[block_full_byte_size]);
		decompress_buf = auto_decompress_buf.get();
		if (decompress_buf == nullptr)
			return ErrorCode::ERR_BUFFER_IS_NULL;

		uint64_t actual_read_size = 0;
		switch (imageInfo->compression)
		{
			//case COMPRESSION_LZ4:
			//	decompress_status = DecompressLZ4Data(load_block_buf, count, decompress_buf, &actual_read_size, block_full_byte_size);
			//	break;
		case COMPRESSION_LZW:
			status = DecompressLZWData(load_block_buf, count, decompress_buf, &actual_read_size, block_full_byte_size);
			break;
			//case COMPRESSION_JPEG:
			//	decompress_status = DecompressJPEGData(load_block_buf, count, decompress_buf, &actual_read_size, &decompress_width);
			//	break;
			//case COMPRESSION_DEFLATE:
			//case COMPRESSION_ADOBE_DEFLATE:
			//	actual_read_size = block_full_byte_size;
			//	decompress_status = DecompressZlibData(load_block_buf, count, decompress_buf, &actual_read_size);
			//	break;
		default:
			status = ErrorCode::ERR_COMPRESS_TYPE_NOTSUPPORT;
		}
	}

	if (status != ErrorCode::STATUS_OK)
		return status;

	//get target rect data from whole block data
	if (rect->width != decompress_width || rect->height != height || (stride != 0 && stride != rect->width * bytes_per_pixel))
	{
		uint16_t actual_y = rect->y % imageInfo->block_height;
		uint16_t actual_x = rect->x % imageInfo->block_width;
		uint32_t src_stride = decompress_width * bytes_per_pixel;
		uint32_t dst_stride = stride == 0 ? rect->width * bytes_per_pixel : stride;

		OmeRect copy_rect = { 0 };
		copy_rect.x = actual_x;
		copy_rect.y = actual_y;
		copy_rect.width = rect->width;
		copy_rect.height = rect->height;
		OmeSize dst_pos = { 0 };
		dst_pos.width = 0;
		dst_pos.height = 0;

		BufferCopy(decompress_buf, src_stride, image, dst_stride, bytes_per_pixel, copy_rect, dst_pos);
	}
	else
	{
		memcpy_s(image, block_actual_byte_size, decompress_buf, block_actual_byte_size);
	}

	return ErrorCode::STATUS_OK;
}

int32_t TiffContainer::GetRectData(uint32_t ifd_no, OmeRect* rect, void* image, uint32_t stride)
{
	ImageInfo imageInfo;
	int32_t result = micro_tiff_GetImageInfo(_hdl, ifd_no, imageInfo);
	if (result != ErrorCode::STATUS_OK)
	{
		return result;
	}

	//now JPEG also padded width to tilewidth
	//if (imageInfo.compression == COMPRESSION_JPEG)
	//	imageInfo.block_width = (min)(imageInfo.block_width, imageInfo.image_width);

	//Only include in one block, use GetOneBlockData to get data
	//If rect is small than one block, jump into this branch also, this situation will be handled in the implementation code.
	if (rect->x % imageInfo.block_width + rect->width <= imageInfo.block_width && rect->y % imageInfo.block_height + rect->height <= imageInfo.block_height)
		return GetOneBlockData(ifd_no, rect, &imageInfo, image, stride);

	return GetMultiBlockData(ifd_no, rect, &imageInfo, image, stride);
}

uint32_t TiffContainer::GetBlockId(uint32_t x, uint32_t y, uint32_t block_width, uint32_t block_height, uint32_t image_width)
{
	if (block_width == 0 || block_height == 0)
		return 0;
	return x / block_width + (uint32_t)(y / block_height * (ceil((float)image_width / block_width)));
}

int32_t TiffContainer::CreateIFD(const uint32_t width, const uint32_t height)
{
	ImageInfo info = {0};
	info.image_width = width;
	info.image_height = height;
	info.bits_per_sample = GetBits(_info.pixel_type);
	info.block_width = _info.tile_pixel_width;
	info.block_height = _info.tile_pixel_height;
	info.image_byte_count = _image_bytes;
	info.photometric = PHOTOMETRIC_MINISBLACK;
	info.planarconfig = PLANARCONFIG_CONTIG;
	info.predictor = PREDICTOR_NONE;
	info.samples_per_pixel = _info.samples_per_pixel;

	switch (_info.compress_mode)
	{
	case CompressionMode::COMPRESSIONMODE_NONE:
		info.compression = COMPRESSION_NONE;
		break;
	case CompressionMode::COMPRESSIONMODE_LZW:
		info.compression = COMPRESSION_LZW;
		break;
	default:
		info.compression = COMPRESSION_NONE;
		break;
	}

	if (info.compression == COMPRESSION_JPEG && info.image_byte_count != 1) {
		return ErrorCode::ERR_JEPG_ONLY_SUPPORT_8BITS;
	}

	return micro_tiff_CreateIFD(_hdl, info);
}

int32_t TiffContainer::SetCustomTag(uint32_t ifd_no, uint16_t tag_id, uint16_t tag_type, uint32_t tag_size, void* tag_value)
{
	return micro_tiff_SetTag(_hdl, ifd_no, tag_id, tag_type, tag_size, tag_value);
}

int32_t TiffContainer::GetCustomTag(uint32_t ifd_no, uint16_t tag_id, uint32_t tag_size, void* tag_value)
{
	uint16_t tag_type = 0;
	uint32_t tag_count = 0;
	int32_t status = micro_tiff_GetTagInfo(_hdl, ifd_no, tag_id, tag_type, tag_count);
	if (status != ErrorCode::STATUS_OK)
		return status;

	uint8_t type_size = 0;
	switch (tag_type)
	{
	case TagDataType::TIFF_ASCII:
	case TagDataType::TIFF_BYTE:
	case TagDataType::TIFF_SBYTE:
	case TagDataType::TIFF_UNDEFINED:
		type_size = 1;
		break;
	case TagDataType::TIFF_SHORT:
	case TagDataType::TIFF_SSHORT:
		type_size = 2;
		break;
	case TagDataType::TIFF_LONG:
	case TagDataType::TIFF_SLONG:
	case TagDataType::TIFF_FLOAT:
	case TagDataType::TIFF_IFD:
		type_size = 4;
		break;
	case TagDataType::TIFF_RATIONAL:
	case TagDataType::TIFF_SRATIONAL:
	case TagDataType::TIFF_DOUBLE:
	case TagDataType::TIFF_LONG8:
	case TagDataType::TIFF_SLONG8:
	case TagDataType::TIFF_IFD8:
		type_size = 8;
		break;
	}

	if (tag_size < (uint32_t)type_size * tag_count)
		return ErrorCode::ERR_TAG_CONDITION_NOT_MET;

	int32_t ret = micro_tiff_GetTag(_hdl, ifd_no, tag_id, tag_value);
	if (ErrorCode::STATUS_OK != ret)
		return ret;
	return ErrorCode::STATUS_OK;
}

//int32_t TiffContainer::SaveTileJpeg(void* image_data, uint32_t ifd_no, uint32_t block_no, uint64_t block_size, int32_t image_width, int32_t image_height)
//{
//	unsigned long dst_size = 0;
//	unsigned char* dst_buf = nullptr;
//
//	long compress_status = jpeg_compress((unsigned char*)image_data, &dst_buf, &dst_size, image_width, image_height);
//
//	int32_t status;
//	if (compress_status == 0)
//		status = ThortiffSaveBlock(_hdl, ifd_no, block_no, (uint64_t)dst_size, (void*)dst_buf);
//	else
//		status = ThortiffSaveBlock(_hdl, ifd_no, block_no, block_size, image_data);
//
//	jpeg_free(dst_buf);
//	if (status != ErrorCode::STATUS_OK)
//		return ErrorCode::ERR_SAVE_BLOCK_FAILED;
//
//	return ErrorCode::STATUS_OK;
//}

int32_t TiffContainer::SaveTileLZW(void* image_data, uint32_t ifd_no, uint32_t block_no, uint64_t block_size)
{
	unique_ptr<uint8_t> auto_dst_buf(new uint8_t[block_size]);
	uint8_t* dst_buf = auto_dst_buf.get();
	if (dst_buf == nullptr) {
		return ErrorCode::ERR_BUFFER_IS_NULL;
	}
	uint64_t raw_data_used_size, output_data_used_size;
	int compress_status = LZWEncode(image_data, block_size, &raw_data_used_size, dst_buf, block_size, &output_data_used_size);

	int32_t status;
	if (compress_status == 1)
		status = micro_tiff_SaveBlock(_hdl, ifd_no, block_no, output_data_used_size, dst_buf);
	else
		status = ErrorCode::ERR_COMPRESS_LZW_FAILED;

	return status;
}

//int32_t TiffContainer::SaveTileLZ4(void* image_data, uint32_t ifd_no, uint32_t block_no, uint64_t block_size)
//{
//	int int_block_size = (int)block_size;
//	int worst_size = LZ4_compressBound(int_block_size);
//	char* dst_buf = (char*)calloc(worst_size, 1);
//	if (dst_buf == nullptr) {
//		return ErrorCode::ERR_BUFFER_IS_NULL;
//	}
//	int count = LZ4_compress_default((const char*)image_data, dst_buf, int_block_size, worst_size);
//
//	int32_t status;
//	if (count > 0 && count < int_block_size)
//		status = ThortiffSaveBlock(_hdl, ifd_no, block_no, count, dst_buf);
//	else
//		status = ThortiffSaveBlock(_hdl, ifd_no, block_no, block_size, image_data);
//	free(dst_buf);
//	if (status != ErrorCode::STATUS_OK)
//		return ErrorCode::ERR_SAVE_BLOCK_FAILED;
//
//	return ErrorCode::STATUS_OK;
//}

//int32_t TiffContainer::SaveTileZlib(void* image_data, uint32_t ifd_no, uint32_t block_no, uint64_t block_size)
//{
//	unsigned long worst_size = (unsigned long)block_size;
//	worst_size = compressBound(worst_size);
//	uint8_t* dst_buf = (uint8_t*)malloc(worst_size);
//	if (dst_buf == nullptr) {
//		return ErrorCode::ERR_BUFFER_IS_NULL;
//	}
//	int compress_status = compress(dst_buf, &worst_size, (uint8_t*)image_data, (unsigned long)block_size);
//
//	int32_t status;
//	if (compress_status == Z_OK && (uint64_t)worst_size < block_size)
//		status = ThortiffSaveBlock(_hdl, ifd_no, block_no, worst_size, dst_buf);
//	else
//		status = ThortiffSaveBlock(_hdl, ifd_no, block_no, block_size, image_data);
//	free(dst_buf);
//	if (status != ErrorCode::STATUS_OK)
//		return ErrorCode::ERR_SAVE_BLOCK_FAILED;
//
//	return ErrorCode::STATUS_OK;
//}

int32_t TiffContainer::SaveTileRaw(void* image_data, uint32_t ifd_no, uint32_t block_no, uint64_t block_size)
{
	return micro_tiff_SaveBlock(_hdl, ifd_no, block_no, block_size, image_data);
}

//long TiffContainer::TransformFloatToU8(void* buf, void* image, uint32_t ifd_no, OmeRect* rect, uint32_t block_width)
//{
//	float min, max;
//	long status = GetMinMax(ifd_no, &min, &max);
//	if (status != ErrorCode::STATUS_OK)
//		return status;
//
//	uint8_t src_bytes = 4;	//type is float absolutely
//	uint8_t dst_bytes = 1;	//dest type is uint8_t absolutely
//
//	img *src = new img();
//	p2d_info src_info = { (int32_t)rect->width, (int32_t)rect->height, 1, 1, (uint32_t)block_width * src_bytes, P2D_32F, src_bytes, P2D_CHANNELS_1, buf };
//	src->init_ext(&src_info);
//
//	img *dst = new img();
//	p2d_info dst_info = { (int32_t)rect->width, (int32_t)rect->height, 1, 1, (uint32_t)rect->width * dst_bytes, P2D_8U, dst_bytes, P2D_CHANNELS_1, image };
//	dst->init_ext(&dst_info);
//
//	do {
//		if (min >= 0 && max <= 255)
//		{
//			status = img_convert(src, dst);
//			if (status != ErrorCode::STATUS_OK) {
//				break;
//			}
//		}
//		else
//		{
//			double m_value = 0;
//			double a_value = 0;
//			if (max - min < 0.001f)	//min max is the same
//			{
//				m_value = 0;
//				a_value = 255;
//			}
//			else
//			{
//				m_value = 255 / (max - min);
//				a_value = 0 - min * m_value;
//			}
//			status = img_scale(src, { 0,0 }, dst, { 0,0 }, { rect->width, rect->height }, m_value, a_value);
//			if (status != ErrorCode::STATUS_OK) {
//				break;
//			}
//		}
//	} while (false);
//
//	delete src;
//	delete dst;
//	return status;
//}

int32_t DecompressLZWData(void* encode_data, uint64_t encode_size, void* decode_data, uint64_t* decode_size, uint64_t max_decode_size)
{
	//decompress data
	*decode_size = LZWDecode(encode_data, encode_size, decode_data, max_decode_size);
	if (*decode_size > max_decode_size)
	{
		return ErrorCode::ERR_DECOMPRESS_LZW_FAILED;
	}

	return ErrorCode::STATUS_OK;
}

//int32_t DecompressLZ4Data(void* encode_data, uint64_t encode_size, void* decode_data, uint64_t* decode_size, uint64_t max_decode_size)
//{
//	//decompress data
//	*decode_size = LZ4_decompress_safe((const char*)encode_data, (char*)decode_data, (int)encode_size, (int)max_decode_size);
//	if (*decode_size > max_decode_size)
//	{
//		return ErrorCode::ERR_DECOMPRESS_LZ4_FAILED;
//	}
//
//	return ErrorCode::STATUS_OK;
//}

//int32_t DecompressJPEGData(void* encode_data, uint64_t encode_size, void* decode_data, uint64_t* decode_size, int32_t* width)
//{
//	int height = 0, samples;
//	int decompress_status = jpeg_decompress((unsigned char*)decode_data, (uint8_t*)encode_data, (unsigned long)encode_size, width, &height, &samples);
//	if (decompress_status != 0)
//	{
//		return ErrorCode::ERR_DECOMPRESS_JPEG_FAILED;
//	}
//	*decode_size = *width * height * samples;
//	return ErrorCode::STATUS_OK;
//}

//int32_t DecompressZlibData(void* encode_data, uint64_t encode_size, void* decode_data, uint64_t* decode_size)
//{
//	int decompress_status = uncompress((uint8_t*)decode_data, (unsigned long*)decode_size, (uint8_t*)encode_data, (unsigned long)encode_size);
//	if (decompress_status != Z_OK)
//	{
//		return ErrorCode::ERR_DECOMPRESS_ZLIB_FAILED;
//	}
//	return ErrorCode::STATUS_OK;
//}

int32_t TiffContainer::GetMultiBlockData(uint32_t ifd_no, OmeRect* rect, ImageInfo* imageInfo, void* image, uint32_t stride)
{
	//bool read_block_succeed = false;
	//Calculate the image size need to get.
	//Because the image can cover several block and sometimes it is not complete	
	int32_t actual_x = rect->x / imageInfo->block_width * imageInfo->block_width;
	int32_t actual_y = rect->y / imageInfo->block_height * imageInfo->block_height;

	//Calculate the offset position to the image we get
	int32_t image_x_offset = rect->x - actual_x;
	int32_t image_y_offset = rect->y - actual_y;
	int32_t actual_width = image_x_offset + rect->width;
	int32_t actual_height = image_y_offset + rect->height;

	//bytes, stride, block size
	uint32_t bytes_per_pixel = _image_bytes * _info.samples_per_pixel;
	uint32_t block_stride = imageInfo->block_width * bytes_per_pixel;
	uint32_t block_byte_size = imageInfo->block_height * block_stride;

	//block count
	uint32_t x_block_count = (uint32_t)ceil((float)actual_width / imageInfo->block_width);
	uint32_t y_block_count = (uint32_t)ceil((float)actual_height / imageInfo->block_height);
	uint32_t total_block = x_block_count * y_block_count;

	unique_ptr<uint8_t[]> auto_block_raw_buffer = make_unique<uint8_t[]>(block_byte_size);
	//in special case with old sample data, tiff data save with LZW and compressed data take more space than raw data, so alloc more buffer to hold data to prevent memory error.
	unique_ptr<uint8_t[]> auto_block_encode_buffer = make_unique<uint8_t[]>((size_t)(1.5 * block_byte_size));

	uint8_t* block_raw_buf = auto_block_raw_buffer.get();
	uint8_t* block_encode_buf = auto_block_encode_buffer.get();
	if (block_raw_buf == nullptr || block_encode_buf == nullptr)
		return ErrorCode::ERR_BUFFER_IS_NULL;

	//uint8_t* block_8bit_buf = nullptr;
	//uint16_t actual_image_byte = image_byte;
	//uint32_t actual_block_byte_size = block_byte_size;
	//PixelType actual_type = _type;
	//if (is_get_8bit_data)
	//{
	//	actual_image_byte = 1;
	//	actual_type = PixelType::PIXEL_UINT8;
	//	actual_block_byte_size = imageInfo->block_height * imageInfo->block_width;
	//	block_8bit_buf = (uint8_t*)malloc(actual_block_byte_size);
	//	if (block_8bit_buf == nullptr)
	//		return ErrorCode::ERR_BUFFER_IS_NULL;
	//}

	int32_t status = ErrorCode::STATUS_OK;

	//Get the block in image along by y-direction
	for (uint32_t block_index = 0; block_index < total_block; ++block_index)
	{
		int32_t x = (block_index % x_block_count) * imageInfo->block_width;
		int32_t y = block_index / x_block_count * imageInfo->block_height;

		uint8_t* block_decode_buf = block_raw_buf;

		int32_t block_x_end = imageInfo->block_width + x;
		int32_t block_x_start = (image_x_offset - x > 0) ? image_x_offset - x : 0;
		int32_t cpy_x_start = (image_x_offset - x > 0) ? image_x_offset - x : x;
		int32_t cpy_x_end = (min)(block_x_end, actual_width);
		int32_t cpy_x_length = cpy_x_end - cpy_x_start;

		int32_t block_y_end = imageInfo->block_height + y;
		int32_t block_y_start = (image_y_offset - y > 0) ? image_y_offset - y : 0;
		int32_t cpy_y_start = (image_y_offset - y > 0) ? image_y_offset - y : y;
		int32_t cpy_y_end = (min)(block_y_end, actual_height);
		int32_t cpy_y_length = cpy_y_end - cpy_y_start;

		uint16_t block_no = GetBlockId(x + actual_x, y + actual_y, imageInfo->block_width, imageInfo->block_height, imageInfo->image_width);

		do {
			uint64_t count;
			status = micro_tiff_LoadBlock(_hdl, ifd_no, block_no, count, block_encode_buf);
			if (status != ErrorCode::STATUS_OK)
				break;
			if (count == 0)
				continue;

			switch (imageInfo->compression)
			{
			case COMPRESSION_NONE:
				block_decode_buf = block_encode_buf;
				break;
			//case COMPRESSION_LZ4:
			case COMPRESSION_LZW:
			//case COMPRESSION_DEFLATE:
			//case COMPRESSION_ADOBE_DEFLATE:
			{
				//if (imageInfo->compression == COMPRESSION_LZ4)
				//	LZ4_decompress_safe((const char*)block_encode_buf, (char*)block_raw_used_buf, (int)count, block_byte_size);
				int32_t decode_size = LZWDecode(block_encode_buf, count, block_decode_buf, block_byte_size);
				if (decode_size != block_byte_size)
					return ErrorCode::ERR_DECOMPRESS_LZW_FAILED;
				//if (imageInfo->compression == COMPRESSION_ADOBE_DEFLATE || imageInfo->compression == COMPRESSION_DEFLATE)
				//{
				//	int decompress_status = uncompress((uint8_t*)block_raw_used_buf, (unsigned long*)&block_byte_size, (uint8_t*)block_encode_buf, (unsigned long)count);
				//	if (decompress_status != Z_OK)
				//		status = ErrorCode::ERR_DECOMPRESS_ZLIB_FAILED;
				//}

				break;
			}
			//case COMPRESSION_JPEG:
			//{
			//	//When use jpeg,it's last tile in one row is not full storage(old version)
			//	//In new version, for ImageJ can load jpeg compression data normal, also expand to a whole for last tile
			//	actual_block_width = (min)(imageInfo->image_width - rect->width - rect->x + actual_width - x, imageInfo->block_width);
			//	int width = 0, height = 0, samples;
			//	if (jpeg_decompress(block_raw_used_buf, block_encode_buf, (unsigned long)count, &width, &height, &samples) != 0) {
			//		status = ErrorCode::ERR_DECOMPRESS_JPEG_FAILED;
			//		break;
			//	}
			//	if (width == imageInfo->block_width && _image_bytes == 1)
			//		actual_block_width = imageInfo->block_width;
			//	break;
			//}
			default:
				status = ErrorCode::ERR_COMPRESS_TYPE_NOTSUPPORT;
				break;
			}

			//if (is_get_8bit_data && actual_image_byte != image_byte)
			//{
			//	if (_type == PixelType::PIXEL_FLOAT32)
			//	{
			//		OmeRect block_rect = { 0, 0, (int)actual_block_width, (int)imageInfo->block_height };
			//		TransformFloatToU8(block_raw_used_buf, block_8bit_buf, ifd_no, &block_rect, imageInfo->block_width);
			//	}
			//	else if (_type == PixelType::PIXEL_UINT16)
			//	{
			//		p2d_region region = { (int)imageInfo->block_width ,(int)imageInfo->block_height };
			//		long p2d_status = p2d_img_shift(block_raw_used_buf, imageInfo->block_width * 2, _parentScan->significant_bits, block_8bit_buf, imageInfo->block_width, 8, region);
			//		if (p2d_status != ErrorCode::STATUS_OK) {
			//			status = ErrorCode::ERR_P2D_SHIFT_FAILED;
			//			break;
			//		}
			//	}
			//	else {
			//		status = ErrorCode::ERR_DATA_TYPE_NOTSUPPORT;
			//		break;
			//	}
			//	block_raw_used_buf = (uint8_t*)block_8bit_buf;
			//}

			//Copy the data from the block to image
			int32_t actual_copy_x = x - image_x_offset > 0 ? x - image_x_offset : 0;
			int32_t actual_copy_y = y - image_y_offset > 0 ? y - image_y_offset : 0;
			//p2d_point src_point = { block_x_start, block_y_start };
			//p2d_region src_size = { (int32_t)actual_block_width, (int32_t)(block_y_start + cpy_y_length) };
			//p2d_point dst_point = { actual_copy_x, actual_copy_y };
			//p2d_region dst_size = { (int32_t)rect->width, (int32_t)rect->height };
			//p2d_region copy_region = { cpy_x_length , cpy_y_length };
			//int32_t p2d_status = p2d_img_copy(block_decode_buf, src_point, actual_block_width * _image_bytes, src_size, image, dst_point, actual_rect_stride, dst_size, (p2d_data_format)_info.pixel_type, copy_region);
			//if (p2d_status != ErrorCode::STATUS_OK)
			//{
			//	status = ErrorCode::ERR_P2D_COPY_FAILED;
			//	break;
			//}

			uint32_t src_stride = block_stride;
			uint32_t dst_stride = stride == 0 ? rect->width * bytes_per_pixel : stride;

			OmeRect copy_rect = { 0 };
			copy_rect.x = block_x_start;
			copy_rect.y = block_y_start;
			copy_rect.width = cpy_x_length;
			copy_rect.height = cpy_y_length;
			OmeSize dst_pos = { 0 };
			dst_pos.width = actual_copy_x;
			dst_pos.height = actual_copy_y;

			BufferCopy(block_decode_buf, src_stride, image, dst_stride, bytes_per_pixel, copy_rect, dst_pos);

		} while (0);
		if (status != ErrorCode::STATUS_OK)
		{
			//free(block_8bit_buf);
			return status;
		}
		//read_block_succeed = true;
	}
	//free(block_8bit_buf);
	//if (read_block_succeed)
	//	return ErrorCode::STATUS_OK;
	return status;
}

int32_t TiffContainer::CloseIFD(uint32_t ifd_no)
{
	if (_hdl < 0)
		return ErrorCode::STATUS_OK;
	return micro_tiff_CloseIFD(_hdl, ifd_no);
}

int32_t TiffContainer::RemoveFile()
{
	if (_hdl >= 0)
		micro_tiff_Close(_hdl);
	_hdl = -1;
	_wremove(_file_full_path.c_str());
	return ErrorCode::STATUS_OK;
}
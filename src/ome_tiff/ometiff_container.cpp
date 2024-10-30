#include <fstream>
#include <filesystem>
#include <sstream>
#include "ometiff_container.h"
//#include "jpeg_handler.h"
//#include "..\lz4-1.9.2\lz4.h"
#include "../lzw/lzw.h"
#include "../common/data_predict.h"
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
	case PixelType::PIXEL_UNDEFINED:
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
	uint32_t multi_height = row * tile_size.height;
	uint32_t multi_width = column * tile_size.width;
	if (full_size.height <= multi_height)
		return ErrorCode::ERR_COLUMN_OUT_OF_RANGE;
	if (full_size.width <= multi_width)
		return ErrorCode::ERR_ROW_OUT_OF_RANGE;

	uint32_t temp_height = full_size.height - multi_height;
	uint32_t temp_width = full_size.width - multi_width;

	rect.x = multi_width;
	rect.y = multi_height;
	rect.width = temp_width < tile_size.width ? temp_width : tile_size.width;
	rect.height = temp_height < tile_size.height ? temp_height : tile_size.height;
	return ErrorCode::STATUS_OK;
}

static uint32_t GetBlockId(const uint32_t x, const uint32_t y, const uint32_t block_width, const uint32_t block_height, const uint32_t image_width)
{
	if (block_width == 0 || block_height == 0)
		return 0;
	return x / block_width + (uint32_t)(y / block_height * (ceil((float)image_width / block_width)));
}

TiffContainer::TiffContainer()
{
	_hdl = -1;
	_open_mode = OpenMode::READ_ONLY_MODE;
	_bin_size = 1;
	_file_full_path = L"";
	_utf8_short_name = "";
}

TiffContainer::~TiffContainer()
{
	if (_hdl < 0)
		return;
	CloseFile();
}

int32_t TiffContainer::Init(const ome::OpenMode open_mode, const wstring& file_full_path, const uint32_t bin_size)
{
	uint8_t open_flag = OPENFLAG_READ;
	switch (open_mode)
	{
	case ome::OpenMode::CREATE_MODE:
		open_flag |= OPENFLAG_CREATE | OPENFLAG_WRITE | OPENFLAG_BIGTIFF;
		break;
	case ome::OpenMode::READ_WRITE_MODE:
		open_flag |= OPENFLAG_WRITE;
		break;
	case ome::OpenMode::READ_ONLY_MODE:
		break;
	default:
		break;
	}
	int32_t hdl = micro_tiff_Open(file_full_path.c_str(), open_flag);
	if (hdl < 0)
		return hdl;

	_hdl = hdl;
	_file_full_path = file_full_path;
	_open_mode = open_mode;
	_bin_size = bin_size;

	fs::path p{ file_full_path };
	_utf8_short_name = p.filename().u8string();

	return ErrorCode::STATUS_OK;
}

int32_t TiffContainer::CloseFile()
{
	if (_open_mode != OpenMode::READ_ONLY_MODE)
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

int32_t TiffContainer::SaveTileData(const uint32_t ifd_no, const uint32_t row, const uint32_t column, void* image_data, const uint32_t stride)
{
	ImageInfo image_info = { 0 };
	int32_t status = micro_tiff_GetImageInfo(_hdl, ifd_no, image_info);
	if (status != ErrorCode::STATUS_OK)
		return status;

	OmeSize image_size = { image_info.image_width, image_info.image_height };
	OmeSize tile_size = { image_info.block_width, image_info.block_height };
	OmeRect rect = { 0 };
	status = convert_tileIndex_to_rect(row, column, image_size, tile_size, rect);
	if (status != ErrorCode::STATUS_OK)
		return status;

	return SaveTileData(ifd_no, rect, image_info, image_data, stride);
}

static int32_t BufferCopy(void* src_buffer, const uint32_t src_stride, void* dst_buffer, const uint32_t dst_stride, 
	const uint32_t bytes_per_pixel, const OmeRect src_copy_rect, const OmeSize dst_paste_position)
{
	uint32_t src_copy_stride = (min)(src_copy_rect.width * bytes_per_pixel, dst_stride);
	for (uint32_t h = 0; h < src_copy_rect.height; h++)
	{
		uint8_t* src_ptr = (uint8_t*)src_buffer + (h + src_copy_rect.y) * src_stride + src_copy_rect.x * bytes_per_pixel;
		uint8_t* dst_ptr = (uint8_t*)dst_buffer + (h + dst_paste_position.height) * dst_stride + dst_paste_position.width * bytes_per_pixel;
		memcpy_s(dst_ptr, src_copy_stride, src_ptr, src_copy_stride);
	}

	return ErrorCode::STATUS_OK;
}

int32_t TiffContainer::SaveTileData(const uint32_t ifd_no, const OmeRect rect, const ImageInfo& image_info, void* image_data, const uint32_t stride)
{
	uint32_t tile_width = image_info.block_width;
	uint32_t tile_height = image_info.block_height;

	uint32_t bytes_per_pixel = image_info.image_byte_count * image_info.samples_per_pixel;

	uint32_t block_no = GetBlockId(rect.x, rect.y, tile_width, tile_height, image_info.image_width);
	uint32_t buf_width = stride == 0 ? rect.width : stride / bytes_per_pixel;

	uint64_t block_byte_size = rect.height * tile_width * bytes_per_pixel;
	//when rect.width != tileWidth, expand to a whole tile width

	void* buf = nullptr;
	unique_ptr<uint8_t[]> auto_buf = make_unique<uint8_t[]>(0);
	if (buf_width != tile_width)
	{
		auto_buf.reset(new uint8_t[block_byte_size]{ 0 });
		buf = auto_buf.get();
		if (buf == nullptr)
			return ErrorCode::ERR_BUFFER_IS_NULL;
		uint32_t src_stride = stride == 0 ? buf_width * bytes_per_pixel : stride;
		//if (buf_width < tile_width) : expand to full tile width
		//if (buf_width > tile_width) : only use buffer within tile width
		OmeRect copy_rect = { 0 };
		copy_rect.x = 0;
		copy_rect.y = 0;
		copy_rect.width = (min)(buf_width, rect.width);
		copy_rect.height = rect.height;
		BufferCopy(image_data, src_stride, buf, tile_width * bytes_per_pixel, bytes_per_pixel, copy_rect, { 0 });
	}
	else
	{
		buf = image_data;
	}

	int32_t status = ErrorCode::ERR_COMPRESS_TYPE_NOTSUPPORT;
	switch (image_info.compression)
	{
	case COMPRESSION_NONE:
		status = micro_tiff_SaveBlock(_hdl, ifd_no, block_no, block_byte_size, buf);
		break;
	case COMPRESSION_LZW:
	{
		//if (imageInfo.predictor == PREDICTOR_HORIZONTAL)
		//{
		//	status = horizontal_differencing(buf, rect.height, buf_width, imageInfo.image_byte_count, imageInfo.samples_per_pixel, false);
		//	if (status != 0) {
		//		return ErrorCode::ERR_LZW_HORIZONTAL_DIFFERENCING;
		//	}
		//}
		status = SaveTileLZW(buf, ifd_no, block_no, block_byte_size);
		break;
	}
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
	default:
		break;
	}

	return status;
}

int32_t TiffContainer::LoadRectData(const uint32_t ifd_no, const OmeSize dst_size, const OmeRect src_rect, void* image_data, const uint32_t stride)
{
	if (src_rect.width == dst_size.width && src_rect.height == dst_size.height)
	{
		return GetRectData(ifd_no, src_rect, image_data, stride);
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

int32_t TiffContainer::LoadTileData(const uint32_t ifd_no, const uint32_t row, const uint32_t column, void* buffer, const uint32_t stride)
{
	ImageInfo image_info = { 0 };
	int32_t status = micro_tiff_GetImageInfo(_hdl, ifd_no, image_info);
	if (status != ErrorCode::STATUS_OK)
		return status;

	OmeSize image_size = { image_info.image_width, image_info.image_height };
	OmeSize tile_size = { image_info.block_width, image_info.block_height };
	OmeRect rect = { 0 };
	status = convert_tileIndex_to_rect(row, column, image_size, tile_size, rect);
	if (status != ErrorCode::STATUS_OK)
		return status;

	return GetOneBlockData(ifd_no, rect, image_info, buffer, stride, { 0 });
}

//int32_t DecompressLZWData(void* encode_data, uint64_t encode_size, void* decode_data, uint64_t* decode_size, uint64_t max_decode_size);
//int32_t DecompressLZ4Data(void* encode_data, uint64_t encode_size, void* decode_data, uint64_t* decode_size, uint64_t max_decode_size);
//int32_t DecompressJPEGData(void* encode_data, uint64_t encode_size, void* decode_data, uint64_t* decode_size, int32_t* width);
//int32_t DecompressZlibData(void* encode_data, uint64_t encode_size, void* decode_data, uint64_t* decode_size);

int32_t TiffContainer::GetOneBlockData(const uint32_t ifd_no, const OmeRect rect, const ImageInfo& image_info, void* image_data, const uint32_t stride, OmeSize paste_start)
{
	if (rect.y +rect.height > image_info.image_height ||rect.x +rect.width > image_info.image_width)
		return ErrorCode::TIFF_ERR_BAD_PARAMETER_VALUE;

	uint32_t bytes_per_pixel = image_info.image_byte_count * image_info.samples_per_pixel;

	uint32_t block_no = GetBlockId(rect.x, rect.y, image_info.block_width, image_info.block_height, image_info.image_width);

	uint32_t height = image_info.block_height;
	uint32_t row_tail = image_info.image_height % image_info.block_height;
	uint32_t row_count = image_info.image_height / image_info.block_height;
	if (row_tail > 0)
	{
		double count = (double)(rect.y + rect.height) / image_info.block_height;
		if (count > (double)row_count && count < (double)row_count + 1)
			height = row_tail;
	}

	uint32_t block_actual_byte_size = height * image_info.block_width * bytes_per_pixel;
	uint32_t block_full_byte_size = image_info.block_height * image_info.block_width * bytes_per_pixel;

	//Get compressed data size
	uint64_t buffer_size;
	int32_t status = micro_tiff_LoadBlock(_hdl, ifd_no, block_no, buffer_size, nullptr);
	if (status != ErrorCode::STATUS_OK)
		return status;
	if (buffer_size == 0)
		return ErrorCode::TIFF_ERR_READ_DATA_FROM_FILE_FAILED;

	unique_ptr<uint8_t[]> auto_load_block_buf = make_unique<uint8_t[]>(buffer_size);
	uint8_t* load_block_buf = auto_load_block_buf.get();
	if (load_block_buf == nullptr)
		return ErrorCode::ERR_BUFFER_IS_NULL;

	//Load compressed data
	status = micro_tiff_LoadBlock(_hdl, ifd_no, block_no, buffer_size, load_block_buf);
	if (status != ErrorCode::STATUS_OK)
		return status;

	//decompress data information
	int32_t decompress_width = image_info.block_width;
	int32_t decompress_height = height;

	uint8_t* decompress_buf = nullptr;
	unique_ptr<uint8_t[]> auto_decompress_buf = make_unique<uint8_t[]>(0);

	if (image_info.compression == COMPRESSION_NONE) {
		decompress_buf = load_block_buf;
	}
	else
	{
		auto_decompress_buf.reset(new uint8_t[block_full_byte_size]{ 0 });
		decompress_buf = auto_decompress_buf.get();
		if (decompress_buf == nullptr)
			return ErrorCode::ERR_BUFFER_IS_NULL;

		//uint64_t actual_read_size = 0;
		switch (image_info.compression)
		{
			//case COMPRESSION_LZ4:
			//	decompress_status = DecompressLZ4Data(load_block_buf, count, decompress_buf, &actual_read_size, block_full_byte_size);
			//	break;
		case COMPRESSION_LZW:
		{
			int32_t decode_size = LZWDecode(load_block_buf, buffer_size, decompress_buf, block_full_byte_size);
			if (decode_size == block_full_byte_size)
				decompress_height = image_info.block_height;
			else if (decode_size == block_actual_byte_size)
				decompress_height = height;
			else
			{
				status = ErrorCode::ERR_DECOMPRESS_LZW_FAILED;
				break;
			}

			if (image_info.predictor == PREDICTOR_HORIZONTAL)
			{
				status = horizontal_acc(decompress_buf, decompress_height, decompress_width, image_info.image_byte_count, image_info.samples_per_pixel, false);
				if (status != 0)
					status = ErrorCode::ERR_HORIZONTAL_ACC_FAILED;
			}
			break;
		}
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
	if (rect.width != decompress_width || rect.height != decompress_height || (stride != 0 && stride != rect.width * bytes_per_pixel))
	{
		uint32_t src_stride = decompress_width * bytes_per_pixel;
		uint32_t dst_stride = stride == 0 ? rect.width * bytes_per_pixel : stride;

		OmeRect copy_rect = { 0 };
		copy_rect.x = rect.x % image_info.block_width;
		copy_rect.y = rect.y % image_info.block_height;
		copy_rect.width = rect.width;
		copy_rect.height = rect.height;
		BufferCopy(decompress_buf, src_stride, image_data, dst_stride, bytes_per_pixel, copy_rect, paste_start);
	}
	else
	{
		memcpy_s(image_data, block_actual_byte_size, decompress_buf, block_actual_byte_size);
	}

	return ErrorCode::STATUS_OK;
}

int32_t TiffContainer::GetRectData(const uint32_t ifd_no, const OmeRect rect, void* image_data, uint32_t stride)
{
	ImageInfo image_info;
	int32_t result = micro_tiff_GetImageInfo(_hdl, ifd_no, image_info);
	if (result != ErrorCode::STATUS_OK)
	{
		return result;
	}

	OmeRect rect_with_bin = { 0 };
	rect_with_bin.x = rect.x * _bin_size;
	rect_with_bin.y = rect.y;
	rect_with_bin.width = rect.width * _bin_size;
	rect_with_bin.height = rect.height;

	if (rect_with_bin.y + rect_with_bin.height > image_info.image_height || rect_with_bin.x + rect_with_bin.width > image_info.image_width)
		return ErrorCode::TIFF_ERR_BAD_PARAMETER_VALUE;

	uint32_t bytes_per_pixel = image_info.image_byte_count * image_info.samples_per_pixel;

	if (stride != 0 && (int32_t)(stride / bytes_per_pixel) < rect_with_bin.width)
		return ErrorCode::ERR_STRIDE_NOT_CORRECT;

	if (stride == 0)
		stride = rect_with_bin.width * bytes_per_pixel;

	int32_t status = ErrorCode::STATUS_OK;

	OmeRect tile_rect = { 0 };
	uint32_t x = 0;
	while (true)
	{
		tile_rect.x = x + rect_with_bin.x;
		uint32_t x_remainder = tile_rect.x % image_info.block_width;
		if (x_remainder + (rect_with_bin.width - x) < image_info.block_width)
			tile_rect.width = rect_with_bin.width - x;
		else
			tile_rect.width = image_info.block_width - x_remainder;

		uint32_t y = 0;
		while (true)
		{
			tile_rect.y = y + rect_with_bin.y;
			uint32_t y_remainder = tile_rect.y % image_info.block_height;
			if (y_remainder + (rect_with_bin.height - y) < image_info.block_height)
				tile_rect.height = rect_with_bin.height - y;
			else
				tile_rect.height = image_info.block_height - y_remainder;

			status = GetOneBlockData(ifd_no, tile_rect, image_info, image_data, stride, {x, y});
			if (status != ErrorCode::STATUS_OK)
				return status;

			y += tile_rect.height;
			if (y == rect_with_bin.height)
				break;
			else if (y > rect_with_bin.height)
				return ErrorCode::ERR_PARAMETER_INVALID;
		}
		x += tile_rect.width;
		if (x == rect_with_bin.width)
			break;
		else if (x > rect_with_bin.width)
			return ErrorCode::ERR_PARAMETER_INVALID;
	}

	return status;
}

int32_t TiffContainer::CreateIFD(const uint32_t width, const uint32_t height, 
	const uint32_t block_width, const uint32_t block_height,
	const PixelType pixel_type, const uint16_t samples_per_pixel, const CompressionMode compress_mode)
{
	ImageInfo info = { 0 };
	info.image_width = width;
	info.image_height = height;
	info.bits_per_sample = GetBits(pixel_type);
	info.block_width = block_width;
	info.block_height = block_height;
	info.image_byte_count = GetBytes(pixel_type);
	info.photometric = PHOTOMETRIC_MINISBLACK;
	info.planarconfig = PLANARCONFIG_CONTIG;
	info.predictor = PREDICTOR_NONE;
	info.samples_per_pixel = samples_per_pixel;

	switch (compress_mode)
	{
	case CompressionMode::COMPRESSIONMODE_NONE:
		info.compression = COMPRESSION_NONE;
		break;
	case CompressionMode::COMPRESSIONMODE_LZW:
		info.compression = COMPRESSION_LZW;
		//info.predictor = PREDICTOR_HORIZONTAL;
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

int32_t TiffContainer::SetTag(const uint32_t ifd_no, const uint16_t tag_id, const TiffTagDataType tag_type, const uint32_t tag_size, void* tag_value)
{
	return micro_tiff_SetTag(_hdl, ifd_no, tag_id, (uint16_t)tag_type, tag_size, tag_value);
}

int32_t TiffContainer::GetTag(const uint32_t ifd_no, const uint16_t tag_id, TiffTagDataType& tag_type, uint32_t& tag_count, void* tag_value)
{
    uint16_t tag_data_type = 0;
    uint32_t tag_data_count = 0;
    int32_t status = micro_tiff_GetTagInfo(_hdl, ifd_no, tag_id, tag_data_type, tag_data_count);
    if (status != ErrorCode::STATUS_OK)
        return status;

    if (tag_value == nullptr)
    {
        tag_type = (TiffTagDataType)tag_data_type;
        tag_count = tag_data_count;
        return ErrorCode::STATUS_OK;
    }

	if ((TiffTagDataType)tag_data_type != tag_type || tag_data_count != tag_count)
		return ErrorCode::ERR_TAG_CONDITION_NOT_MET;

	return micro_tiff_GetTag(_hdl, ifd_no, tag_id, tag_value);
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

int32_t TiffContainer::SaveTileLZW(void* image_data, const uint32_t ifd_no, const uint32_t block_no, const uint64_t block_size)
{
	size_t dst_size = (size_t)(block_size * 1.5);
	unique_ptr<uint8_t[]> auto_dst_buf = make_unique<uint8_t[]>(dst_size);
	uint8_t* dst_buf = auto_dst_buf.get();
	if (dst_buf == nullptr) {
		return ErrorCode::ERR_BUFFER_IS_NULL;
	}

	uint64_t raw_data_used_size;
	uint64_t output_data_used_size;
	int compress_status = LZWEncode(image_data, block_size, &raw_data_used_size, dst_buf, dst_size, &output_data_used_size);

	int32_t status = ErrorCode::STATUS_OK;
	if (compress_status != 1 || raw_data_used_size != block_size)
		return ErrorCode::ERR_COMPRESS_LZW_FAILED;

	status = micro_tiff_SaveBlock(_hdl, ifd_no, block_no, output_data_used_size, dst_buf);

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

//int32_t DecompressLZWData(void* encode_data, uint64_t encode_size, void* decode_data, uint64_t* decode_size, uint64_t max_decode_size)
//{
//	//decompress data
//	*decode_size = LZWDecode(encode_data, encode_size, decode_data, max_decode_size);
//	if (*decode_size != max_decode_size)
//	{
//		return ErrorCode::ERR_DECOMPRESS_LZW_FAILED;
//	}
//
//	return ErrorCode::STATUS_OK;
//}

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

int32_t TiffContainer::CloseIFD(const uint32_t ifd_no)
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
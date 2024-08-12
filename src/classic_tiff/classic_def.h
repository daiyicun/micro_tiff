#pragma once
#include <stdint.h>

namespace tiff
{
	enum ErrorCode {
		STATUS_OK = 0,

		TIFF_ERR_USELESS_HDL = -1,
		TIFF_ERR_READ_DATA_FROM_FILE_FAILED = -2,
		TIFF_ERR_OPEN_FILE_PARAMETER_ERROR = -3,
		TIFF_ERR_OPEN_FILE_FAILED = -4,
		TIFF_ERR_CLOSE_FILE_FAILED = -5,
		TIFF_ERR_NO_TIFF_FORMAT = -6,
		TIFF_ERR_BLOCK_SIZE_IN_IFD_IS_EMPTY = -7,
		TIFF_ERR_NEED_IFD_NOT_FOUND = -8,
		TIFF_ERR_BLOCK_OUT_OF_RANGE = -9,
		TIFF_ERR_NO_IFD_FOUND = -10,
		TIFF_ERR_FIRST_IFD_STRUCT_NOT_COMPLETE = -11,
		TIFF_ERR_BLOCK_OFFSET_OUT_OF_RANGE = -12,
		TIFF_ERR_TAG_NOT_FOUND = -13,
		TIFF_ERR_BAD_PARAMETER_VALUE = -14,
		TIFF_ERR_IFD_ALREADY_EXIST = -15,
		TIFF_ERR_IFD_OUT_OF_DIRECTORY = -16,
		TIFF_ERR_IFD_FULL = -17,
		TIFF_ERR_UNSUPPORTED_BITS_PER_SAMPLE = -18,
		TIFF_ERR_PREVIOUS_IFD_NOT_CLOSED = -19,
		TIFF_ERR_TAG_TYPE_INCORRECT = -20,
		TIFF_ERR_TAG_SIZE_INCORRECT = -21,
		TIFF_ERR_SEEK_FILE_POINTER_FAILED = -22,
		TIFF_ERR_ALLOC_MEMORY_FAILED = -23,
		TIFF_ERR_WRONG_OPEN_MODE = -24,
		TIFF_ERR_APPEND_TAG_NOT_ALLOWED = -25,
		TIFF_ERR_DUPLICATE_WRITE_NOT_ALLOWED = -26,
		TIFF_ERR_BIGENDIAN_NOT_SUPPORT = -27,

		ERR_FILE_PATH_ERROR = -101,
		ERR_HANDLE_NOT_EXIST = -102,
		ERR_BUFFER_SIZE_ERROR = -104,
		ERR_BUFFER_IS_NULL = -105,
		ERR_DECOMPRESS_JPEG_FAILED = -125,
		ERR_STRIDE_NOT_CORRECT = -137,
		ERR_DATA_TYPE_NOTSUPPORT = -138,
		ERR_COMPRESS_TYPE_NOTSUPPORT = -139,
		ERR_COMPRESS_ZLIB_ERROR = -143,
		ERR_DECOMPRESS_LZW_FAILED = -144,
		ERR_DECOMPRESS_ZLIB_FAILED = -147,
		ERR_OPENMODE = -148,
		ERR_SAVEDATA_WITHOUT_INFO = -149,
		ERR_TAG_CONDITION_NOT_MET = -150,
		ERR_LZW_HORIZONTAL_DIFFERENCING = -163,
		ERR_COMPRESS_LZW_ERROR = -164,
		ERR_COMPRESS_JPEG_ERROR = -165,
	};

	enum class TiffTagDataType {
		TIFF_NOTYPE = 0,      /* placeholder */
		TIFF_BYTE = 1,        /* 8-bit unsigned integer */
		TIFF_ASCII = 2,       /* 8-bit bytes w/ last byte null */
		TIFF_SHORT = 3,       /* 16-bit unsigned integer */
		TIFF_LONG = 4,        /* 32-bit unsigned integer */
		TIFF_RATIONAL = 5,    /* 64-bit unsigned fraction */
		TIFF_SBYTE = 6,       /* !8-bit signed integer */
		TIFF_UNDEFINED = 7,   /* !8-bit untyped data */
		TIFF_SSHORT = 8,      /* !16-bit signed integer */
		TIFF_SLONG = 9,       /* !32-bit signed integer */
		TIFF_SRATIONAL = 10,  /* !64-bit signed fraction */
		TIFF_FLOAT = 11,      /* !32-bit IEEE floating point */
		TIFF_DOUBLE = 12,     /* !64-bit IEEE floating point */
		TIFF_IFD = 13,        /* %32-bit unsigned integer (offset) */
		TIFF_LONG8 = 16,      /* BigTIFF 64-bit unsigned integer */
		TIFF_SLONG8 = 17,     /* BigTIFF 64-bit signed integer */
		TIFF_IFD8 = 18        /* BigTIFF 64-bit unsigned integer (offset) */
	};

	enum class PixelType {
		PIXEL_UNDEFINED = -1,
		PIXEL_INT8 = 0,
		PIXEL_UINT8 = 1,
		PIXEL_UINT16 = 2,
		PIXEL_INT16 = 3,
		PIXEL_FLOAT32 = 4,
	};

	enum class ImageType {
		IMAGE_GRAY = 0,
		IMAGE_RGB = 1,
		IMAGE_FLIM = 2,
	};

	enum class OpenMode {
		CREATE_MODE = 1,
		READ_WRITE_MODE = 2,
		READ_ONLY_MODE = 3,
	};

	enum class CompressionMode {
		COMPRESSIONMODE_NONE = 0,
		COMPRESSIONMODE_LZW = 2,
		COMPRESSIONMODE_JPEG = 3,
		COMPRESSIONMODE_ZIP = 4,
	};

	struct SingleImageInfo
	{
		uint32_t width;
		uint32_t height;
		uint16_t valid_bits;
		uint16_t samples_per_pixel;
		PixelType pixel_type;
		ImageType image_type;
		CompressionMode compress_mode;
	};
}
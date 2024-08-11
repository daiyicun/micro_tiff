#pragma once
#include <stdint.h>

namespace tiff
{
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
		IMAGE_RGB = 1
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
		uint16_t sample_per_pixel;
		PixelType pixel_type;
		ImageType image_type;
		CompressionMode compress_mode;
	};
}
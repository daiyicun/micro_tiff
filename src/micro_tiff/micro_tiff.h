#pragma once
#include <stdint.h>

#define	TIFFTAG_IMAGEWIDTH				256	/* image width in pixels */
#define	TIFFTAG_IMAGELENGTH				257	/* image height in pixels */
#define	TIFFTAG_BITSPERSAMPLE			258	/* bits per channel (sample) */
#define	TIFFTAG_COMPRESSION				259	/* data compression technique */
#define	    COMPRESSION_NONE				1	/* dump mode */
#define	    COMPRESSION_LZW					5	/* Lempel-Ziv  & Welch */
#define	    COMPRESSION_JPEG				7	/* %JPEG DCT compression */
#define     COMPRESSION_ADOBE_DEFLATE		8	/* Deflate compression, as recognized by Adobe */
#define	    COMPRESSION_LZ4					80
#define	    COMPRESSION_DEFLATE				32946	/* Deflate compression */
#define	TIFFTAG_PHOTOMETRIC				262 /* photometric interpretation */
#define	    PHOTOMETRIC_MINISWHITE			0	/* min value is white */
#define	    PHOTOMETRIC_MINISBLACK			1	/* min value is black */
#define	    PHOTOMETRIC_RGB					2	/* RGB color model */
#define	    PHOTOMETRIC_PALETTE				3	/* color map indexed */
#define	    PHOTOMETRIC_MASK				4	/* $holdout mask */
#define	    PHOTOMETRIC_SEPARATED			5	/* !color separations */
#define	    PHOTOMETRIC_YCBCR				6	/* !CCIR 601 */
#define	TIFFTAG_IMAGEDESCRIPTION		270 /* info about image */
#define TIFFTAG_MAKE					271 /* The scanner manufacturer */
#define TIFFTAG_MODEL					272 /* The scanner model name or number */
#define	TIFFTAG_STRIPOFFSETS			273 /* offsets to data strips */
#define TIFFTAG_SAMPLESPERPIXEL			277 /* The number of components per pixel. */
#define	TIFFTAG_ROWSPERSTRIP			278 /* rows per strip of data */
#define	TIFFTAG_STRIPBYTECOUNTS			279 /* bytes counts for strips */
#define	TIFFTAG_PLANARCONFIG			284 /* storage organization */
#define	    PLANARCONFIG_CONTIG				1	/* Chunky format. The component values for each pixel are stored contiguously. For example, for RGB data, the data is stored as RGBRGBRGB*/
#define	    PLANARCONFIG_SEPARATE			2	/* Planar format. The components are stored in separate component planes. For example, RGB data is stored with the Red components in one component plane, the Green in another, and the Blue in another. */

#define TIFFTAG_XRESOLUTION				282 /* The number of pixels per ResolutionUnit in the ImageWidth direction. */
#define TIFFTAG_YRESOLUTION				283 /* The number of pixels per ResolutionUnit in the ImageLength direction. */
#define TIFFTAG_RESOLUTIONUNIT			296 /* The unit of measurement for XResolution and YResolution. */
#define TIFFTAG_PREDICTOR				317 /* prediction scheme w/ LZW */
#define	    PREDICTOR_NONE					1	/* no prediction scheme used */
#define	    PREDICTOR_HORIZONTAL			2	/* horizontal differencing */
#define	    PREDICTOR_FLOATINGPOINT			3	/* floating point predictor */
#define	TIFFTAG_TILEWIDTH				322	/* !tile width in pixels */
#define	TIFFTAG_TILELENGTH				323	/* !tile height in pixels */
#define TIFFTAG_TILEOFFSETS				324	/* !offsets to data tiles */
#define TIFFTAG_TILEBYTECOUNTS			325	/* !byte counts for tiles */

#define OPENFLAG_READ		0x00
#define OPENFLAG_WRITE		0x01
#define	OPENFLAG_CREATE		0x02
#define OPENFLAG_BIGTIFF	0x04

typedef struct 
{
	uint32_t image_width;
	uint32_t image_height;
	uint32_t block_width;
	uint32_t block_height;
	uint16_t bits_per_sample;
	uint16_t samples_per_pixel;
	uint16_t image_byte_count;
	uint16_t compression;
	uint16_t photometric;
	uint16_t planarconfig;
	uint16_t predictor;
}ImageInfo;

typedef enum {
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
}TagDataType;


int32_t micro_tiff_Open(const wchar_t* full_name, uint8_t open_flag);
int32_t micro_tiff_Close(int32_t hdl);

int32_t micro_tiff_CreateIFD(int32_t hdl, ImageInfo &image_info);
int32_t micro_tiff_CloseIFD(int32_t hdl, int32_t ifd_no);

int32_t micro_tiff_GetIFDSize(int32_t hdl);
int32_t micro_tiff_GetImageInfo(int32_t hdl, uint32_t ifd_no, ImageInfo& image_info);

int32_t micro_tiff_SaveBlock(int32_t hdl, uint32_t ifd_no, uint32_t block_no, uint64_t actual_byte_size, void* buf);
int32_t micro_tiff_LoadBlock(int32_t hdl, uint32_t ifd_no, uint32_t block_no, uint64_t &actual_load_size, void* buf);

int32_t micro_tiff_SetTag(int32_t hdl, uint32_t ifd_no, uint16_t tag_id, uint16_t tag_data_type, uint32_t tag_count, void* buf);
int32_t micro_tiff_GetTagInfo(int32_t hdl, uint32_t ifd_no, uint16_t tag_id, uint16_t &tag_data_type, uint32_t &tag_count);
int32_t micro_tiff_GetTag(int32_t hdl, uint32_t ifd_no, uint16_t tag_id, void* buf);

//int32_t micro_tiff_SetPara(int32_t hdl, TiffParameter para, int32_t value);

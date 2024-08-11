#pragma once
#include <stdint.h>
#include <memory.h>
#include <vector>

#define TIFF_CLASSIC_HEADER_CHAR		{0x49,0x49,0x2a,0x00}//Fixed header in classic tiff format
#define TIFF_BIGTIFF_HEADER_CHAR		{0x49,0x49,0x2b,0x00,0x08,0x00,0x00,0x00}//Fixed header in bigtiff format

#define TIFF_HEADER_FLAG_STR "MICRO TIFF V2"
#define CLASSIC_TIFF_OFFSET_SIZE 4
#define BIG_TIFF_OFFSET_SIZE 8

#define CHECK_TIFF_ERROR(err) \
if (err != TiffErrorCode::TIFF_STATUS_OK) {\
	return err;\
}

//#define TIFF_MIN_IFD_SIZE				20*9+16//at least 9 tag
//#define CLASSIC_TIFF_MIN_IFD_SIZE		12*9+6


#define ReadSequence(buf, size, count) fread(buf, size, count, _tiff_hdl)
#define WriteSequence(buf, size, count) fwrite(buf, size, count, _tiff_hdl)
#define MemcpySequence(dst, src, size) memcpy_s(dst, size, src, size); dst+=size

inline uint16_t read_uint16(uint8_t* p, bool is_big_endian)
{
	uint16_t v;
	uint8_t* cp = (uint8_t*)&v;
	if (is_big_endian) {
		cp[1] = p[0]; cp[0] = p[1];
	}
	else {
		memcpy_s(cp, 2, p, 2);
	}
	return v;
}

inline uint32_t read_uint32(uint8_t* p, bool is_big_endian)
{
	uint32_t v;
	uint8_t* cp = (uint8_t*)&v;
	if (is_big_endian) {
		cp[3] = p[0]; cp[2] = p[1]; cp[1] = p[2]; cp[0] = p[3];
	}
	else {
		memcpy_s(cp, 4, p, 4);
	}
	return v;
}

inline uint64_t read_uint64(uint8_t* p, bool is_big_endian)
{
	uint64_t v;
	uint8_t* cp = (uint8_t*)&v;
	if (is_big_endian) {
		cp[7] = p[0]; cp[6] = p[1]; cp[5] = p[2]; cp[4] = p[3];
		cp[3] = p[4]; cp[2] = p[5]; cp[1] = p[6]; cp[0] = p[7];
	}
	else {
		memcpy_s(cp, 8, p, 8);
	}
	return v;
}

template<typename T>
inline int32_t check_handle(int32_t handle, std::vector<T*>& array, int32_t err)
{
	if (handle < 0 || handle >= (int32_t)array.size()) { return err; }
	//T* t = array[handle];
	if (array[handle] == nullptr) { return err; }
	return 0;
}

struct TagBigTiff
{
	uint16_t id;
	uint16_t data_type;
	uint64_t count;
	uint64_t value;
};

struct TagClassicTiff
{
	uint16_t id;
	uint16_t data_type;
	uint32_t count;
	uint32_t value;
};


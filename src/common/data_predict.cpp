#include "data_predict.h"
#include <stdint.h>

#define REPEAT4(n, op)		\
    switch (n) {		\
    default: { \
        unsigned long i; for (i = n-4; i > 0; i--) { op; } }  /*-fallthrough*/  \
    case 4:  op; /*-fallthrough*/ \
    case 3:  op; /*-fallthrough*/ \
    case 2:  op; /*-fallthrough*/ \
    case 1:  op; /*-fallthrough*/ \
    case 0:  ;			\
    }

void SwabArrayOfShort(uint16_t* sp, unsigned long n)
{
	unsigned char* cp;
	unsigned char t;
	/* XXX unroll loop some */
	while (n-- > 0) {
		cp = (unsigned char*)sp;
		t = cp[1]; cp[1] = cp[0]; cp[0] = t;
		sp++;
	}
}

void SwabArrayOfLong(uint32_t* lp, unsigned long n)
{
	unsigned char* cp;
	unsigned char t;
	/* XXX unroll loop some */
	while (n-- > 0) {
		cp = (unsigned char*)lp;
		t = cp[3]; cp[3] = cp[0]; cp[0] = t;
		t = cp[2]; cp[2] = cp[1]; cp[1] = t;
		lp++;
	}
}

int horizontal_differencing_8bits(void* data, unsigned long size, unsigned short stride)
{
	if ((size % stride) != 0)
		return -1;

	uint8_t* cp = (uint8_t*)data;

	if (size > stride)
	{
		size -= stride;

		if (stride == 3) {
			unsigned int r1, g1, b1;
			unsigned int r2 = cp[0];
			unsigned int g2 = cp[1];
			unsigned  int b2 = cp[2];
			do {
				r1 = cp[3]; cp[3] = (unsigned char)((r1 - r2) & 0xff); r2 = r1;
				g1 = cp[4]; cp[4] = (unsigned char)((g1 - g2) & 0xff); g2 = g1;
				b1 = cp[5]; cp[5] = (unsigned char)((b1 - b2) & 0xff); b2 = b1;
				cp += 3;
			} while ((size -= 3) > 0);
		}
		else if (stride == 4) {
			unsigned int r1, g1, b1, a1;
			unsigned int r2 = cp[0];
			unsigned int g2 = cp[1];
			unsigned int b2 = cp[2];
			unsigned int a2 = cp[3];
			do {
				r1 = cp[4]; cp[4] = (unsigned char)((r1 - r2) & 0xff); r2 = r1;
				g1 = cp[5]; cp[5] = (unsigned char)((g1 - g2) & 0xff); g2 = g1;
				b1 = cp[6]; cp[6] = (unsigned char)((b1 - b2) & 0xff); b2 = b1;
				a1 = cp[7]; cp[7] = (unsigned char)((a1 - a2) & 0xff); a2 = a1;
				cp += 4;
			} while ((size -= 4) > 0);
		}
		else {
			cp += size - 1;
			do {
				REPEAT4(stride, cp[stride] = (unsigned char)((cp[stride] - cp[0]) & 0xff); cp--)
			} while ((size -= stride) > 0);
		}
	}

	return 0;
}

int horizontal_differencing_16bits(void* data, unsigned long size, unsigned short stride)
{
	if ((size % (2 * stride)) != 0)
		return -1;

	uint16_t* wp = (uint16_t*)data;
	unsigned long wc = size / 2;

	if (wc > stride) {
		wc -= stride;
		wp += wc - 1;
		do {
			REPEAT4(stride, wp[stride] = (uint16_t)(((unsigned int)wp[stride] - (unsigned int)wp[0]) & 0xffff); wp--)
				wc -= stride;
		} while (wc > 0);
	}

	return 0;
}

int horizontal_differencing_32bits(void* data, unsigned long size, unsigned short stride)
{
	if ((size % (4 * stride)) != 0)
		return -1;

	uint32_t* wp = (uint32_t*)data;
	unsigned long wc = size / 4;

	if (wc > stride) {
		wc -= stride;
		wp += wc - 1;
		do {
			REPEAT4(stride, wp[stride] -= wp[0]; wp--)
				wc -= stride;
		} while (wc > 0);
	}

	return 0;
}

int swab_horizontal_differencing_16bits(void* data, unsigned long size, unsigned short stride)
{
	uint16_t* wp = (uint16_t*)data;
	unsigned long wc = size / 2;

	int status = horizontal_differencing_16bits(data, size, stride);
	if (status != 0)
		return status;

	SwabArrayOfShort(wp, wc);
	return 0;
}

int swab_horizontal_differencing_32bits(void* data, unsigned long size, unsigned short stride)
{
	uint32_t* wp = (uint32_t*)data;
	unsigned long wc = size / 4;

	int status = horizontal_differencing_32bits(data, size, stride);
	if (status != 0)
		return status;

	SwabArrayOfLong(wp, wc);
	return 0;
}

int horizontal_acc_8bits(void* data, unsigned long size, unsigned short stride)
{
	if ((size % stride) != 0)
		return -1;

	uint8_t* cp = (uint8_t*)data;

	if (size > stride) {
		/*
		 * Pipeline the most common cases.
		 */
		if (stride == 3) {
			unsigned int cr = cp[0];
			unsigned int cg = cp[1];
			unsigned int cb = cp[2];
			size -= 3;
			cp += 3;
			while (size > 0) {
				cp[0] = (unsigned char)((cr += cp[0]) & 0xff);
				cp[1] = (unsigned char)((cg += cp[1]) & 0xff);
				cp[2] = (unsigned char)((cb += cp[2]) & 0xff);
				size -= 3;
				cp += 3;
			}
		}
		else if (stride == 4) {
			unsigned int cr = cp[0];
			unsigned int cg = cp[1];
			unsigned int cb = cp[2];
			unsigned int ca = cp[3];
			size -= 4;
			cp += 4;
			while (size > 0) {
				cp[0] = (unsigned char)((cr += cp[0]) & 0xff);
				cp[1] = (unsigned char)((cg += cp[1]) & 0xff);
				cp[2] = (unsigned char)((cb += cp[2]) & 0xff);
				cp[3] = (unsigned char)((ca += cp[3]) & 0xff);
				size -= 4;
				cp += 4;
			}
		}
		else {
			size -= stride;
			do {
				REPEAT4(stride, cp[stride] =
					(unsigned char)((cp[stride] + *cp) & 0xff); cp++)
					size -= stride;
			} while (size > 0);
		}
	}

	return 0;
}

int horizontal_acc_16bits(void* data, unsigned long size, unsigned short stride)
{
	if ((size % (2 * stride)) != 0)
		return -1;

	uint16_t* wp = (uint16_t*)data;
	unsigned long wc = size / 2;

	if (wc > stride) {
		wc -= stride;
		do {
			REPEAT4(stride, wp[stride] = (uint16_t)(((unsigned int)wp[stride] + (unsigned int)wp[0]) & 0xffff); wp++)
				wc -= stride;
		} while (wc > 0);
	}

	return 0;
}

int horizontal_acc_32bits(void* data, unsigned long size, unsigned short stride)
{
	if ((size % (4 * stride)) != 0)
		return -1;

	uint32_t* wp = (uint32_t*)data;
	unsigned long wc = size / 4;

	if (wc > stride) {
		wc -= stride;
		do {
			REPEAT4(stride, wp[stride] += wp[0]; wp++)
				wc -= stride;
		} while (wc > 0);
	}

	return 0;
}

int swab_horizontal_acc_16bits(void* data, unsigned long size, unsigned short stride)
{
	uint16_t* wp = (uint16_t*)data;
	unsigned long wc = size / 2;

	SwabArrayOfShort(wp, wc);
	return horizontal_acc_16bits(data, size, stride);
}

int swab_horizontal_acc_32bits(void* data, unsigned long size, unsigned short stride)
{
	uint32_t* wp = (uint32_t*)data;
	unsigned long wc = size / 4;

	SwabArrayOfLong(wp, wc);
	return horizontal_acc_32bits(data, size, stride);
}

typedef int(*encodepfunc)(void* data, unsigned long size, unsigned short stride);
//encode
int horizontal_differencing(void* data, unsigned int height, unsigned int width, unsigned short data_bytes, unsigned short samples_per_pixel, bool is_big_endian)
{
	if (data == nullptr) return -1;
	encodepfunc func;
	switch (data_bytes)
	{
	case 1:
		func = horizontal_differencing_8bits;
		break;
	case 2:
		func = is_big_endian ? swab_horizontal_differencing_16bits : horizontal_differencing_16bits;
		break;
	default:
		return -2;
	}

	uint32_t stride = width * data_bytes * samples_per_pixel;

	for (unsigned int i = 0; i < height; ++i)
	{
		uint8_t* hori_buff = (uint8_t*)data + i * stride;
		func(hori_buff, stride, samples_per_pixel);
	}

	return 0;
}

typedef int(*decodepfunc)(void* data, unsigned long size, unsigned short stride);
//decode
int horizontal_acc(void* data, unsigned int height, unsigned int width, unsigned short data_bytes, unsigned short samples_per_pixel, bool is_big_endian)
{
	if (data == nullptr) return -1;
	decodepfunc func;
	switch (data_bytes)
	{
	case 1:
		func = horizontal_acc_8bits;
		break;
	case 2:
		func = is_big_endian ? swab_horizontal_acc_16bits : horizontal_acc_16bits;
		break;
	default:
		return -2;
	}

	uint32_t stride = width * data_bytes * samples_per_pixel;

	for (unsigned int i = 0; i < height; ++i)
	{
		uint8_t* hori_acc_buff = (uint8_t*)data + i * stride;
		func(hori_acc_buff, stride, samples_per_pixel);
	}

	return 0;
}
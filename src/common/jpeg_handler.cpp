#include "..\turbojpeg-2.0.6\turbojpeg.h"

int jpeg_decompress(unsigned char* buf, unsigned char* encoded_buf, unsigned long encoded_size, int* width, int* height, int* samples)
{
	int ret = -1;
	tjhandle handle = tjInitDecompress();
	if (handle == nullptr)
		return -1;
	int subsample, colorspace;
	if (tjDecompressHeader3(handle, encoded_buf, encoded_size, width, height, &subsample, &colorspace) == 0)
	{
		int pixelFormat = TJPF_RGB;	//only support RBG, other format not support
		*samples = 3;
		if (subsample == TJSAMP_GRAY && colorspace == TJCS_GRAY)
		{
			pixelFormat = TJPF_GRAY;
			*samples = 1;
		}
		ret = tjDecompress2(handle, encoded_buf, encoded_size, buf, *width, 0, *height, pixelFormat, TJFLAG_ACCURATEDCT);
	}
	tjDestroy(handle);
	return ret;
}

int jpeg_compress(unsigned char* buf, unsigned char** encoded_buf, unsigned long* encoded_size, unsigned long width, unsigned long height, int pitch, int pixelFormat, int subsamples)
{
	int ret = -1;
	tjhandle handle = tjInitCompress();
	if (handle == nullptr)
		return -1;
	ret = tjCompress2(handle, buf, width, pitch, height, pixelFormat, encoded_buf, encoded_size, subsamples, 99, TJFLAG_ACCURATEDCT);
	tjDestroy(handle);
	return ret;
}

void jpeg_free(unsigned char* buffer)
{
	tjFree(buffer);
}

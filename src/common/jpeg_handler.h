#pragma once
int jpeg_decompress(unsigned char* buf, unsigned char* encoded_buf, unsigned long encoded_size, int* width, int* height, int* samples);
//Default : save gray image, pixelFormat --- 6 --- TJPF_GRAY; subsamples --- 3 ---TJSAMP_GRAY
int jpeg_compress(unsigned char* buf, unsigned char** encoded_buf, unsigned long* encoded_size, unsigned long width, unsigned long height, int pitch = 0, int pixelFormat = 6, int subsamples = 3);

//When use jpeg_compress:
//If pass encoded_buf with null ptr, JpegLib will alloc new buffer as the encoded_buf, then you must call tjFree(unsigned char* buffer) to free.
//If pass an allocated encoded_buf, then JpegLib may re-alloc encoded_buf, you must call tjFree(unsigned char* buffer) to free also.
//If you really want manage encoded_buf yourself, you must setting #TJFLAG_NOREALLOC for JpegLib to guarantee JpegLib won't re-alloc buffer.
//See document : turbojepeg.h --- Line:706~Line715 & Line:1633~Line:1637
void jpeg_free(unsigned char* buffer);
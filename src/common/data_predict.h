#pragma once
//encode
int horizontal_differencing(void* data, unsigned int height, unsigned int width, unsigned short data_bytes, unsigned short samples_per_pixel, bool is_big_endian);

//decode
int horizontal_acc(void* data, unsigned int height, unsigned int width, unsigned short data_bytes, unsigned short samples_per_pixel, bool is_big_endian);
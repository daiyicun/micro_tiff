#pragma once
#include <stdint.h>
#if defined (__cplusplus)
extern "C" {
#endif
int LZWEncode(void* raw_data, uint64_t raw_data_size, uint64_t* raw_data_used_size, void* output_data, uint64_t max_output_size, uint64_t* output_data_used_size);
int LZWDecode(void* input_data, uint64_t input_data_size, void* output_data, uint64_t max_output_size);
#if defined (__cplusplus)
}
#endif
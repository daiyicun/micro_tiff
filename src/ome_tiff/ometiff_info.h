#pragma once
#include <stdint.h>
#include "ometiff.h"

int32_t parse_ome_xml(const char* xml, size_t xml_size, OmeTiff* tiff_obj);
int32_t generate_ome_xml(char** xml, OmeTiff* tiff_obj, int32_t header_width, int32_t header_height);


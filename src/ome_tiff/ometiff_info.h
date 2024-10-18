#pragma once
#include "ometiff.h"

int32_t parse_ome_xml(const std::string& xml, OmeTiff* tiff_obj);
int32_t generate_ome_xml(std::string& xml, const OmeTiff* tiff_obj);


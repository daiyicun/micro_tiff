#pragma once
#include "..\common\tiff_def.h"
#include <map>
#include <vector>

class tiff_single
{
public:
	int32_t open_tiff(const wchar_t* file_name, tiff::OpenMode mode);
	int32_t close_tiff();
	int32_t create_image(tiff::SingleImageInfo info);
	int32_t get_image_info(uint32_t image_number, tiff::SingleImageInfo* info);
	int32_t save_image_data(uint32_t image_number, void* image_data, uint32_t stride);
	int32_t load_image_data(uint32_t image_number, void* image_data, uint32_t stride);
	int32_t set_image_tag(uint32_t image_number, uint16_t tag_id, uint16_t tag_type, uint32_t tag_count, void* tag_value);
	int32_t get_image_tag(uint32_t image_number, uint16_t tag_id, uint32_t tag_size, void* tag_value);
	int32_t get_image_count(uint32_t* image_count);

private:
	int32_t _hdl;
	std::map<uint32_t, tiff::SingleImageInfo> _infos;
	tiff::OpenMode _openMode;
};
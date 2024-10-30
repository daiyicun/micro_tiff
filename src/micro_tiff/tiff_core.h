#pragma once
#include <vector>
#include <string>
#include <mutex>
#include "micro_tiff.h"
#include "tiff_ifd.h"
#include "tiff_err.h"

class tiff_core
{
public:
	tiff_core(void);
	~tiff_core(void);
	TiffErrorCode open(const wchar_t* tiffFullName, uint8_t open_flag);
	TiffErrorCode close(void);
	//int32_t get_tiff_hdl(void) { return _tiff_hdl; }
	uint8_t get_open_flag(void) const { return _open_flag; }
	std::wstring get_full_path_name(void) const { return _full_path_name; }
	uint32_t get_ifd_size(void) const { return (uint32_t)_ifd_container.size(); }
	bool is_big_tiff(void) const { return _big_tiff; }
	bool is_big_endian(void) const { return _big_endian; }

	int32_t create_ifd(const ImageInfo& image_info);
	int32_t close_ifd(uint32_t ifd_no);
	int32_t save_block(uint32_t ifd_no, uint32_t block_no, uint64_t actual_byte_size, uint8_t* buf);
	int32_t load_block(uint32_t ifd_no, uint32_t block_no, uint64_t& actual_byte_size, uint8_t* buf);
	int32_t get_image_info(uint32_t ifd_no, ImageInfo& image_info);
	int32_t set_tag(uint32_t ifd_no, uint16_t tag_id, uint16_t tag_data_type, uint32_t tag_count, void* buf);
	int32_t get_tag_info(uint32_t ifd_no, uint16_t tag_id, uint16_t& tag_data_type, uint32_t& tag_count);
	int32_t get_tag(uint32_t ifd_no, uint16_t tag_id, void* buf);

	//public variable
private:
	FILE* _tiff_hdl;
	uint8_t _open_flag;
	bool _big_tiff;
	bool _big_endian;
	std::wstring _full_path_name;
	std::vector<tiff_ifd*> _ifd_container;
	std::mutex _mutex;

	uint64_t _tif_first_ifd_position;
	uint64_t _tif_first_ifd_offset;

	TiffErrorCode write_header(void);
	TiffErrorCode read_header(void);
	int32_t load_ifds(void);
	void dispose(void);
};


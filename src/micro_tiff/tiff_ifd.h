#pragma once
#include "tiff_def.h"
#include "tiff_err.h"
#include "micro_tiff.h"
#include <map>
//#include <vector>
//#include <mutex>

class tiff_ifd
{
public:
	tiff_ifd(bool is_big_tiff, bool is_big_endian, FILE* hdl);
	~tiff_ifd(void);
	TiffErrorCode wr_ifd_info(const ImageInfo& image_info);
	//TiffErrorCode wr_close(void);
	TiffErrorCode wr_purge(void);
	TiffErrorCode wr_block(uint32_t block_no, uint64_t buf_size, const uint8_t* buf);

	//TiffErrorCode rd_init(FILE* hdl);
	//TiffErrorCode rd_close(void);
	TiffErrorCode load_ifd(uint64_t ifd_offset);
	void rd_ifd_info(ImageInfo& image_info) const;
	TiffErrorCode rd_block(uint32_t block_no, uint64_t& buf_size, uint8_t* buf);

	TiffErrorCode set_tag(uint16_t tag_id, uint16_t tag_data_type, uint32_t tag_count, void* buf);
	TiffErrorCode get_tag_info(uint16_t tag_id, uint16_t& tag_data_type, uint32_t& tag_count);
	TiffErrorCode get_tag(uint16_t tag_id, void* buf);

	bool get_is_purged() const { return _is_purged; }
	uint64_t get_current_ifd_offset(void) const { return _current_ifd_offset; }
	uint64_t get_next_ifd_pos(void) const { return _next_ifd_pos; }
	uint64_t get_next_ifd_offset(void) const { return _next_ifd_offset; }

private:
	FILE* _tiff_hdl;
	ImageInfo _info;
	bool _is_purged;
	bool _big_tiff;
	bool _big_endian;
	uint64_t _current_ifd_offset;
	uint64_t _next_ifd_pos;
	uint64_t _next_ifd_offset;
	size_t _block_count;
	size_t _num_of_tags;

	uint64_t* _big_block_byte_size_array;
	uint64_t* _big_block_offset_array;
	std::map<uint16_t, TagBigTiff> _big_tags;

	uint32_t* _classic_block_byte_size_array;
	uint32_t* _classic_block_offset_array;
	std::map<uint16_t, TagClassicTiff> _classic_tag;

	void generate_tag_list(uint64_t pos_offset, uint64_t pos_byte_count);
	TiffErrorCode parse_ifd_info(void);
	size_t get_block_count(void) const;

	TiffErrorCode purge_tag(uint16_t tag_id, size_t previous_size);
};

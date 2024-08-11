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
	TiffErrorCode wr_ifd_info(ImageInfo& image_info);
	//TiffErrorCode wr_close(void);
	TiffErrorCode wr_purge(void);
	int32_t wr_block(uint32_t block_no, uint64_t buf_size, uint8_t* buf);

	//TiffErrorCode rd_init(FILE* hdl);
	//TiffErrorCode rd_close(void);
	TiffErrorCode load_ifd(uint64_t ifd_offset);
	void rd_ifd_info(ImageInfo& image_info);
	int32_t rd_block(uint32_t block_no, uint64_t &buf_size, uint8_t* buf);

	TiffErrorCode set_tag(uint16_t tag_id, uint16_t tag_data_type, uint32_t tag_count, void* buf);
	TiffErrorCode get_tag_info(uint16_t tag_id, uint16_t& tag_data_type, uint32_t& tag_count);
	TiffErrorCode get_tag(uint16_t tag_id, void* buf);

	bool get_is_purged() { return is_purged; }
	uint64_t get_current_ifd_offset(void) { return current_ifd_offset; }
	uint64_t get_next_ifd_pos(void) { return next_ifd_pos; }
	uint64_t get_next_ifd_offset(void) { return next_ifd_offset; }

private:
	FILE* _tiff_hdl;
	ImageInfo info;
	bool is_purged;
	bool _big_tiff;
	bool _big_endian;
	uint64_t current_ifd_offset;
	uint64_t next_ifd_pos;
	uint64_t next_ifd_offset;
	size_t block_count;
	size_t num_of_tags;

	uint64_t* big_block_byte_size;
	uint64_t* big_block_offset;
	std::map<uint16_t, TagBigTiff> big_tag;

	uint32_t* classic_block_byte_size;
	uint32_t* classic_block_offset;
	std::map<uint16_t, TagClassicTiff> classic_tag;

	void GenerateTagList(uint64_t pos_offset, uint64_t pos_byte_count);
	int32_t ParseIFDInfo(void);
	size_t GetBlockCount(void);

	TiffErrorCode purge_tag(uint16_t tag_id, size_t previous_size);
};

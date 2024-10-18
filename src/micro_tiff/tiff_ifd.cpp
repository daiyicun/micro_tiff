#include "tiff_ifd.h"

uint8_t byte_size_of_tiff_data_type(uint16_t type)
{
	switch (type)
	{
	case TagDataType::TIFF_ASCII:
	case TagDataType::TIFF_BYTE:
	case TagDataType::TIFF_SBYTE:
	case TagDataType::TIFF_UNDEFINED:
		return 1;
	case TagDataType::TIFF_SHORT:
	case TagDataType::TIFF_SSHORT:
		return 2;
	case TagDataType::TIFF_LONG:
	case TagDataType::TIFF_SLONG:
	case TagDataType::TIFF_FLOAT:
	case TagDataType::TIFF_IFD:
		return 4;
	case TagDataType::TIFF_RATIONAL:
	case TagDataType::TIFF_SRATIONAL:
	case TagDataType::TIFF_DOUBLE:
	case TagDataType::TIFF_LONG8:
	case TagDataType::TIFF_SLONG8:
	case TagDataType::TIFF_IFD8:
		return 8;
	}
	return 0;
}

tiff_ifd::tiff_ifd(bool is_big_tiff, bool is_big_endian, FILE* hdl)
{
	//ifd_no = 0;
	_tiff_hdl = hdl;
	num_of_tags = 0;
	_big_tiff = is_big_tiff;
	_big_endian = is_big_endian;
	is_purged = false;
	current_ifd_offset = 0;	
	big_block_byte_size = nullptr;
	big_block_offset = nullptr;
	classic_block_byte_size = nullptr;
	classic_block_offset = nullptr;
	next_ifd_pos = 0;
	block_count = 0;
	next_ifd_offset = 0;
	info = { 0 };
}

tiff_ifd::~tiff_ifd(void)
{
	if (_big_tiff) {
		big_tag.clear();
		if (big_block_offset != nullptr) free(big_block_offset);
		if (big_block_byte_size != nullptr) free(big_block_byte_size);
	}
	else {
		classic_tag.clear();
		if (classic_block_offset != nullptr) free(classic_block_offset);
		if (classic_block_byte_size != nullptr) free(classic_block_byte_size);
	}
}

TiffErrorCode tiff_ifd::wr_ifd_info(ImageInfo& image_info)
{
	memcpy_s(&info, sizeof(ImageInfo), &image_info, sizeof(ImageInfo));
	block_count = GetBlockCount();

	if (_big_tiff) {
		big_block_offset = (uint64_t*)calloc(block_count, sizeof(uint64_t));
		big_block_byte_size = (uint64_t*)calloc(block_count, sizeof(uint64_t));
		if (big_block_offset == nullptr || big_block_byte_size == nullptr) {
			return TiffErrorCode::TIFF_ERR_ALLOC_MEMORY_FAILED;
		}
	}
	else {
		classic_block_offset = (uint32_t*)calloc(block_count, sizeof(uint32_t));
		classic_block_byte_size = (uint32_t*)calloc(block_count, sizeof(uint32_t));
		if (classic_block_offset == nullptr || classic_block_byte_size == nullptr) {
			return TiffErrorCode::TIFF_ERR_ALLOC_MEMORY_FAILED;
		}
	}
	return TiffErrorCode::TIFF_STATUS_OK;
}

size_t tiff_ifd::GetBlockCount(void)
{
	size_t r1 = info.image_width / info.block_width;
	size_t r2 = info.image_height / info.block_height;
	if (info.image_width % info.block_width) r1++;
	if (info.image_height % info.block_height) r2++;
	return r1 * r2;
}

void tiff_ifd::GenerateTagList(uint64_t pos_offset, uint64_t pos_byte_count)
{
	bool is_width_eq_block = (info.image_width == info.block_width);

	uint64_t block_offset = pos_offset;
	uint64_t block_bytes = pos_byte_count;
	
	//size_t extra_tag_size = is_width_eq_block ? 3 : 4;
	if (_big_tiff) {
		big_tag[TIFFTAG_IMAGEWIDTH] = { TIFFTAG_IMAGEWIDTH, TIFF_LONG, 1, info.image_width };
		big_tag[TIFFTAG_IMAGELENGTH] = { TIFFTAG_IMAGELENGTH, TIFF_LONG, 1, info.image_height };
		big_tag[TIFFTAG_BITSPERSAMPLE] = { TIFFTAG_BITSPERSAMPLE, TIFF_LONG, 1, info.bits_per_sample };
		big_tag[TIFFTAG_COMPRESSION] = { TIFFTAG_COMPRESSION, TIFF_LONG, 1, info.compression };
		big_tag[TIFFTAG_PHOTOMETRIC] = { TIFFTAG_PHOTOMETRIC, TIFF_LONG, 1, info.photometric };
		big_tag[TIFFTAG_SAMPLESPERPIXEL] = { TIFFTAG_SAMPLESPERPIXEL,TIFF_LONG,1,info.samples_per_pixel };
		big_tag[TIFFTAG_PLANARCONFIG] = { TIFFTAG_PLANARCONFIG, TIFF_LONG, 1, info.planarconfig };
		big_tag[TIFFTAG_PREDICTOR] = { TIFFTAG_PREDICTOR, TIFF_LONG, 1, info.predictor };

		if (block_count <= 1)
		{
			block_offset = big_block_offset[0];
			block_bytes = big_block_byte_size[0];
		}

		if (is_width_eq_block)
		{
			big_tag[TIFFTAG_ROWSPERSTRIP] = { TIFFTAG_ROWSPERSTRIP ,TIFF_LONG, 1, info.block_height };			
			big_tag[TIFFTAG_STRIPOFFSETS] = { TIFFTAG_STRIPOFFSETS, TIFF_LONG8, block_count, block_offset };
			big_tag[TIFFTAG_STRIPBYTECOUNTS] = { TIFFTAG_STRIPBYTECOUNTS,TIFF_LONG8, block_count, block_bytes };
		}
		else
		{
			big_tag[TIFFTAG_TILEWIDTH] = { TIFFTAG_TILEWIDTH, TIFF_LONG, 1, info.block_width };
			big_tag[TIFFTAG_TILELENGTH] = { TIFFTAG_TILELENGTH,TIFF_LONG, 1, info.block_height };
			big_tag[TIFFTAG_TILEOFFSETS] = { TIFFTAG_TILEOFFSETS, TIFF_LONG8, block_count, block_offset };
			big_tag[TIFFTAG_TILEBYTECOUNTS] = { TIFFTAG_TILEBYTECOUNTS,TIFF_LONG8, block_count, block_bytes };
		}
	}
	else {
		classic_tag[TIFFTAG_IMAGEWIDTH] = { TIFFTAG_IMAGEWIDTH, TIFF_SHORT, 1, info.image_width };
		classic_tag[TIFFTAG_IMAGELENGTH] = { TIFFTAG_IMAGELENGTH, TIFF_SHORT, 1, info.image_height };
		classic_tag[TIFFTAG_BITSPERSAMPLE] = { TIFFTAG_BITSPERSAMPLE, TIFF_SHORT, 1, info.bits_per_sample };
		classic_tag[TIFFTAG_COMPRESSION] = { TIFFTAG_COMPRESSION, TIFF_SHORT, 1, info.compression };
		classic_tag[TIFFTAG_PHOTOMETRIC] = { TIFFTAG_PHOTOMETRIC, TIFF_SHORT, 1, info.photometric };
		classic_tag[TIFFTAG_SAMPLESPERPIXEL] = { TIFFTAG_SAMPLESPERPIXEL,TIFF_SHORT,1,info.samples_per_pixel };
		classic_tag[TIFFTAG_PLANARCONFIG] = { TIFFTAG_PLANARCONFIG, TIFF_SHORT, 1, info.planarconfig };
		classic_tag[TIFFTAG_PREDICTOR] = { TIFFTAG_PREDICTOR, TIFF_SHORT, 1, info.predictor };


		if (block_count <= 1)
		{
			block_offset = classic_block_offset[0];
			block_bytes = classic_block_byte_size[0];
		}

		classic_tag[TIFFTAG_ROWSPERSTRIP] = { TIFFTAG_ROWSPERSTRIP,TIFF_LONG,1,info.block_height };
		classic_tag[TIFFTAG_STRIPOFFSETS] = { TIFFTAG_STRIPOFFSETS,TIFF_LONG,(uint32_t)block_count, (uint32_t)block_offset };
		classic_tag[TIFFTAG_STRIPBYTECOUNTS] = { TIFFTAG_STRIPBYTECOUNTS,TIFF_LONG,(uint32_t)block_count, (uint32_t)block_bytes };
	}
}

template<typename T1, typename T2>
inline T1 get_map_value(std::map<uint16_t, T2>& tags, uint16_t tag_id, T1 default_value)
{
	auto iter = tags.find(tag_id);
	if (iter == tags.end())
		return default_value;
	else
		return (T1)iter->second.value;
}

int32_t tiff_ifd::ParseIFDInfo()
{
	if (_big_tiff)
	{
		uint64_t pos_offset = 0, pos_count = 0;
		info.image_width = (uint32_t)big_tag[TIFFTAG_IMAGEWIDTH].value;
		info.image_height = (uint32_t)big_tag[TIFFTAG_IMAGELENGTH].value;
		auto iter_tileheight = big_tag.find(TIFFTAG_TILELENGTH);
		if (iter_tileheight != big_tag.end())
		{
			info.block_height = (uint32_t)big_tag[TIFFTAG_TILELENGTH].value;
			info.block_width = (uint32_t)big_tag[TIFFTAG_TILEWIDTH].value;
			block_count = (size_t)big_tag[TIFFTAG_TILEOFFSETS].count;
			pos_offset = big_tag[TIFFTAG_TILEOFFSETS].value;
			pos_count = big_tag[TIFFTAG_TILEBYTECOUNTS].value;
		}
		else
		{
			info.block_height = get_map_value<uint32_t, TagBigTiff>(big_tag, TIFFTAG_ROWSPERSTRIP, info.image_height);
			info.block_width = info.image_width;
			block_count = (size_t)big_tag[TIFFTAG_STRIPOFFSETS].count;
			pos_offset = big_tag[TIFFTAG_STRIPOFFSETS].value;
			pos_count = big_tag[TIFFTAG_STRIPBYTECOUNTS].value;
		}
		uint64_t bits_count = big_tag[TIFFTAG_BITSPERSAMPLE].count;
		if (bits_count == 1)
		{
			info.bits_per_sample = (uint16_t)big_tag[TIFFTAG_BITSPERSAMPLE].value;
		}
		else
		{
			uint64_t bits_offset = big_tag[TIFFTAG_BITSPERSAMPLE].value;
			int seek = _fseeki64(_tiff_hdl, bits_offset, SEEK_SET);
			if (seek != 0)
				return TiffErrorCode::TIFF_ERR_SEEK_FILE_POINTER_FAILED;
			ReadSequence(&info.bits_per_sample, 4, 1);
		}
		info.samples_per_pixel = get_map_value<uint16_t, TagBigTiff>(big_tag, TIFFTAG_SAMPLESPERPIXEL, 1);
		info.image_byte_count = (uint16_t)ceil((float)info.bits_per_sample / 8);
		info.compression = (uint16_t)big_tag[TIFFTAG_COMPRESSION].value;
		info.photometric = (uint16_t)big_tag[TIFFTAG_PHOTOMETRIC].value;
		info.planarconfig = (uint16_t)big_tag[TIFFTAG_PLANARCONFIG].value;
		info.predictor = get_map_value<uint16_t, TagBigTiff>(big_tag, TIFFTAG_PREDICTOR, 1);

		big_block_offset = (uint64_t*)calloc(block_count, sizeof(uint64_t));
		big_block_byte_size = (uint64_t*)calloc(block_count, sizeof(uint64_t));
		if (big_block_offset == nullptr || big_block_byte_size == nullptr) {
			return TiffErrorCode::TIFF_ERR_ALLOC_MEMORY_FAILED;
		}
		if (block_count == 1)
		{
			big_block_offset[0] = pos_offset;
			big_block_byte_size[0] = pos_count;
		}
		else
		{
			int seek = _fseeki64(_tiff_hdl, pos_offset, SEEK_SET);
			if (seek != 0)
				return TiffErrorCode::TIFF_ERR_SEEK_FILE_POINTER_FAILED;
			ReadSequence(big_block_offset, 8, block_count);
			seek = _fseeki64(_tiff_hdl, pos_count, SEEK_SET);
			if (seek != 0)
				return TiffErrorCode::TIFF_ERR_SEEK_FILE_POINTER_FAILED;
			ReadSequence(big_block_byte_size, 8, block_count);
		}
	}
	else
	{
		uint32_t pos_offset = 0, pos_count = 0;
		info.image_width = classic_tag[TIFFTAG_IMAGEWIDTH].value;
		info.image_height = classic_tag[TIFFTAG_IMAGELENGTH].value;
		auto iter_tileheight = classic_tag.find(TIFFTAG_TILELENGTH);
		if (iter_tileheight != classic_tag.end())
		{
			info.block_height = classic_tag[TIFFTAG_TILELENGTH].value;
			info.block_width = classic_tag[TIFFTAG_TILEWIDTH].value;
			block_count = classic_tag[TIFFTAG_TILEOFFSETS].count;
			pos_offset = classic_tag[TIFFTAG_TILEOFFSETS].value;
			pos_count = classic_tag[TIFFTAG_TILEBYTECOUNTS].value;
		}
		else
		{
			info.block_height = get_map_value<uint32_t, TagClassicTiff>(classic_tag, TIFFTAG_ROWSPERSTRIP, info.image_height);
			info.block_width = info.image_width;
			block_count = classic_tag[TIFFTAG_STRIPOFFSETS].count;
			pos_offset = classic_tag[TIFFTAG_STRIPOFFSETS].value;
			pos_count = classic_tag[TIFFTAG_STRIPBYTECOUNTS].value;
		}
		uint32_t bits_count = classic_tag[TIFFTAG_BITSPERSAMPLE].count;
		if (bits_count == 1)
		{
			info.bits_per_sample = (uint16_t)classic_tag[TIFFTAG_BITSPERSAMPLE].value;
		}
		else
		{
			uint32_t bits_offset = classic_tag[TIFFTAG_BITSPERSAMPLE].value;
			int seek = _fseeki64(_tiff_hdl, bits_offset, SEEK_SET);
			if (seek != 0)
				return TiffErrorCode::TIFF_ERR_SEEK_FILE_POINTER_FAILED;
			ReadSequence(&info.bits_per_sample, 4, 1);
		}
		info.samples_per_pixel = (uint16_t)classic_tag[TIFFTAG_SAMPLESPERPIXEL].value;
		info.image_byte_count = (uint16_t)ceil((float)info.bits_per_sample / 8);
		info.compression = (uint16_t)classic_tag[TIFFTAG_COMPRESSION].value;
		info.photometric = (uint16_t)classic_tag[TIFFTAG_PHOTOMETRIC].value;
		info.planarconfig = (uint16_t)classic_tag[TIFFTAG_PLANARCONFIG].value;
		info.predictor = get_map_value<uint16_t, TagClassicTiff>(classic_tag, TIFFTAG_PREDICTOR, 1);

		classic_block_offset = (uint32_t*)calloc(block_count, sizeof(uint32_t));
		classic_block_byte_size = (uint32_t*)calloc(block_count, sizeof(uint32_t));
		if (classic_block_offset == nullptr || classic_block_byte_size == nullptr) {
			return TiffErrorCode::TIFF_ERR_ALLOC_MEMORY_FAILED;
		}
		if (block_count == 1)
		{
			classic_block_offset[0] = pos_offset;
			classic_block_byte_size[0] = pos_count;
		}
		else
		{
			int seek = _fseeki64(_tiff_hdl, pos_offset, SEEK_SET);
			if (seek != 0)
				return TiffErrorCode::TIFF_ERR_SEEK_FILE_POINTER_FAILED;
			ReadSequence(classic_block_offset, 4, block_count);
			seek = _fseeki64(_tiff_hdl, pos_count, SEEK_SET);
			if (seek != 0)
				return TiffErrorCode::TIFF_ERR_SEEK_FILE_POINTER_FAILED;
			ReadSequence(classic_block_byte_size, 4, block_count);
		}
	}
	return TiffErrorCode::TIFF_STATUS_OK;
}

TiffErrorCode tiff_ifd::load_ifd(uint64_t ifd_offset)
{
	if (ifd_offset == 0) {
		return TiffErrorCode::TIFF_ERR_NO_IFD_FOUND;
	}
	else {
	 	int seek = _fseeki64(_tiff_hdl, ifd_offset, SEEK_SET);
		if (seek != 0)
			return TiffErrorCode::TIFF_ERR_IFD_OUT_OF_DIRECTORY;
	}

	size_t ifd_data_size = 0, ifd_size = 0;
	uint8_t data_num[8] = { 0 };

	if (_big_tiff) {
		ReadSequence(data_num, 8, 1);
		num_of_tags = (size_t)read_uint64(data_num, _big_endian);
		ifd_size = num_of_tags * sizeof(TagBigTiff);
		ifd_data_size = ifd_size + BIG_TIFF_OFFSET_SIZE;
		next_ifd_pos = ifd_offset + 8 + ifd_size;
		big_tag.clear();
	}
	else {
		ReadSequence(data_num, 2, 1);
		num_of_tags = (size_t)read_uint16(data_num, _big_endian);
		ifd_size = num_of_tags * sizeof(TagClassicTiff);
		ifd_data_size = ifd_size + CLASSIC_TIFF_OFFSET_SIZE;
		next_ifd_pos = ifd_offset + 2 + ifd_size;
		classic_tag.clear();
	}

	//the judgement number 1 and 1000 is from ImageJ code : "TiffDecoder.java"__line368
	if (num_of_tags < 1 || num_of_tags > 1000)
		return TiffErrorCode::TIFF_ERR_TAG_SIZE_INCORRECT;

	uint8_t* data_ifd = (uint8_t*)calloc(ifd_data_size, sizeof(uint8_t));
	if (data_ifd == nullptr) {
		return TiffErrorCode::TIFF_ERR_ALLOC_MEMORY_FAILED;
	}
	uint8_t* p = data_ifd;
	ReadSequence(data_ifd, 1, ifd_data_size);

	for (size_t idx = 0; idx < num_of_tags; idx++)
	{
		if (_big_tiff) {
			TagBigTiff bTag;
			memcpy_s(&bTag, sizeof(TagBigTiff), p, sizeof(TagBigTiff));
			big_tag[bTag.id] = bTag;
			p += sizeof(TagBigTiff);
		}
		else {
			TagClassicTiff cTag;
			memcpy_s(&cTag, sizeof(TagClassicTiff), p, sizeof(TagClassicTiff));
			classic_tag[cTag.id] = cTag;
			p += sizeof(TagClassicTiff);
		}
	}

	int32_t parse_err = ParseIFDInfo();
	if (parse_err != TiffErrorCode::TIFF_STATUS_OK)
		return (TiffErrorCode)parse_err;

	if (_big_tiff) {
		next_ifd_offset = read_uint64(p, _big_endian);
	}
	else {
		next_ifd_offset = read_uint32(p, _big_endian);
	}
	is_purged = true;
	current_ifd_offset = ifd_offset;
	free(data_ifd);
	return TiffErrorCode::TIFF_STATUS_OK;

}

void tiff_ifd::rd_ifd_info(ImageInfo& image_info)
{
	memcpy_s(&image_info, sizeof(ImageInfo), &info, sizeof(ImageInfo));
}

//TiffErrorCode tiff_ifd::wr_close(void)
//{
//	return TiffErrorCode::TIFF_STATUS_OK;
//}

TiffErrorCode tiff_ifd::wr_purge(void)
{
	_fseeki64(_tiff_hdl, 0, SEEK_END);
	uint64_t pos_offset = _ftelli64(_tiff_hdl);
	size_t block_size = 0;
	size_t total_size = 0;
	if (block_count > 1)
	{
		block_size = _big_tiff ? (block_count * BIG_TIFF_OFFSET_SIZE) : (block_count * CLASSIC_TIFF_OFFSET_SIZE);
	}
	total_size += block_size;
	uint64_t pos_byte_count = pos_offset + total_size;
	total_size += block_size;

	GenerateTagList(pos_offset, pos_byte_count);
	current_ifd_offset = pos_offset + total_size;

	uint64_t next_ifd_offset = 0;
	if (_big_tiff) {
		num_of_tags = big_tag.size();
		total_size += 8 + num_of_tags * sizeof(TagBigTiff);
	}
	else {
		num_of_tags = classic_tag.size();
		total_size += 2 + num_of_tags * sizeof(TagClassicTiff);
	}
	next_ifd_pos = pos_offset + total_size;
	total_size += _big_tiff ? BIG_TIFF_OFFSET_SIZE : CLASSIC_TIFF_OFFSET_SIZE;

	uint8_t* data_ifd = (uint8_t*)calloc(total_size, sizeof(uint8_t));
	if (data_ifd == nullptr) {
		return TiffErrorCode::TIFF_ERR_ALLOC_MEMORY_FAILED;
	}
	uint8_t* p = data_ifd;

	if (_big_tiff)
	{
		MemcpySequence(p, big_block_offset, block_size);
		MemcpySequence(p, big_block_byte_size, block_size);
		MemcpySequence(p, &num_of_tags, 8);
		for (auto i = big_tag.begin(); i != big_tag.end(); ++i)
		{
			TagBigTiff tagNew = i->second;
			MemcpySequence(p, &tagNew, sizeof(TagBigTiff));
		}
		MemcpySequence(p, &next_ifd_offset, BIG_TIFF_OFFSET_SIZE);
	}
	else
	{
		MemcpySequence(p, classic_block_offset, block_size);
		MemcpySequence(p, classic_block_byte_size, block_size);
		MemcpySequence(p, &num_of_tags, 2);
		for (auto i = classic_tag.begin(); i != classic_tag.end(); ++i)
		{
			TagClassicTiff tagNew = i->second;
			MemcpySequence(p, &tagNew, sizeof(TagClassicTiff));
		}
		MemcpySequence(p, &next_ifd_offset, CLASSIC_TIFF_OFFSET_SIZE);
	}
	WriteSequence(data_ifd, total_size, 1);
	free(data_ifd);
	is_purged = true;
	return TiffErrorCode::TIFF_STATUS_OK;
}

int32_t tiff_ifd::wr_block(uint32_t block_no, uint64_t buf_size, uint8_t* buf)
{
	_fseeki64(_tiff_hdl, 0, SEEK_END);
	uint64_t cur_offset = _ftelli64(_tiff_hdl);
	if (block_no >= block_count) {
		return TiffErrorCode::TIFF_ERR_BLOCK_OUT_OF_RANGE;
	}

	if (_big_tiff){
		big_block_offset[block_no] = cur_offset;
		big_block_byte_size[block_no] = buf_size;
	}
	else{
		classic_block_offset[block_no] = (uint32_t)cur_offset;
		classic_block_byte_size[block_no] = (uint32_t)buf_size;
	}
	WriteSequence(buf, (size_t)buf_size, 1);
	return TiffErrorCode::TIFF_STATUS_OK;
}

//TiffErrorCode tiff_ifd::rd_init(FILE* hdl)
//{
//	_tiff_hdl = hdl;
//	return TiffErrorCode::TIFF_STATUS_OK;
//}
//
//TiffErrorCode tiff_ifd::rd_close(void)
//{
//	return TiffErrorCode::TIFF_STATUS_OK;
//}

int32_t tiff_ifd::rd_block(uint32_t block_no, uint64_t &buf_size, uint8_t* buf)
{
	uint64_t cur_offset;
	if (block_no >= block_count) {
		return TiffErrorCode::TIFF_ERR_BLOCK_OUT_OF_RANGE;
	}

	if (_big_tiff) {
		cur_offset = big_block_offset[block_no];
		buf_size = big_block_byte_size[block_no];
	}
	else {
		cur_offset = classic_block_offset[block_no];
		buf_size = classic_block_byte_size[block_no];
	}
	if (buf != nullptr)
	{
		int seek = _fseeki64(_tiff_hdl, cur_offset, SEEK_SET);
		if (seek != 0)
			return TiffErrorCode::TIFF_ERR_BLOCK_OFFSET_OUT_OF_RANGE;
		ReadSequence(buf, (size_t)buf_size, 1);
	}
	return TiffErrorCode::TIFF_STATUS_OK;
}

TiffErrorCode tiff_ifd::set_tag(uint16_t tag_id, uint16_t tag_data_type, uint32_t tag_count, void* buf)
{
	if (tag_count < 1) {
		return TiffErrorCode::TIFF_ERR_TAG_SIZE_INCORRECT;
	}
	uint8_t type_size = byte_size_of_tiff_data_type(tag_data_type);
	if (type_size <= 0)
		return TiffErrorCode::TIFF_ERR_TAG_TYPE_INCORRECT;
	uint32_t size = type_size * tag_count;
	if (!_big_tiff && size > 4 && tag_count == 1)
		return TiffErrorCode::TIFF_ERR_TAG_SIZE_INCORRECT;

	uint32_t limit_size = _big_tiff ? BIG_TIFF_OFFSET_SIZE : CLASSIC_TIFF_OFFSET_SIZE;

	bool need_purge_tags = current_ifd_offset != 0;

	int origin = SEEK_END;
	long long offset = 0;
	size_t count = 0;

	if (_big_tiff)
	{
		if (!need_purge_tags) {
			big_tag[tag_id] = { tag_id,tag_data_type,tag_count, 0 };
		}
		else
		{
			auto iter = big_tag.find(tag_id);
			if (iter == big_tag.end())
			{
				return TiffErrorCode::TIFF_ERR_APPEND_TAG_NOT_ALLOWED;
			}
			else
			{
				TagBigTiff tag = iter->second;
				if (tag.data_type != tag_data_type)
					return TiffErrorCode::TIFF_ERR_TAG_TYPE_INCORRECT;
				if (tag.count != tag_count)
					return TiffErrorCode::TIFF_ERR_TAG_SIZE_INCORRECT;

				count = (size_t)std::distance(big_tag.begin(), iter);
				origin = SEEK_SET;
				offset = (long long)tag.value;
			}
		}
	}
	else
	{
		if (!need_purge_tags) {
			classic_tag[tag_id] = { tag_id,tag_data_type,tag_count, 0 };
		}
		else
		{
			auto iter = classic_tag.find(tag_id);
			if (iter == classic_tag.end())
			{
				return TiffErrorCode::TIFF_ERR_APPEND_TAG_NOT_ALLOWED;
			}
			else
			{
				TagClassicTiff tag = iter->second;
				if (tag.data_type != tag_data_type)
					return TiffErrorCode::TIFF_ERR_TAG_TYPE_INCORRECT;
				if (tag.count != tag_count)
					return TiffErrorCode::TIFF_ERR_TAG_SIZE_INCORRECT;

				count = (size_t)std::distance(classic_tag.begin(), iter);
				origin = SEEK_SET;
				offset = (long long)tag.value;
			}
		}
	}

	uint64_t value = 0;
	if (size > limit_size)
	{
		_fseeki64(_tiff_hdl, offset, origin);
		value = (uint64_t)_ftelli64(_tiff_hdl);
		WriteSequence(buf, 1, size);
		need_purge_tags = false;
	}
	else
	{
		memcpy(&value, buf, size);
	}

	if (_big_tiff)
	{
		big_tag[tag_id].value = value;
	}
	else
	{
		classic_tag[tag_id].value = (uint32_t)value;
	}

	if (need_purge_tags)
	{
		purge_tag(tag_id, count);
	}

	return TiffErrorCode::TIFF_STATUS_OK;
}

TiffErrorCode tiff_ifd::get_tag_info(uint16_t tag_id, uint16_t& tag_data_type, uint32_t& tag_count)
{
	if (_big_tiff)
	{
		auto iter = big_tag.find(tag_id);
		if (iter == big_tag.end())
			return TiffErrorCode::TIFF_ERR_TAG_NOT_FOUND;

		tag_data_type = iter->second.data_type;
		tag_count = (uint32_t)iter->second.count;
	}
	else
	{
		auto iter = classic_tag.find(tag_id);
		if (iter == classic_tag.end())
			return TiffErrorCode::TIFF_ERR_TAG_NOT_FOUND;

		tag_data_type = iter->second.data_type;
		tag_count = iter->second.count;
	}
	return TiffErrorCode::TIFF_STATUS_OK;
}

TiffErrorCode tiff_ifd::get_tag(uint16_t tag_id, void* buf)
{
	if (_big_tiff)
	{
		auto iter = big_tag.find(tag_id);
		if (iter == big_tag.end())
			return TiffErrorCode::TIFF_ERR_TAG_NOT_FOUND;

		TagBigTiff tag = iter->second;
		uint8_t type_size = byte_size_of_tiff_data_type(tag.data_type);
		uint64_t size = (uint64_t)type_size * tag.count;

		if (size > BIG_TIFF_OFFSET_SIZE)
		{
			int seek = _fseeki64(_tiff_hdl, tag.value, SEEK_SET);
			if (seek != 0) {
				return TiffErrorCode::TIFF_ERR_SEEK_FILE_POINTER_FAILED;
			}
			ReadSequence(buf, 1, (size_t)size);
		}
		else
		{
			memcpy(buf, &tag.value, (size_t)size);
		}
	}
	else
	{
		auto iter = classic_tag.find(tag_id);
		if (iter == classic_tag.end())
			return TiffErrorCode::TIFF_ERR_TAG_NOT_FOUND;

		TagClassicTiff tag = iter->second;
		uint8_t type_size = byte_size_of_tiff_data_type(tag.data_type);
		uint32_t size = type_size * tag.count;

		if (size > CLASSIC_TIFF_OFFSET_SIZE)
		{
			int seek = _fseeki64(_tiff_hdl, tag.value, SEEK_SET);
			if (seek != 0) {
				return TiffErrorCode::TIFF_ERR_SEEK_FILE_POINTER_FAILED;
			}
			ReadSequence(buf, 1, size);
		}
		else
		{
			memcpy(buf, &tag.value, size);
		}
	}
	return TiffErrorCode::TIFF_STATUS_OK;
}

TiffErrorCode tiff_ifd::purge_tag(uint16_t tag_id, size_t previous_size)
{
	size_t offset = (size_t)current_ifd_offset;
	if (_big_tiff) {
		offset += 8 + previous_size * sizeof(TagBigTiff);
		_fseeki64(_tiff_hdl, offset, SEEK_SET);
		WriteSequence(&big_tag[tag_id], sizeof(TagBigTiff), 1);
	}
	else {
		offset += 2 + previous_size * sizeof(TagClassicTiff);
		_fseeki64(_tiff_hdl, offset, SEEK_SET);
		WriteSequence(&classic_tag[tag_id], sizeof(TagClassicTiff), 1);
	}

	return TiffErrorCode::TIFF_STATUS_OK;
}
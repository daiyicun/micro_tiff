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

tiff_ifd::tiff_ifd(const bool is_big_tiff, const bool is_big_endian, FILE* hdl)
{
	//ifd_no = 0;
	_tiff_hdl = hdl;
	_num_of_tags = 0;
	_big_tiff = is_big_tiff;
	_big_endian = is_big_endian;
	_is_purged = false;
	_current_ifd_offset = 0;
	_big_block_byte_size_array = nullptr;
	_big_block_offset_array = nullptr;
	_classic_block_byte_size_array = nullptr;
	_classic_block_offset_array = nullptr;
	_next_ifd_pos = 0;
	_block_count = 0;
	_next_ifd_offset = 0;
	_info = { 0 };
}

tiff_ifd::~tiff_ifd(void)
{
	if (_big_tiff) {
		_big_tags.clear();
		if (_big_block_offset_array != nullptr) free(_big_block_offset_array);
		if (_big_block_byte_size_array != nullptr) free(_big_block_byte_size_array);
	}
	else {
		_classic_tag.clear();
		if (_classic_block_offset_array != nullptr) free(_classic_block_offset_array);
		if (_classic_block_byte_size_array != nullptr) free(_classic_block_byte_size_array);
	}
}

TiffErrorCode tiff_ifd::wr_ifd_info(const ImageInfo& image_info)
{
	memcpy_s(&_info, sizeof(ImageInfo), &image_info, sizeof(ImageInfo));
	_block_count = get_block_count();

	if (_big_tiff) {
		_big_block_offset_array = (uint64_t*)calloc(_block_count, sizeof(uint64_t));
		_big_block_byte_size_array = (uint64_t*)calloc(_block_count, sizeof(uint64_t));
		if (_big_block_offset_array == nullptr || _big_block_byte_size_array == nullptr) {
			return TiffErrorCode::TIFF_ERR_ALLOC_MEMORY_FAILED;
		}
	}
	else {
		_classic_block_offset_array = (uint32_t*)calloc(_block_count, sizeof(uint32_t));
		_classic_block_byte_size_array = (uint32_t*)calloc(_block_count, sizeof(uint32_t));
		if (_classic_block_offset_array == nullptr || _classic_block_byte_size_array == nullptr) {
			return TiffErrorCode::TIFF_ERR_ALLOC_MEMORY_FAILED;
		}
	}
	return TiffErrorCode::TIFF_STATUS_OK;
}

size_t tiff_ifd::get_block_count(void) const
{
	size_t r1 = _info.image_width / _info.block_width;
	size_t r2 = _info.image_height / _info.block_height;
	if (_info.image_width % _info.block_width) r1++;
	if (_info.image_height % _info.block_height) r2++;
	return r1 * r2;
}

void tiff_ifd::generate_tag_list(const uint64_t pos_offset, const uint64_t pos_byte_count)
{
	bool is_width_eq_block = (_info.image_width == _info.block_width);

	uint64_t block_offset = pos_offset;
	uint64_t block_bytes = pos_byte_count;

	//size_t extra_tag_size = is_width_eq_block ? 3 : 4;
	if (_big_tiff) {
		_big_tags[TIFFTAG_IMAGEWIDTH] = { TIFFTAG_IMAGEWIDTH, TIFF_LONG, 1, _info.image_width };
		_big_tags[TIFFTAG_IMAGELENGTH] = { TIFFTAG_IMAGELENGTH, TIFF_LONG, 1, _info.image_height };
		_big_tags[TIFFTAG_BITSPERSAMPLE] = { TIFFTAG_BITSPERSAMPLE, TIFF_LONG, 1, _info.bits_per_sample };
		_big_tags[TIFFTAG_COMPRESSION] = { TIFFTAG_COMPRESSION, TIFF_LONG, 1, _info.compression };
		_big_tags[TIFFTAG_PHOTOMETRIC] = { TIFFTAG_PHOTOMETRIC, TIFF_LONG, 1, _info.photometric };
		_big_tags[TIFFTAG_SAMPLESPERPIXEL] = { TIFFTAG_SAMPLESPERPIXEL,TIFF_LONG,1,_info.samples_per_pixel };
		_big_tags[TIFFTAG_PLANARCONFIG] = { TIFFTAG_PLANARCONFIG, TIFF_LONG, 1, _info.planarconfig };
		_big_tags[TIFFTAG_PREDICTOR] = { TIFFTAG_PREDICTOR, TIFF_LONG, 1, _info.predictor };

		if (_block_count <= 1)
		{
			block_offset = _big_block_offset_array[0];
			block_bytes = _big_block_byte_size_array[0];
		}

		if (is_width_eq_block)
		{
			_big_tags[TIFFTAG_ROWSPERSTRIP] = { TIFFTAG_ROWSPERSTRIP ,TIFF_LONG, 1, _info.block_height };
			_big_tags[TIFFTAG_STRIPOFFSETS] = { TIFFTAG_STRIPOFFSETS, TIFF_LONG8, _block_count, block_offset };
			_big_tags[TIFFTAG_STRIPBYTECOUNTS] = { TIFFTAG_STRIPBYTECOUNTS,TIFF_LONG8, _block_count, block_bytes };
		}
		else
		{
			_big_tags[TIFFTAG_TILEWIDTH] = { TIFFTAG_TILEWIDTH, TIFF_LONG, 1, _info.block_width };
			_big_tags[TIFFTAG_TILELENGTH] = { TIFFTAG_TILELENGTH,TIFF_LONG, 1, _info.block_height };
			_big_tags[TIFFTAG_TILEOFFSETS] = { TIFFTAG_TILEOFFSETS, TIFF_LONG8, _block_count, block_offset };
			_big_tags[TIFFTAG_TILEBYTECOUNTS] = { TIFFTAG_TILEBYTECOUNTS,TIFF_LONG8, _block_count, block_bytes };
		}
	}
	else {
		_classic_tag[TIFFTAG_IMAGEWIDTH] = { TIFFTAG_IMAGEWIDTH, TIFF_SHORT, 1, _info.image_width };
		_classic_tag[TIFFTAG_IMAGELENGTH] = { TIFFTAG_IMAGELENGTH, TIFF_SHORT, 1, _info.image_height };
		_classic_tag[TIFFTAG_BITSPERSAMPLE] = { TIFFTAG_BITSPERSAMPLE, TIFF_SHORT, 1, _info.bits_per_sample };
		_classic_tag[TIFFTAG_COMPRESSION] = { TIFFTAG_COMPRESSION, TIFF_SHORT, 1, _info.compression };
		_classic_tag[TIFFTAG_PHOTOMETRIC] = { TIFFTAG_PHOTOMETRIC, TIFF_SHORT, 1, _info.photometric };
		_classic_tag[TIFFTAG_SAMPLESPERPIXEL] = { TIFFTAG_SAMPLESPERPIXEL,TIFF_SHORT,1,_info.samples_per_pixel };
		_classic_tag[TIFFTAG_PLANARCONFIG] = { TIFFTAG_PLANARCONFIG, TIFF_SHORT, 1, _info.planarconfig };
		_classic_tag[TIFFTAG_PREDICTOR] = { TIFFTAG_PREDICTOR, TIFF_SHORT, 1, _info.predictor };


		if (_block_count <= 1)
		{
			block_offset = _classic_block_offset_array[0];
			block_bytes = _classic_block_byte_size_array[0];
		}

		_classic_tag[TIFFTAG_ROWSPERSTRIP] = { TIFFTAG_ROWSPERSTRIP,TIFF_LONG,1,_info.block_height };
		_classic_tag[TIFFTAG_STRIPOFFSETS] = { TIFFTAG_STRIPOFFSETS,TIFF_LONG,(uint32_t)_block_count, (uint32_t)block_offset };
		_classic_tag[TIFFTAG_STRIPBYTECOUNTS] = { TIFFTAG_STRIPBYTECOUNTS,TIFF_LONG,(uint32_t)_block_count, (uint32_t)block_bytes };
	}
}

template<typename T1, typename T2>
inline T1 get_map_value(const std::map<uint16_t, T2>& tags, const uint16_t tag_id, const T1 default_value)
{
	auto iter = tags.find(tag_id);
	if (iter == tags.end())
		return default_value;
	else
		return (T1)iter->second.value;
}

TiffErrorCode tiff_ifd::parse_ifd_info()
{
	if (_big_tiff)
	{
		uint64_t pos_offset = 0, pos_count = 0;
		_info.image_width = (uint32_t)_big_tags[TIFFTAG_IMAGEWIDTH].value;
		_info.image_height = (uint32_t)_big_tags[TIFFTAG_IMAGELENGTH].value;
		auto iter_tileheight = _big_tags.find(TIFFTAG_TILELENGTH);
		if (iter_tileheight != _big_tags.end())
		{
			_info.block_height = (uint32_t)_big_tags[TIFFTAG_TILELENGTH].value;
			_info.block_width = (uint32_t)_big_tags[TIFFTAG_TILEWIDTH].value;
			_block_count = (size_t)_big_tags[TIFFTAG_TILEOFFSETS].count;
			pos_offset = _big_tags[TIFFTAG_TILEOFFSETS].value;
			pos_count = _big_tags[TIFFTAG_TILEBYTECOUNTS].value;
		}
		else
		{
			_info.block_height = get_map_value<uint32_t, TagBigTiff>(_big_tags, TIFFTAG_ROWSPERSTRIP, _info.image_height);
			_info.block_width = _info.image_width;
			_block_count = (size_t)_big_tags[TIFFTAG_STRIPOFFSETS].count;
			pos_offset = _big_tags[TIFFTAG_STRIPOFFSETS].value;
			pos_count = _big_tags[TIFFTAG_STRIPBYTECOUNTS].value;
		}
		uint64_t bits_count = _big_tags[TIFFTAG_BITSPERSAMPLE].count;
		if (bits_count == 1)
		{
			_info.bits_per_sample = (uint16_t)_big_tags[TIFFTAG_BITSPERSAMPLE].value;
		}
		else
		{
			uint64_t bits_offset = _big_tags[TIFFTAG_BITSPERSAMPLE].value;
			int seek = _fseeki64(_tiff_hdl, bits_offset, SEEK_SET);
			if (seek != 0)
				return TiffErrorCode::TIFF_ERR_SEEK_FILE_POINTER_FAILED;
			ReadSequence(&_info.bits_per_sample, 4, 1);
		}
		_info.samples_per_pixel = get_map_value<uint16_t, TagBigTiff>(_big_tags, TIFFTAG_SAMPLESPERPIXEL, 1);
		_info.image_byte_count = (uint16_t)ceil((float)_info.bits_per_sample / 8);
		_info.compression = (uint16_t)_big_tags[TIFFTAG_COMPRESSION].value;
		_info.photometric = (uint16_t)_big_tags[TIFFTAG_PHOTOMETRIC].value;
		_info.planarconfig = (uint16_t)_big_tags[TIFFTAG_PLANARCONFIG].value;
		_info.predictor = get_map_value<uint16_t, TagBigTiff>(_big_tags, TIFFTAG_PREDICTOR, 1);

		_big_block_offset_array = (uint64_t*)calloc(_block_count, sizeof(uint64_t));
		_big_block_byte_size_array = (uint64_t*)calloc(_block_count, sizeof(uint64_t));
		if (_big_block_offset_array == nullptr || _big_block_byte_size_array == nullptr) {
			return TiffErrorCode::TIFF_ERR_ALLOC_MEMORY_FAILED;
		}
		if (_block_count == 1)
		{
			_big_block_offset_array[0] = pos_offset;
			_big_block_byte_size_array[0] = pos_count;
		}
		else
		{
			int seek = _fseeki64(_tiff_hdl, pos_offset, SEEK_SET);
			if (seek != 0)
				return TiffErrorCode::TIFF_ERR_SEEK_FILE_POINTER_FAILED;
			ReadSequence(_big_block_offset_array, 8, _block_count);
			seek = _fseeki64(_tiff_hdl, pos_count, SEEK_SET);
			if (seek != 0)
				return TiffErrorCode::TIFF_ERR_SEEK_FILE_POINTER_FAILED;
			ReadSequence(_big_block_byte_size_array, 8, _block_count);
		}
	}
	else
	{
		uint32_t pos_offset = 0, pos_count = 0;
		_info.image_width = _classic_tag[TIFFTAG_IMAGEWIDTH].value;
		_info.image_height = _classic_tag[TIFFTAG_IMAGELENGTH].value;
		auto iter_tileheight = _classic_tag.find(TIFFTAG_TILELENGTH);
		if (iter_tileheight != _classic_tag.end())
		{
			_info.block_height = _classic_tag[TIFFTAG_TILELENGTH].value;
			_info.block_width = _classic_tag[TIFFTAG_TILEWIDTH].value;
			_block_count = _classic_tag[TIFFTAG_TILEOFFSETS].count;
			pos_offset = _classic_tag[TIFFTAG_TILEOFFSETS].value;
			pos_count = _classic_tag[TIFFTAG_TILEBYTECOUNTS].value;
		}
		else
		{
			_info.block_height = get_map_value<uint32_t, TagClassicTiff>(_classic_tag, TIFFTAG_ROWSPERSTRIP, _info.image_height);
			_info.block_width = _info.image_width;
			_block_count = _classic_tag[TIFFTAG_STRIPOFFSETS].count;
			pos_offset = _classic_tag[TIFFTAG_STRIPOFFSETS].value;
			pos_count = _classic_tag[TIFFTAG_STRIPBYTECOUNTS].value;
		}
		uint32_t bits_count = _classic_tag[TIFFTAG_BITSPERSAMPLE].count;
		if (bits_count == 1)
		{
			_info.bits_per_sample = (uint16_t)_classic_tag[TIFFTAG_BITSPERSAMPLE].value;
		}
		else
		{
			uint32_t bits_offset = _classic_tag[TIFFTAG_BITSPERSAMPLE].value;
			int seek = _fseeki64(_tiff_hdl, bits_offset, SEEK_SET);
			if (seek != 0)
				return TiffErrorCode::TIFF_ERR_SEEK_FILE_POINTER_FAILED;
			ReadSequence(&_info.bits_per_sample, 4, 1);
		}
		_info.samples_per_pixel = (uint16_t)_classic_tag[TIFFTAG_SAMPLESPERPIXEL].value;
		_info.image_byte_count = (uint16_t)ceil((float)_info.bits_per_sample / 8);
		_info.compression = (uint16_t)_classic_tag[TIFFTAG_COMPRESSION].value;
		_info.photometric = (uint16_t)_classic_tag[TIFFTAG_PHOTOMETRIC].value;
		_info.planarconfig = (uint16_t)_classic_tag[TIFFTAG_PLANARCONFIG].value;
		_info.predictor = get_map_value<uint16_t, TagClassicTiff>(_classic_tag, TIFFTAG_PREDICTOR, 1);

		_classic_block_offset_array = (uint32_t*)calloc(_block_count, sizeof(uint32_t));
		_classic_block_byte_size_array = (uint32_t*)calloc(_block_count, sizeof(uint32_t));
		if (_classic_block_offset_array == nullptr || _classic_block_byte_size_array == nullptr) {
			return TiffErrorCode::TIFF_ERR_ALLOC_MEMORY_FAILED;
		}
		if (_block_count == 1)
		{
			_classic_block_offset_array[0] = pos_offset;
			_classic_block_byte_size_array[0] = pos_count;
		}
		else
		{
			int seek = _fseeki64(_tiff_hdl, pos_offset, SEEK_SET);
			if (seek != 0)
				return TiffErrorCode::TIFF_ERR_SEEK_FILE_POINTER_FAILED;
			ReadSequence(_classic_block_offset_array, 4, _block_count);
			seek = _fseeki64(_tiff_hdl, pos_count, SEEK_SET);
			if (seek != 0)
				return TiffErrorCode::TIFF_ERR_SEEK_FILE_POINTER_FAILED;
			ReadSequence(_classic_block_byte_size_array, 4, _block_count);
		}
	}
	return TiffErrorCode::TIFF_STATUS_OK;
}

TiffErrorCode tiff_ifd::load_ifd(const uint64_t ifd_offset)
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
		_num_of_tags = (size_t)read_uint64(data_num, _big_endian);
		ifd_size = _num_of_tags * sizeof(TagBigTiff);
		ifd_data_size = ifd_size + BIG_TIFF_OFFSET_SIZE;
		_next_ifd_pos = ifd_offset + 8 + ifd_size;
		_big_tags.clear();
	}
	else {
		ReadSequence(data_num, 2, 1);
		_num_of_tags = (size_t)read_uint16(data_num, _big_endian);
		ifd_size = _num_of_tags * sizeof(TagClassicTiff);
		ifd_data_size = ifd_size + CLASSIC_TIFF_OFFSET_SIZE;
		_next_ifd_pos = ifd_offset + 2 + ifd_size;
		_classic_tag.clear();
	}

	//the judgment number 1 and 1000 is from ImageJ code : "TiffDecoder.java"__line368
	if (_num_of_tags < 1 || _num_of_tags > 1000)
		return TiffErrorCode::TIFF_ERR_TAG_SIZE_INCORRECT;

	uint8_t* data_ifd = (uint8_t*)calloc(ifd_data_size, sizeof(uint8_t));
	if (data_ifd == nullptr) {
		return TiffErrorCode::TIFF_ERR_ALLOC_MEMORY_FAILED;
	}
	uint8_t* p = data_ifd;
	ReadSequence(data_ifd, 1, ifd_data_size);

	for (size_t idx = 0; idx < _num_of_tags; idx++)
	{
		if (_big_tiff) {
			TagBigTiff bTag;
			memcpy_s(&bTag, sizeof(TagBigTiff), p, sizeof(TagBigTiff));
			_big_tags[bTag.id] = bTag;
			p += sizeof(TagBigTiff);
		}
		else {
			TagClassicTiff cTag;
			memcpy_s(&cTag, sizeof(TagClassicTiff), p, sizeof(TagClassicTiff));
			_classic_tag[cTag.id] = cTag;
			p += sizeof(TagClassicTiff);
		}
	}

	int32_t parse_err = parse_ifd_info();
	if (parse_err != TiffErrorCode::TIFF_STATUS_OK)
		return (TiffErrorCode)parse_err;

	if (_big_tiff) {
		_next_ifd_offset = read_uint64(p, _big_endian);
	}
	else {
		_next_ifd_offset = read_uint32(p, _big_endian);
	}
	_is_purged = true;
	_current_ifd_offset = ifd_offset;
	free(data_ifd);
	return TiffErrorCode::TIFF_STATUS_OK;

}

void tiff_ifd::rd_ifd_info(ImageInfo& image_info) const
{
	memcpy_s(&image_info, sizeof(ImageInfo), &_info, sizeof(ImageInfo));
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
	if (_block_count > 1)
	{
		block_size = _big_tiff ? (_block_count * BIG_TIFF_OFFSET_SIZE) : (_block_count * CLASSIC_TIFF_OFFSET_SIZE);
	}
	total_size += block_size;
	uint64_t pos_byte_count = pos_offset + total_size;
	total_size += block_size;

	generate_tag_list(pos_offset, pos_byte_count);
	_current_ifd_offset = pos_offset + total_size;

	uint64_t next_ifd_offset = 0;
	if (_big_tiff) {
		_num_of_tags = _big_tags.size();
		total_size += 8 + _num_of_tags * sizeof(TagBigTiff);
	}
	else {
		_num_of_tags = _classic_tag.size();
		total_size += 2 + _num_of_tags * sizeof(TagClassicTiff);
	}
	_next_ifd_pos = pos_offset + total_size;
	total_size += _big_tiff ? BIG_TIFF_OFFSET_SIZE : CLASSIC_TIFF_OFFSET_SIZE;

	uint8_t* data_ifd = (uint8_t*)calloc(total_size, sizeof(uint8_t));
	if (data_ifd == nullptr) {
		return TiffErrorCode::TIFF_ERR_ALLOC_MEMORY_FAILED;
	}
	uint8_t* p = data_ifd;

	if (_big_tiff)
	{
		MemcpySequence(p, _big_block_offset_array, block_size);
		MemcpySequence(p, _big_block_byte_size_array, block_size);
		MemcpySequence(p, &_num_of_tags, 8);
		for (auto i = _big_tags.begin(); i != _big_tags.end(); ++i)
		{
			TagBigTiff tagNew = i->second;
			MemcpySequence(p, &tagNew, sizeof(TagBigTiff));
		}
		MemcpySequence(p, &next_ifd_offset, BIG_TIFF_OFFSET_SIZE);
	}
	else
	{
		MemcpySequence(p, _classic_block_offset_array, block_size);
		MemcpySequence(p, _classic_block_byte_size_array, block_size);
		MemcpySequence(p, &_num_of_tags, 2);
		for (auto i = _classic_tag.begin(); i != _classic_tag.end(); ++i)
		{
			TagClassicTiff tagNew = i->second;
			MemcpySequence(p, &tagNew, sizeof(TagClassicTiff));
		}
		MemcpySequence(p, &next_ifd_offset, CLASSIC_TIFF_OFFSET_SIZE);
	}
	WriteSequence(data_ifd, total_size, 1);
	free(data_ifd);
	_is_purged = true;
	return TiffErrorCode::TIFF_STATUS_OK;
}

TiffErrorCode tiff_ifd::wr_block(const uint32_t block_no, const uint64_t buf_size, const uint8_t* buf)
{
	_fseeki64(_tiff_hdl, 0, SEEK_END);
	uint64_t cur_offset = _ftelli64(_tiff_hdl);
	if (block_no >= _block_count) {
		return TiffErrorCode::TIFF_ERR_BLOCK_OUT_OF_RANGE;
	}

	if (_big_tiff) {
		_big_block_offset_array[block_no] = cur_offset;
		_big_block_byte_size_array[block_no] = buf_size;
	}
	else {
		_classic_block_offset_array[block_no] = (uint32_t)cur_offset;
		_classic_block_byte_size_array[block_no] = (uint32_t)buf_size;
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

TiffErrorCode tiff_ifd::rd_block(const uint32_t block_no, uint64_t& buf_size, uint8_t* buf)
{
	uint64_t cur_offset;
	if (block_no >= _block_count) {
		return TiffErrorCode::TIFF_ERR_BLOCK_OUT_OF_RANGE;
	}

	if (_big_tiff) {
		cur_offset = _big_block_offset_array[block_no];
		buf_size = _big_block_byte_size_array[block_no];
	}
	else {
		cur_offset = _classic_block_offset_array[block_no];
		buf_size = _classic_block_byte_size_array[block_no];
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

TiffErrorCode tiff_ifd::set_tag(const uint16_t tag_id, const uint16_t tag_data_type, const uint32_t tag_count, void* buf)
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

	bool need_purge_tags = _current_ifd_offset != 0;

	int origin = SEEK_END;
	long long offset = 0;
	size_t count = 0;

	if (_big_tiff)
	{
		if (!need_purge_tags) {
			_big_tags[tag_id] = { tag_id,tag_data_type,tag_count, 0 };
		}
		else
		{
			auto iter = _big_tags.find(tag_id);
			if (iter == _big_tags.end())
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

				count = (size_t)std::distance(_big_tags.begin(), iter);
				origin = SEEK_SET;
				offset = (long long)tag.value;
			}
		}
	}
	else
	{
		if (!need_purge_tags) {
			_classic_tag[tag_id] = { tag_id,tag_data_type,tag_count, 0 };
		}
		else
		{
			auto iter = _classic_tag.find(tag_id);
			if (iter == _classic_tag.end())
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

				count = (size_t)std::distance(_classic_tag.begin(), iter);
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
		_big_tags[tag_id].value = value;
	}
	else
	{
		_classic_tag[tag_id].value = (uint32_t)value;
	}

	if (need_purge_tags)
	{
		purge_tag(tag_id, count);
	}

	return TiffErrorCode::TIFF_STATUS_OK;
}

TiffErrorCode tiff_ifd::get_tag_info(const uint16_t tag_id, uint16_t& tag_data_type, uint32_t& tag_count)
{
	if (_big_tiff)
	{
		auto iter = _big_tags.find(tag_id);
		if (iter == _big_tags.end())
			return TiffErrorCode::TIFF_ERR_TAG_NOT_FOUND;

		tag_data_type = iter->second.data_type;
		tag_count = (uint32_t)iter->second.count;
	}
	else
	{
		auto iter = _classic_tag.find(tag_id);
		if (iter == _classic_tag.end())
			return TiffErrorCode::TIFF_ERR_TAG_NOT_FOUND;

		tag_data_type = iter->second.data_type;
		tag_count = iter->second.count;
	}
	return TiffErrorCode::TIFF_STATUS_OK;
}

TiffErrorCode tiff_ifd::get_tag(const uint16_t tag_id, void* buf)
{
	if (_big_tiff)
	{
		auto iter = _big_tags.find(tag_id);
		if (iter == _big_tags.end())
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
		auto iter = _classic_tag.find(tag_id);
		if (iter == _classic_tag.end())
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

TiffErrorCode tiff_ifd::purge_tag(const uint16_t tag_id, const size_t previous_size)
{
	size_t offset = (size_t)_current_ifd_offset;
	if (_big_tiff) {
		offset += 8 + previous_size * sizeof(TagBigTiff);
		_fseeki64(_tiff_hdl, offset, SEEK_SET);
		WriteSequence(&_big_tags[tag_id], sizeof(TagBigTiff), 1);
	}
	else {
		offset += 2 + previous_size * sizeof(TagClassicTiff);
		_fseeki64(_tiff_hdl, offset, SEEK_SET);
		WriteSequence(&_classic_tag[tag_id], sizeof(TagClassicTiff), 1);
	}

	return TiffErrorCode::TIFF_STATUS_OK;
}
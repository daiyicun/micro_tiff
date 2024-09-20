#include "tiff_core.h"
#include <io.h>
#include <fcntl.h>
using namespace std;

tiff_core::tiff_core(void)
{
	_tiff_hdl = nullptr;
	ifd_container.clear();
	_big_endian = 0;
	_big_tiff = 0;
	_open_flag = 0;
	tif_first_ifd_offset = 0;
	tif_first_ifd_position = 0;
}

tiff_core::~tiff_core(void)
{
	dispose();
}

void tiff_core::dispose(void)
{
	for (auto i : ifd_container) {
		if (i != nullptr) {
			delete(i);
		}
	}
	ifd_container.clear();
}

TiffErrorCode tiff_core::open(const wchar_t* tiffFullName, uint8_t open_flag)
{
	bool isCreate = open_flag & OPENFLAG_CREATE;
	bool isWrite = open_flag & OPENFLAG_WRITE;
	TiffErrorCode ret;
	if (isCreate && !isWrite) {
		return TiffErrorCode::TIFF_ERR_OPEN_FILE_PARAMETER_ERROR;
	}	

	if (isCreate) {
		_big_tiff = open_flag & OPENFLAG_BIGTIFF;
		_tiff_hdl = _wfsopen(tiffFullName, L"wb+", _SH_DENYWR);
	}
	else if (isWrite) {
		_tiff_hdl = _wfsopen(tiffFullName, L"rb+", _SH_DENYWR);
	}
	else {
		_tiff_hdl = _wfsopen(tiffFullName, L"rb", _SH_DENYWR);
	}
	if (_tiff_hdl == nullptr) {
		return TiffErrorCode::TIFF_ERR_OPEN_FILE_FAILED;
	}
	else {
		_full_path_name = wstring(tiffFullName);
		if (isCreate) 
		{
			ret = WriteHeader();
		} 
		else{
			ret = ReadHeader();//read/read_write mode.
		}
	}
	if (ret == TiffErrorCode::TIFF_STATUS_OK) {
		_open_flag = open_flag;		
	}
	return ret;
}

TiffErrorCode tiff_core::WriteHeader(void)
{
	uint8_t zero_data[8] = { 0 };
	_fseeki64(_tiff_hdl, 0, SEEK_SET);
	//tif_curOffset = 0;
	if (_big_tiff) {
		unsigned char data[] = TIFF_BIGTIFF_HEADER_CHAR;
		size_t _size = sizeof(data);
		WriteSequence(data, _size, 1);
		WriteSequence(zero_data, BIG_TIFF_OFFSET_SIZE, 1);
	}
	else {
		unsigned char data[] = TIFF_CLASSIC_HEADER_CHAR;
		size_t _size = sizeof(data);
		WriteSequence(data, _size, 1);
		WriteSequence(zero_data, CLASSIC_TIFF_OFFSET_SIZE, 1);
	}
	{
		unsigned char data[] = TIFF_HEADER_FLAG_STR;
		size_t _size = sizeof(data);
		WriteSequence(data, _size, 1);
	}

	return TiffErrorCode::TIFF_STATUS_OK;
}

TiffErrorCode tiff_core::ReadHeader(void)
{
	uint8_t data[16] = { 0 };
	size_t rd_size = 0;
	_fseeki64(_tiff_hdl, 0, SEEK_SET);
	rd_size = ReadSequence(data, 1, 16);
	if (rd_size != 16) return TiffErrorCode::TIFF_ERR_NO_TIFF_FORMAT;
	// 0x4d,0x4d:big endian ;0x49,0x49:little endian
	if (data[0] == 0x4d && data[1] == 0x4d) {
		_big_endian = true;
		return TiffErrorCode::TIFF_ERR_BIGENDIAN_NOT_SUPPORT;
	}
	else if (data[0] == 0x49 || data[1] == 0x49){
		_big_endian = false;
	}
	else {
		return TiffErrorCode::TIFF_ERR_NO_TIFF_FORMAT;
	}
	uint16_t bigtiff_flag = 0;
	bigtiff_flag = read_uint16(&data[2], _big_endian);
	//0x2a:classic tiff ;0x2b:BigTiff
	if (bigtiff_flag == 0x2b) {
		_big_tiff = true;
		tif_first_ifd_position = 4;
	}
	else if (bigtiff_flag == 0x2a) {
		_big_tiff = false;
		tif_first_ifd_position = 8;
	}
	else return TiffErrorCode::TIFF_ERR_NO_TIFF_FORMAT;

	if (_big_tiff) {
		tif_first_ifd_offset = read_uint64(&data[8], _big_endian);
	}
	else {
		tif_first_ifd_offset = read_uint32(&data[4], _big_endian);
	}

	int32_t ret_load_ifds = load_ifds();
	if (ret_load_ifds < 0)
		return TiffErrorCode::TIFF_ERR_NO_IFD_FOUND;
	return TiffErrorCode::TIFF_STATUS_OK;
}

TiffErrorCode tiff_core::close(void)
{
	if (_tiff_hdl != nullptr)
		fclose(_tiff_hdl);
	dispose();
	return TiffErrorCode::TIFF_STATUS_OK;
}

int32_t tiff_core::create_ifd(ImageInfo& image_info)
{
	if ((_open_flag & OPENFLAG_WRITE) == OPENFLAG_READ) {
		return TiffErrorCode::TIFF_ERR_WRONG_OPEN_MODE;
	}
	tiff_ifd* ifd = new(nothrow) tiff_ifd(_big_tiff, _big_endian, _tiff_hdl);
	if (ifd == nullptr) {
		return TiffErrorCode::TIFF_ERR_ALLOC_MEMORY_FAILED;
	}
	ifd_container.emplace_back(ifd);
	TiffErrorCode ret = ifd->wr_ifd_info(image_info);
	if (ret != TiffErrorCode::TIFF_STATUS_OK) {
		return ret;
	}
	return (int32_t)(ifd_container.size() - 1);
}

int32_t tiff_core::load_ifds(void)
{
	int32_t ifd_size = 0;
	uint64_t next_ifd_offset = tif_first_ifd_offset;
	dispose(); //clear if idf list not empty.

	if (tif_first_ifd_offset == 0) {
		return TiffErrorCode::TIFF_ERR_NO_IFD_FOUND;
	}

	int32_t err;
	while (next_ifd_offset != 0) {
		tiff_ifd* ifd = new(nothrow) tiff_ifd(_big_tiff, _big_endian, _tiff_hdl);
		if (ifd == nullptr) {
			return TiffErrorCode::TIFF_ERR_ALLOC_MEMORY_FAILED;
		}
		err = ifd->load_ifd(next_ifd_offset);
		if (err != TiffErrorCode::TIFF_STATUS_OK)
			break;
		next_ifd_offset = ifd->get_next_ifd_offset();
		ifd_container.emplace_back(ifd);
		ifd_size++;
	}

	return ifd_size;
}

int32_t tiff_core::save_block(uint32_t ifd_no, uint32_t block_no, uint64_t actual_byte_size, uint8_t* buf)
{
	if ((_open_flag & OPENFLAG_WRITE) == OPENFLAG_READ) {
		return TiffErrorCode::TIFF_ERR_WRONG_OPEN_MODE;
	}
	CHECK_TIFF_ERROR(check_handle<tiff_ifd>(ifd_no, ifd_container, TiffErrorCode::TIFF_ERR_NO_IFD_FOUND));
	tiff_ifd* ifd = ifd_container[ifd_no];
	unique_lock<mutex> lck(_mutex);
	return ifd->wr_block(block_no, actual_byte_size, buf);
}

int32_t tiff_core::load_block(uint32_t ifd_no, uint32_t block_no, uint64_t& actual_byte_size, uint8_t* buf)
{
	CHECK_TIFF_ERROR(check_handle<tiff_ifd>(ifd_no, ifd_container, TiffErrorCode::TIFF_ERR_NO_IFD_FOUND));
	tiff_ifd* ifd = ifd_container[ifd_no];
	unique_lock<mutex> lck(_mutex);
	return ifd->rd_block(block_no, actual_byte_size, buf);
}

int32_t tiff_core::close_ifd(uint32_t ifd_no)
{
	if ((_open_flag & OPENFLAG_WRITE) == OPENFLAG_READ) {
		return TiffErrorCode::TIFF_STATUS_OK;
	}
	CHECK_TIFF_ERROR(check_handle<tiff_ifd>(ifd_no, ifd_container, TiffErrorCode::TIFF_ERR_NO_IFD_FOUND));
	tiff_ifd* ifd = ifd_container[ifd_no];

	int64_t ifd_offset_pos;
	size_t write_size;
	if (_big_tiff) {
		ifd_offset_pos = 8;
		write_size = BIG_TIFF_OFFSET_SIZE;
	}
	else {
		ifd_offset_pos = 4;
		write_size = CLASSIC_TIFF_OFFSET_SIZE;
	}

	if (ifd_no > 0)
	{
		tiff_ifd* previous_ifd = ifd_container[ifd_no - 1];
		if (!previous_ifd->get_is_purged()) {
			return TiffErrorCode::TIFF_ERR_PREVIOUS_IFD_NOT_CLOSED;
		}
		ifd_offset_pos = previous_ifd->get_next_ifd_pos();
	}
	if (!ifd->get_is_purged()) {
		unique_lock<mutex> lck(_mutex);
		TiffErrorCode code = ifd->wr_purge();
		if (code != TiffErrorCode::TIFF_STATUS_OK)
			return code;

		uint64_t ifd_offset = ifd->get_current_ifd_offset();
		_fseeki64(_tiff_hdl, ifd_offset_pos, SEEK_SET);
		WriteSequence(&ifd_offset, write_size, 1);
		_fseeki64(_tiff_hdl, 0, SEEK_END);
	}
	
	return TiffErrorCode::TIFF_STATUS_OK;
}

int32_t tiff_core::get_image_info(uint32_t ifd_no, ImageInfo& image_info)
{
	CHECK_TIFF_ERROR(check_handle<tiff_ifd>(ifd_no, ifd_container, TiffErrorCode::TIFF_ERR_NO_IFD_FOUND));
	tiff_ifd* ifd = ifd_container[ifd_no];
	ifd->rd_ifd_info(image_info);
	return TiffErrorCode::TIFF_STATUS_OK;
}

int32_t tiff_core::set_tag(uint32_t ifd_no, uint16_t tag_id, uint16_t tag_data_type, uint32_t tag_count, void* buf)
{
	if ((_open_flag & OPENFLAG_WRITE) == OPENFLAG_READ) {
		return TiffErrorCode::TIFF_ERR_WRONG_OPEN_MODE;
	}
	CHECK_TIFF_ERROR(check_handle<tiff_ifd>(ifd_no, ifd_container, TiffErrorCode::TIFF_ERR_NO_IFD_FOUND));
	tiff_ifd* ifd = ifd_container[ifd_no];
	unique_lock<mutex> lck(_mutex);
	return ifd->set_tag(tag_id, tag_data_type, tag_count, buf);
}

int32_t tiff_core::get_tag_info(uint32_t ifd_no, uint16_t tag_id, uint16_t& tag_data_type, uint32_t& tag_count)
{
	CHECK_TIFF_ERROR(check_handle<tiff_ifd>(ifd_no, ifd_container, TiffErrorCode::TIFF_ERR_NO_IFD_FOUND));
	tiff_ifd* ifd = ifd_container[ifd_no];
	return ifd->get_tag_info(tag_id, tag_data_type, tag_count);
}

int32_t tiff_core::get_tag(uint32_t ifd_no, uint16_t tag_id, void* buf)
{
	CHECK_TIFF_ERROR(check_handle<tiff_ifd>(ifd_no, ifd_container, TiffErrorCode::TIFF_ERR_NO_IFD_FOUND));
	tiff_ifd* ifd = ifd_container[ifd_no];
	unique_lock<mutex> lck(_mutex);
	return ifd->get_tag(tag_id, buf);
}

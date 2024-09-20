#pragma once
#include <string>
#include <vector>
#include <mutex>
#include "tiff_core.h"
#include "micro_tiff.h"

using namespace std;

static vector<tiff_core*> g_tiff_array;
static mutex g_tiff_mutex;

int32_t check_valid_hdl(void)
{
	size_t size = g_tiff_array.size();
	tiff_core* a = nullptr;
	for (size_t i = 0; i < size; i++) {
		a = g_tiff_array.at(i);
		if (a == nullptr) return (int32_t)i;
	}
	return -1;
}

int32_t micro_tiff_Open(const wchar_t* full_name, uint8_t open_flag)
{
	if (open_flag & OPENFLAG_WRITE) {
		wstring s = wstring(full_name);
		size_t size = g_tiff_array.size();
		tiff_core* a = nullptr;
		for (size_t i = 0; i < size; i++) {
			a = g_tiff_array.at(i);
			if (a != nullptr) {
				if (a->get_full_path_name() == s) {
					if (a->get_open_flag() & OPENFLAG_WRITE) return TIFF_ERR_WRONG_OPEN_MODE;
				}
			}
		}
	}

	tiff_core* tiff = new(nothrow) tiff_core();
	if (tiff == nullptr) {
		return TiffErrorCode::TIFF_ERR_ALLOC_MEMORY_FAILED;
	}

	unique_lock<mutex> lock(g_tiff_mutex);
	int32_t hdl = check_valid_hdl();
	if (hdl < 0) {
		g_tiff_array.emplace_back(tiff);
		hdl = (int32_t)g_tiff_array.size() - 1;
	}
	else {
		g_tiff_array[hdl] = tiff;
	}

	TiffErrorCode status = tiff->open(full_name, open_flag);
	if (status != TiffErrorCode::TIFF_STATUS_OK)
	{
		tiff->close();
		delete tiff;
		g_tiff_array[hdl] = nullptr;
		return status;
	}
	return hdl;
}

int32_t micro_tiff_Close(int32_t hdl)
{
	unique_lock<mutex> lock(g_tiff_mutex);
	tiff_core* tiff = nullptr;
	if (hdl < 0 || hdl >= (int32_t)g_tiff_array.size()) {
		return TiffErrorCode::TIFF_ERR_USELESS_HDL;
	}
	tiff = g_tiff_array[hdl];
	if (tiff != nullptr) {
		tiff->close();
		delete tiff;
	}
	g_tiff_array[hdl] = nullptr;
	return TiffErrorCode::TIFF_STATUS_OK;
}

int32_t micro_tiff_CreateIFD(int32_t hdl, ImageInfo& image_info)
{
	CHECK_TIFF_ERROR(check_handle<tiff_core>(hdl, g_tiff_array, TiffErrorCode::TIFF_ERR_USELESS_HDL));
	tiff_core* tiff = g_tiff_array[hdl];
	return tiff->create_ifd(image_info);
}

int32_t micro_tiff_SaveBlock(int32_t hdl, uint32_t ifd_no, uint32_t block_no, uint64_t actual_byte_size, void* buf)
{
	CHECK_TIFF_ERROR(check_handle<tiff_core>(hdl, g_tiff_array, TiffErrorCode::TIFF_ERR_USELESS_HDL));
	tiff_core* tiff = g_tiff_array[hdl];
	return tiff->save_block(ifd_no, block_no, actual_byte_size, (uint8_t*)buf);
}

int32_t micro_tiff_LoadBlock(int32_t hdl, uint32_t ifd_no, uint32_t block_no, uint64_t& actual_load_size, void* buf)
{
	CHECK_TIFF_ERROR(check_handle<tiff_core>(hdl, g_tiff_array, TiffErrorCode::TIFF_ERR_USELESS_HDL));
	tiff_core* tiff = g_tiff_array[hdl];
	return tiff->load_block(ifd_no, block_no, actual_load_size, (uint8_t*)buf);
}

int32_t micro_tiff_CloseIFD(int32_t hdl, int32_t ifd_no)
{
	CHECK_TIFF_ERROR(check_handle<tiff_core>(hdl, g_tiff_array, TiffErrorCode::TIFF_ERR_USELESS_HDL));
	tiff_core* tiff = g_tiff_array[hdl];
	return tiff->close_ifd(ifd_no);
}

int32_t micro_tiff_GetImageInfo(int32_t hdl, uint32_t ifd_no, ImageInfo& image_info)
{
	CHECK_TIFF_ERROR(check_handle<tiff_core>(hdl, g_tiff_array, TiffErrorCode::TIFF_ERR_USELESS_HDL));
	tiff_core* tiff = g_tiff_array[hdl];
	return tiff->get_image_info(ifd_no, image_info);
}

int32_t micro_tiff_SetTag(int32_t hdl, uint32_t ifd_no, uint16_t tag_id, uint16_t tag_data_type, uint32_t tag_count, void* buf)
{
	CHECK_TIFF_ERROR(check_handle<tiff_core>(hdl, g_tiff_array, TiffErrorCode::TIFF_ERR_USELESS_HDL));
	tiff_core* tiff = g_tiff_array[hdl];
	return tiff->set_tag(ifd_no, tag_id, tag_data_type, tag_count, buf);
}

int32_t micro_tiff_GetTagInfo(int32_t hdl, uint32_t ifd_no, uint16_t tag_id, uint16_t& tag_data_type, uint32_t& tag_count)
{
	CHECK_TIFF_ERROR(check_handle<tiff_core>(hdl, g_tiff_array, TiffErrorCode::TIFF_ERR_USELESS_HDL));
	tiff_core* tiff = g_tiff_array[hdl];
	return tiff->get_tag_info(ifd_no, tag_id, tag_data_type, tag_count);
}

int32_t micro_tiff_GetTag(int32_t hdl, uint32_t ifd_no, uint16_t tag_id, void* buf)
{
	CHECK_TIFF_ERROR(check_handle<tiff_core>(hdl, g_tiff_array, TiffErrorCode::TIFF_ERR_USELESS_HDL));
	tiff_core* tiff = g_tiff_array[hdl];
	return tiff->get_tag(ifd_no, tag_id, buf);
}

int32_t micro_tiff_GetIFDSize(int32_t hdl)
{
	CHECK_TIFF_ERROR(check_handle<tiff_core>(hdl, g_tiff_array, TiffErrorCode::TIFF_ERR_USELESS_HDL));
	tiff_core* tiff = g_tiff_array[hdl];
	return tiff->get_ifd_size();
}
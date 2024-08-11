#include "classic_tiff_library.h"
#include "classic_tiff.h"
#include "..\common\error.h"
#include <vector>
#include <mutex>

//Functions for SINGLE-TIFF image
static std::vector<tiff_single*> vecSingleTiff;
static std::mutex mutexSingleTiff;

#ifndef CHECK_TIFF_HANDLE
#define CHECK_TIFF_HANDLE(handle) \
	if (handle < 0 || (size_t)handle >= vecSingleTiff.size() || vecSingleTiff[handle] == nullptr)\
		return ErrorCode::ERR_HANDLE_NOT_EXIST;
#endif // !CHECK_TIFF_HANDLE

#ifndef CHECK_TIFF_BUFFER
#define CHECK_TIFF_BUFFER(buffer) if (buffer == nullptr) return ErrorCode::ERR_BUFFER_IS_NULL;
#endif // !CHECK_TIFF_BUFFER


int32_t open_tiff(const wchar_t* file_name, tiff::OpenMode mode)
{
	tiff_single* tiff = new tiff_single();
	int32_t ret = tiff->open_tiff(file_name, mode);
	if (ret == ErrorCode::STATUS_OK) {
		std::unique_lock<std::mutex> tiff_lock(mutexSingleTiff);
		for (size_t i = 0; i < vecSingleTiff.size(); i++)
		{
			if (vecSingleTiff[i] == nullptr) {
				vecSingleTiff[i] = tiff;
				return (int32_t)i;
			}
		}
		vecSingleTiff.emplace_back(tiff);
		return (int32_t)vecSingleTiff.size() - 1;
	}
	delete tiff;
	return ret;
}

int32_t close_tiff(int32_t handle)
{
	std::unique_lock<std::mutex> tiff_lock(mutexSingleTiff);
	CHECK_TIFF_HANDLE(handle);
	tiff_single* tiff = vecSingleTiff[handle];
	tiff->close_tiff();
	delete(tiff);
	vecSingleTiff[handle] = nullptr;
	return ErrorCode::STATUS_OK;
}

int32_t create_image(int32_t handle, tiff::SingleImageInfo image_info)
{
	CHECK_TIFF_HANDLE(handle);
	return vecSingleTiff[handle]->create_image(image_info);
}

int32_t save_image_data(int32_t handle, uint32_t image_number, void* image_data, uint32_t stride)
{
	CHECK_TIFF_BUFFER(image_data);
	CHECK_TIFF_HANDLE(handle);
	return vecSingleTiff[handle]->save_image_data(image_number, image_data, stride);
}

int32_t get_image_info(int32_t handle, uint32_t image_number, tiff::SingleImageInfo* image_info)
{
	CHECK_TIFF_BUFFER(image_info);
	CHECK_TIFF_HANDLE(handle);
	return vecSingleTiff[handle]->get_image_info(image_number, image_info);
}

int32_t load_image_data(int32_t handle, uint32_t image_number, void* image_data, uint32_t stride)
{
	CHECK_TIFF_BUFFER(image_data);
	CHECK_TIFF_HANDLE(handle);
	return vecSingleTiff[handle]->load_image_data(image_number, image_data, stride);
}

int32_t set_image_tag(int32_t handle, uint32_t image_number, uint16_t tag_id, tiff::TiffTagDataType tag_type, uint32_t tag_count, void* tag_value)
{
	CHECK_TIFF_BUFFER(tag_value);
	if (tag_count == 0) return ErrorCode::ERR_BUFFER_SIZE_ERROR;
	CHECK_TIFF_HANDLE(handle);
	return vecSingleTiff[handle]->set_image_tag(image_number, tag_id, (uint16_t)tag_type, tag_count, tag_value);
}

int32_t get_image_tag(int32_t handle, uint32_t image_number, uint16_t tag_id, uint32_t tag_size, void* tag_value)
{
	CHECK_TIFF_BUFFER(tag_value);
	if (tag_size == 0) return ErrorCode::ERR_BUFFER_SIZE_ERROR;
	CHECK_TIFF_HANDLE(handle);
	return vecSingleTiff[handle]->get_image_tag(image_number, tag_id, tag_size, tag_value);
}

int32_t get_image_count(int32_t handle, uint32_t* image_count)
{
	CHECK_TIFF_BUFFER(image_count);
	CHECK_TIFF_HANDLE(handle);
	return vecSingleTiff[handle]->get_image_count(image_count);
}
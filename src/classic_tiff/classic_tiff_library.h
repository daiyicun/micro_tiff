#pragma once
#ifdef CLASSIC_TIFF_LIBRARY_EXPORTS
#define CLASSIC_TIFF_LIBRARY_API extern "C" __declspec(dllexport)
#else
#define CLASSIC_TIFF_LIBRARY_API extern "C" __declspec(dllimport)
#endif
#include "classic_def.h"
/**********************************************************************************
Important Tips:

<1> image_number start with 0.
<1> In this library, "Stride" means data's byte width(width * byte_count).

**********************************************************************************/

/**
 * @brief		Open a CLASSIC-TIFF file for read or/and write.
 *
 * @param[in]	file_name		Full path with file name. e.g: c:\\test.tif.
 * @param[in]	mode			Create mode, read mode or read write mode.
 *
 * @return		File handle or Error code.
 *  @retval		>=0	Handle of Tiff file.
 *  @retval		<0	Error code defines by "ErrorCode" in "error.h".
*/
CLASSIC_TIFF_LIBRARY_API int32_t open_tiff(const wchar_t* file_name, tiff::OpenMode mode);

/**
 * @brief		Close an opened CLASSIC-TIFF file.
 *
 * @param[in]	handle			The handle of an opened CLASSIC-TIFF file.
 *
 * @return		Status code defines by "ErrorCode" in "error.h".
 *
 * @note		Some work will be done when closing CLASSIC-TIFF file. Don't forget to call this when finish saving.
*/
CLASSIC_TIFF_LIBRARY_API int32_t close_tiff(int32_t handle);

/**
 * @brief		Add a new frame for saving image.
 *
 * @param[in]	handle			The handle of an opened CLASSIC-TIFF file.
 * @param[in]	image_info		The image information which will be saved in this frame.
 *
 * @return		Image frame number or Error code.
 *  @retval		>=0	Image frame number.
 *  @retval		<0	Error code defines by "ErrorCode" in "error.h".
 *
 * @note		You must call this before "save_image_data".
 *				A tiff file can hold many frame of images, these image can have different info.
*/
CLASSIC_TIFF_LIBRARY_API int32_t create_image(int32_t handle, tiff::SingleImageInfo image_info);

/**
 * @brief		Save image data into an exist frame.
 *
 * @param[in]	handle			The handle of an opened CLASSIC-TIFF file.
 * @param[in]	image_number	The image frame number which you want to save data in.
 * @param[in]	image_data		The buffer of image data to be stored.
 * @param[in]	stride			The buffer stride of "image_data". Default parameter "0" means stride is equal with tile's byte size of width.
 *
 * @return		Status code defines by "ErrorCode" in "error.h".
 *
 * @note		You must call this after "create_image" to make sure the image_number is exist.
*/
CLASSIC_TIFF_LIBRARY_API int32_t save_image_data(int32_t handle, uint32_t image_number, void* image_data, uint32_t stride = 0);

/**
 * @brief		Get image frame count of an opened CLASSIC-TIFF file.
 *
 * @param[in]	handle			The handle of an opened CLASSIC-TIFF file.
 * @param[out]	image_count		A pointer to number of image count.
 *
 * @return		Status code defines by "ErrorCode" in "error.h".
*/
CLASSIC_TIFF_LIBRARY_API int32_t get_image_count(int32_t handle, uint32_t* image_count);

/**
 * @brief		Get image information with specified image number.
 *
 * @param[in]	handle			The handle of an opened CLASSIC-TIFF file.
 * @param[in]	image_number	The image frame number which you want to get information.
 * @param[out]	image_info		A pointer to image information.
 *
 * @return		Status code defines by "ErrorCode" in "error.h".
 *
 * @note		You must call this after "get_image_count" to make sure the image_number is exist.
*/
CLASSIC_TIFF_LIBRARY_API int32_t get_image_info(int32_t handle, uint32_t image_number, tiff::SingleImageInfo* image_info);

/**
 * @brief		Get image data with specified image number.
 *
 * @param[in]	handle			The handle of an opened CLASSIC-TIFF file.
 * @param[in]	image_number	The image frame number which you want to get data.
 * @param[out]	image_data		The buffer of image data to be loaded.
 * @param[in]	stride			The buffer stride of "image_data". Default parameter "0" means stride is equal with tile's byte size of width.
 *
 * @return		Status code defines by "ErrorCode" in "error.h".
 *
 * @note		You must call this after "get_image_count" to make sure the image_number is exist.
*/
CLASSIC_TIFF_LIBRARY_API int32_t load_image_data(int32_t handle, uint32_t image_number, void* image_data, uint32_t stride = 0);

/**
 * @brief		Set tag with specified image number.
 *
 * @param[in]	handle			The handle of an opened CLASSIC-TIFF file.
 * @param[in]	image_number	The image frame number which you want to set tag.
 * @param[in]	tag_id			The id of tag Which you want to set value.
 * @param[in]	tag_type		Element type of "tag_value".
 * @param[in]	tag_count		Element count of "tag_value".
 * @param[in]	tag_value		A pointer to the value need to stored in with "tag_id".
 *
 * @return		Status code defines by "ErrorCode" in "error.h".
 *
 * @note		You must call this after "create_image" to make sure the image_number is exist.
*/
CLASSIC_TIFF_LIBRARY_API int32_t set_image_tag(int32_t handle, uint32_t image_number, uint16_t tag_id, tiff::TiffTagDataType tag_type, uint32_t tag_count, void* tag_value);

/**
 * @brief		Get tag value with specified image number.
 *
 * @param[in]	handle			The handle of an opened CLASSIC-TIFF file.
 * @param[in]	image_number	The image frame number which you want to get tag.
 * @param[in]	tag_id			The id of tag Which you want to get value.
 * @param[in]	tag_size		How many memory space have allocated for the "tag_value"(size in byte).
 * @param[out]	tag_value		The buffer used to carry the data of specified "tag_id".
 *
 * @return		Status code defines by "ErrorCode" in "error.h".
 *
 * @note		You must call this after "get_image_count" to make sure the image_number is exist.
*/
CLASSIC_TIFF_LIBRARY_API int32_t get_image_tag(int32_t handle, uint32_t image_number, uint16_t tag_id, uint32_t tag_size, void* tag_value);
#pragma once
#ifdef OME_TIFF_LIBRARY_EXPORTS
#define OME_TIFF_LIBRARY_API extern "C" __declspec(dllexport)
#else
#define OME_TIFF_LIBRARY_API extern "C" __declspec(dllimport)
#endif
#include "ome_def.h"
/**********************************************************************************
Important Tips:

<1> Don't forget using namespace "ome".
<2> In this library, "Stride" means data's byte width(width * byte_count).

**********************************************************************************/

/**
 * @brief		Open an ome-tiff file for read or write.
 * 
 * @param[in] file_name				Full path of the ome-tiff file. e.g: c:\\test.tif
 * @param[in] mode					Create, read write or read only mode
 * @param[in] cm					Data compression mode
 * 
 * @return		Handle or Error code.
 *  @retval		>=0 Handle of ome-tiff file.
 *  @retval		<0 Error code defines by "ErrorCode" in "ome.def.h".
 * 
 * @note		"cm" is useful in write or create mode and only for raw data saved by function ome_save_tile_data.
 *				It need more CPU resource if you choose any compression mode. You should take care about that.
 *				In some extreme case, you can not save disk space when you choose an compression method such as LZW.
 */
OME_TIFF_LIBRARY_API int32_t ome_open_file(const wchar_t* file_name, ome::OpenMode mode, 
	ome::CompressionMode cm = ome::CompressionMode::COMPRESSIONMODE_NONE);

/**
 * @brief		Close an opened ome-tiff file.
 * 
 * @param[in] handle				Handle of an opened ome-tiff file.
 * 
 * @return		Error code defines by "ErrorCode" in "ome.def.h".
 * 
 * @note		Some work will be done when close TIFF file. Don't forget to call it when finish saving.
 */
OME_TIFF_LIBRARY_API int32_t ome_close_file(int32_t handle);

/**
 * @brief		Add plate info to an opened ome-tiff file.
 * @details		PlateInfo tells the total size of the experiment and it include one or more Well inside.
 *				The complete meaning of Plates info you can see in the definition of it in "ome_def.h".
 * 
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[in] plate_info			Plate information which need to be set.
 * 
 * @return		Error code defines by "ErrorCode" in "ome.def.h".
 * 
* @note		Only can be set for once.
 *			Only useful in create mode.
 *			Must be set before save image with "ome_save_tile_data".
 */
OME_TIFF_LIBRARY_API int32_t ome_add_plate(int32_t handle, ome::PlateInfo plate_info);

/**
 * @brief		Get all plates information with opened ome-tiff file.
 * 
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[out] plates_info			PlateInfo array to carry all plates information.
 * 
 * @return		Error code defines by "ErrorCode" in "ome.def.h".
 * 
 * @note		You must call "ome_get_plates_num" to get count of plates, then alloc array of PlateInfo to carry all plates information.
 */
OME_TIFF_LIBRARY_API int32_t ome_get_plates(int32_t handle, ome::PlateInfo* plates_info);

/**
 * @brief		Get count of plates with opened ome-tiff file.
 * 
 * @param[in] handle				Handle of an opened ome-tiff file.
 * 
 * @return		Count or Error code.
 *  @retval		>=0 Count of plates.
 *  @retval		<0 Error code defines by "ErrorCode" in "ome.def.h".	
 */
OME_TIFF_LIBRARY_API int32_t ome_get_plates_num(int32_t handle);

/**
 * @brief		Add well information to an opened ome-tiff file with specific plate id.
 * 
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[in] plate_id				Which plate you want add well information into.
 * @param[in] well_info				Well information need to be set.
 * 
 * @return		Error code defines by "ErrorCode" in "ome.def.h".
 */
OME_TIFF_LIBRARY_API int32_t ome_add_well(int32_t handle, uint32_t plate_id, ome::WellInfo well_info);

/**
 * @brief		Get all wells information with opened ome-tiff file and specific plate id.
 * 
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[in] plate_id				Which plate you want get wells information.
 * @param[out] wells_info			WellInfo array to carry all wells information.
 * 
 * @return		Error code defines by "ErrorCode" in "ome.def.h".
 * 
 * @note		You must call "ome_get_wells_num" to get count of wells, then alloc array of WellInfo to carry all wells information.
 */
OME_TIFF_LIBRARY_API int32_t ome_get_wells(int32_t handle, uint32_t plate_id, ome::WellInfo* wells_info);

/**
 * @brief		Get count of wells with opened ome-tiff file and specific plate id.
 *
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[in] plate_id				Which plate you want get wells information.
 *
 * @return		Count or Error code.
 *  @retval		>=0 Count of wells.
 *  @retval		<0 Error code defines by "ErrorCode" in "ome.def.h".
 */
OME_TIFF_LIBRARY_API int32_t ome_get_wells_num(int32_t handle, uint32_t plate_id);

/**
 * @brief		Add scan information to an opened ome-tiff file with specific plate id.
 * 
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[in] plate_id				Which plate you want add scan information into.
 * @param[in] scan_info				Scan information need to be set.
 * 
 * @return		Error code defines by "ErrorCode" in "ome.def.h".
 */
OME_TIFF_LIBRARY_API int32_t ome_add_scan(int32_t handle, uint32_t plate_id, ome::ScanInfo scan_info);

/**
 * @brief		Get all scans information with opened ome-tiff file and specific plate id.
 * 
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[in] plate_id				Which plate you want get scans information.
 * @param[out] scans_info			ScanInfo array to carry all scans information.
 * 
 * @return		Error code defines by "ErrorCode" in "ome.def.h".
 * 
 * @note		You must call "ome_get_scans_num" to get count of scans, then alloc array of ScanInfo to carry all wells information.
 */
OME_TIFF_LIBRARY_API int32_t ome_get_scans(int32_t handle, uint32_t plate_id, ome::ScanInfo* scans_info);

/**
 * @brief		Get count of scans with opened ome-tiff file and specific plate id.
 * 
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[in] plate_id				Which plate you want get scans information.
 * 
 * @return		Count or Error code.
 *  @retval		>=0 Count of scans.
 *  @retval		<0 Error code defines by "ErrorCode" in "ome.def.h".
 */
OME_TIFF_LIBRARY_API int32_t ome_get_scans_num(int32_t handle, uint32_t plate_id);

/**
 * @brief		Add channel information to an opened ome-tiff file with specific plate id and specific scan id.
 * 
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[in] plate_id				Which plate you want add channel information into.
 * @param[in] scan_id				Which scan you want add channel information into.
 * @param[in] channel_info			Channel information need to be set.
 * 
 * @return		Error code defines by "ErrorCode" in "ome.def.h".
 */
OME_TIFF_LIBRARY_API int32_t ome_add_channel(int32_t handle, uint32_t plate_id, uint32_t scan_id, ome::ChannelInfo channel_info);

/**
 * @brief		Get all channels information with opened ome-tiff file and specific plate id and specific scan id.
 * 
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[in] plate_id				Which plate you want get channels information.
 * @param[in] scan_id				Which scan you want get channels information.
 * @param[out] channels_info		ChannelInfo array to carry all channels information.
 * 
 * @return		Error code defines by "ErrorCode" in "ome.def.h".
 * 
 * @note		You must call "ome_get_channels_num" to get count of channels, then alloc array of ChannelInfo to carry all channels information.
 */
OME_TIFF_LIBRARY_API int32_t ome_get_channels(int32_t handle, uint32_t plate_id, uint32_t scan_id, ome::ChannelInfo* channels_info);

/**
 * @brief		Get count of channels with opened ome-tiff file and specific plate id and specific scan id.
 * 
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[in] plate_id				Which plate you want get channels information.
 * @param[in] scan_id				Which scan you want get channels information.
 * 
 * @return		Count or Error code.
 *  @retval		>=0 Count of channels.
 *  @retval		<0 Error code defines by "ErrorCode" in "ome.def.h".
 */
OME_TIFF_LIBRARY_API int32_t ome_get_channels_num(int32_t handle, uint32_t plate_id, uint32_t scan_id);

/**
 * @brief		Remove channel from an opened ome-tiff file with specific plate_id and scan_id.
 * 
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[in] plate_id				Which plate you want remove channel from.
 * @param[in] scan_id				Which scan you want remove channel from.
 * @param[in] channel_id			Id of channel which you want to remove.
 * 
 * @return		Error code defines by "ErrorCode" in "ome.def.h".
 */
OME_TIFF_LIBRARY_API int32_t ome_remove_channel(int32_t handle, uint32_t plate_id, uint32_t scan_id, uint32_t channel_id);

/**
 * @brief		Add scan region information to an opened ome-tiff file with specific plate id and specific scan id and specific well_id.
 * 
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[in] plate_id				Which plate you want add scan region information into.
 * @param[in] scan_id				Which scan you want add scan region information into.
 * @param[in] well_id				Which well you want add scan region information into.
 * @param[in] scan_region_info		Scan region information need to be set.
 * 
 * @return		Error code defines by "ErrorCode" in "ome.def.h".
 */
OME_TIFF_LIBRARY_API int32_t ome_add_scan_region(int32_t handle, uint32_t plate_id, uint32_t scan_id, uint32_t well_id, ome::ScanRegionInfo scan_region_info);

/**
 * @brief		Get all scan regions information with opened ome-tiff file and specific plate id and specific scan id and specific well_id.
 * 
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[in] plate_id				Which plate you want get scan regions information.
 * @param[in] scan_id				Which scan you want get scan regions information.
 * @param[in] well_id				Which well you want get scan regions information.
 * @param[out] scan_regions_info	ScanRegionInfo array to carry all scan regions information.
 * 
 * @return		Error code defines by "ErrorCode" in "ome.def.h".
 * 
 * @note		You must call "ome_get_scan_regions_num" to get count of scan regions, then alloc array of ScanRegionInfo to carry all scan regions information.
 */
OME_TIFF_LIBRARY_API int32_t ome_get_scan_regions(int32_t handle, uint32_t plate_id, uint32_t scan_id, uint32_t well_id, ome::ScanRegionInfo* scan_regions_info);

/**
 * @brief		Get count of scan regions with opened ome-tiff file and specific plate id and specific scan id and specific well id.
 * 
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[in] plate_id				Which plate you want get scan regions information.
 * @param[in] scan_id				Which scan you want get scan regions information.
 * @param[in] well_id				Which well you want get scan regions information.
 * 
 * @return		Count or Error code.
 *  @retval		>=0 Count of scan regions.
 *  @retval		<0 Error code defines by "ErrorCode" in "ome.def.h".
 */
OME_TIFF_LIBRARY_API int32_t ome_get_scan_regions_num(int32_t handle, uint32_t plate_id, uint32_t scan_id, uint32_t well_id);

/**
 * @brief		Save raw data to Tiff.
 * @details		Tile size is defined by "tile_pixel_size_width" and "tile_pixel_size_height" in Scan.
 *				In tiff file,a large image is not stored by a whole file stream but separate in several tiles.
 *				You need to assign the index of row and column when you save it.
 *
 *				For example, you want to save a 512*512 image and you hope the tile_pixel_size_width and tile_pixel_size_height are 256.
 *				In this case,you need to call ome_save_tile_data 4 times.The "tile_row" and "tile_column" set to (0,0) (0,1) (1,0) (1,1)
 *				in four different calls and the buffer "image_data" carry a different 256*256 image every-time.
 *
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[in] image_data			The buffer which carry the image.Its size should equal or less than tile-width* tile-height.
 * @param[in] frame					The information of current image.It means which scan,region,channel,z_position,stream_series,time_series it is.
 *									OME-TIFF contains region,channel,z_position,time_series to describe an microscopy experiment.
 *									You can find more information in https://www-legacy.openmicroscopy.org/.
 * @param[in] row					The row index located.Please see * @brief to get its mean.
 * @param[in] column				The column index located.Please see * @brief to get its mean.
 * @param[in] stride				The buffer stride of "image_data". Default parameter "0" means stride is equal with tile's byte size of width.
 *
 * @return		Error code defines by "ErrorCode" in "ome.def.h".
 *
 * @note		Only save a tile's data once.
 *				Open file, set plates and add scan before save tile. If you want to save image easier, you can set "tile_pixel_size_width" and
 *				"tile_pixel_size_height" in Scan same as image width and height.Then you can save a image once by set "row" and "column" with 0.
 *				It could affect performance when you read a small area in a large image. You can do it like this if you don't care about that.
 *				Take care about the "pixel_type".
*/
OME_TIFF_LIBRARY_API int32_t ome_save_tile_data(int32_t handle, void* image_data, ome::FrameInfo frame, uint32_t row, uint32_t column, uint32_t stride = 0);

/**
 * @brief		When all the tile data of a frame are saved, you must call this function to write its ifd.
 *
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[in] frame					The information of current image.
 *
 * @return		Error code defines by "ErrorCode" in "ome.def.h".
 *
 * @note		If you call this function before all tile data saved, you can not save remaining tile data.
 *				Carefully call this function, if you never call this function, image data also saved after you call "ome_close_file".
*/
OME_TIFF_LIBRARY_API int32_t ome_purge_frame(int32_t handle, ome::FrameInfo frame);

/**
 * @brief		Load raw data from OME-TIFF file.
 *
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[in] frame					The information of current image.
 * @param[in] src_rect				The area you want to get in an image.
 * @param[out] image_data			Buffer to get the specific area of an image. You need to initialize the buffer before use the function.
 * @param[in] stride				The buffer stride of "image_data". Default parameter "0" means stride is equal with "src_rect" 's byte size of width.
 *
 * @return		Error code defines by "ErrorCode" in "ome.def.h".
 *
 * @note		Raw data is stored in the file like "test_s0c0.tid".
*/
OME_TIFF_LIBRARY_API int32_t ome_get_raw_data(int32_t handle, ome::FrameInfo frame, ome::OmeRect src_rect, void* image_data, uint32_t stride = 0);

/**
 * @brief		Load raw data from OME-TIFF file with specific row index and column index.
 * @detail		Raw data is separate by tile width and tile height, use this function to get specific tile data.
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[in] frame					The information of current image.
 * @param[in] row					The row index of image.
 * @param[in] column				The column index of image.
 * @param[in] image_data			Buffer to get the specific area of an image. You need to initialize the buffer before use the function.
 * @param[in] stride				The buffer stride of "image_data". Default parameter "0" means stride is equal with "src_rect" 's byte size of width.
 * 
 * @return		Error code defines by "ErrorCode" in "ome.def.h".
 * 
 * @note		Raw data is stored in the file like "test_s0c0.tid".
 */
OME_TIFF_LIBRARY_API int32_t ome_get_raw_tile_data(int32_t handle, ome::FrameInfo frame, uint32_t row, uint32_t column, void* image_data, uint32_t stride = 0);

/**
 * @brief		Get data of tag with specific frame saved in tiff file.
 *
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[in] frame					The information of current image.\n
 									if "plate_id" in "frame" is UINT32_MAX(0xffffffff), this function will get tag from header file.
 * @param[in] tag_id				Tag id, which tag you want to get its value.
 * @param[in,out] tag_size			If the "tag_value" is null pointer, this is a output parameter, indicate byte size of "tag_value";\n
									Else, this is a input parameter, indicate memory space you have allocated for the "tag_value"(size in byte).
 * @param[out] tag_value			Buffer of tag value will be stored with tag_id.
 *
 * @return		Error code defines by "ErrorCode" in "ome.def.h".
*/
OME_TIFF_LIBRARY_API int32_t ome_get_tag(int32_t handle, ome::FrameInfo frame, uint16_t tag_id, uint32_t* tag_size, void* tag_value);

/**
 * @brief		Set data of tag to specific frame in tiff file.
 *
 * @param[in] handle				Handle of an opened ome-tiff file.
 * @param[in] frame					The information of current image.\n
									if "plate_id" in "frame" is UINT32_MAX(0xffffffff), this function will set tag to header file.
 * @param[in] tag_id				Tag id, which tag you want to store value within.
 * @param[in] tag_type				Element type of "tag_value".
 * @param[in] tag_count				Element count of "tag_value".
 * @param[in] tag_value				Buffer of tag value stored with tag_id.
 *
 * @return		Error code defines by "ErrorCode" in "ome.def.h".
*/
OME_TIFF_LIBRARY_API int32_t ome_set_tag(int32_t handle, ome::FrameInfo frame, uint16_t tag_id, ome::TiffTagDataType tag_type, uint32_t tag_count, void* tag_value);

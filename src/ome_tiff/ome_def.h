#pragma once
#ifdef OME_TIFF_LIBRARY_EXPORTS
#define OME_TIFF_LIBRARY_CLASS __declspec(dllexport)
#else
#define OME_TIFF_LIBRARY_CLASS __declspec(dllimport)
#endif

#include <stdint.h>

/**
* Explain
* Plate			:	This is a physical concept that designates an experimental area.
* Well			:	This is also a physical concept, it specifies the physical dimensions of the test specimen and its relative physical location in Plate.
					Every WellSample must be included in well, every Well can have more than 1 WellSample.
* Scan			:	This is a fictional concept that you can interpret it as an operation.
					Scan must have ScanRegions(1 or more), through the Scan's own pixel_physical_size_* and ScanRegion's size_pixel_* to calculate the physical size of these ScanRegions.
					Each ScanRegion corresponds to a unique WellSample, and the location of this WellSample is the physical location of the ScanRegion.
* ScanRegion	:	This is a pixel concept, it specifies the pixel size of the scan image.
* WellSample	:	This is a physical concept, it has a one-to-one correspondence with ScanRegion, and it specifies the starting physical location of ScanRegion.
*
* Imagine a specific scenario: you have a lab table (this is Plate),
  on the table there are several slides (these are Well),
  each slide contains one or more samples to be photographed for observation (these are WellSample),
  the camera to photograph these samples is ScanRegion,
  we can photograph multiple samples at the same time in one operation, this operation we call Scan.
*
* Example
* <Plate ID="Plate:1" Name="4 Slide Carrier" Width="115" Height="70" PhysicalSizeXUnit="mm" PhysicalSizeYUnit="mm" Rows="1" Columns="1">
	  <Well ID="Well:1.2" PositionX="7" PositionY="5" Width="20" Height="60" Shape="Rectangle" Row="0" Column="0">
		  <WellSample ID="WellSample:1.2.1" PositionX="8619" PositionY="9828" PositionZ="0" PositionXUnit="µm" PositionYUnit="µm" PositionZUnit="µm">
			  <ImageRef ID="Image:1"/>
		  </WellSample>
	  </Well>
	  <PlateAcquisition ID="PlateAcquisition:1.3">
		  <WellSampleRef ID="WellSample:1.2.1" RegionID="0"/>
	  </PlateAcquisition>
  </Plate>
  <Image Name="1" ID="Image:1">
	  <Pixels ID="Pixels:0" DimensionOrder="XYZTC" PhysicalSizeX="20.000" PhysicalSizeY="21.013" PhysicalSizeZ="1.000" PhysicalSizeXUnit="µm" PhysicalSizeYUnit="µm" PhysicalSizeZUnit="µm" TimeIncrement="1" TimeIncrementUnit="s" Type="uint16" SizeC="4" SizeT="1" SizeX="850" SizeY="1346" SizeZ="1" TileWidth="256" TileHeight="512" SignificantBits="14">
		  <Channel ID="Channel:0" Name="Green" SamplesPerPixel="1"/>
		  <TiffData FirstC="0" FirstT="0" FirstZ="0" IFD="0">
			  <UUID FileName="11-Animal 4 Lean T1 RevB-07-17-2015_scan_1.tif"/>
		  </TiffData>
	  </Pixels>
  </Image>
********* The above xml will eventually be written to the boot file, but it is not created by the user.
		  What the user needs to do is to create objects like Plate, Scan, etc., and then write the image data.
		  When the file is closed, this xml will be automatically created by the library and written to the boot file. *********
* How to generate?
* 1. Open file with create mode : 
		int32_t hdl = ome_open_file(L"custom_file_full_path.tif", ome::OpenMode::CREATE_MODE, ome::CompressionMode::COMPRESSIONMODE_LZW);
* NOTE : We only support 1 plate now.
* 2. Add Plate :
		ome::PlateInfo plate_info = { 0 };
		plate_info.id = 1;		//ID in Plate
		plate_info.width = 115;
		plate_info.height = 70;
		plate_info.physicalsize_unit_x = DistanceUnit::DISTANCE_MILLIMETER;
		plate_info.physicalsize_unit_y = DistanceUnit::DISTANCE_MILLIMETER;
		plate_info.row_size = 1;
		plate_info.column_size = 1;
		const wchar_t* plate_name = L"4 Slide Carrier";
		wmemcpy_s(plate_info.name, NAME_LEN, plate_name, wcslen(plate_name));
		ome_add_plate(hdl, plate_info);
* 3. Add Well :
		ome::WellInfo well1_info = { 0 };
		well1_info.id = 2;	//Why 2 ? see Well ID, separate with colon(:), first number indicate plate id, second number indicate well id.
		well1_info.position_x = 7;		//PositionX in Well
		well1_info.position_y = 5;
		well1_info.width = 20;
		well1_info.height = 60;
		well1_info.well_shape = Shape::SHAPE_RECTANGLE;
		well1_info.row_index = 0;
		well1_info.column_index = 0;
		//Why well has no unit_x / unit_y?  It share the same unit as plate.
		ome_add_well(hdl, plate_info.id, well_info);
* 4. Add Scan :
		ome::ScanInfo scan_info = { 0 };
		scan_info.id = 3;	//why 3 ? see WellSample ID, separate with colon(:), first number indicate plate id, second number indicate well id, third number indicate the index.
		scan_info.pixel_physical_size_x = 20.000f;		//PhysicalSizeX in Pixels.
		scan_info.pixel_physical_size_y = 21.013f;		//PhysicalSizeY in Pixels
		scan_info.pixel_physical_size_z = 1.000f;		//PhysicalSizeZ in Pixels
		scan_info.pixel_physical_uint_x = DistanceUnit::DISTANCE_MICROMETER;	//PhysicalSizeXUnit in Pixels
		scan_info.pixel_physical_uint_y = DistanceUnit::DISTANCE_MICROMETER;	//PhysicalSizeYUnit in Pixels
		scan_info.pixel_physical_uint_z = DistanceUnit::DISTANCE_MICROMETER;	//PhysicalSizeZUnit in Pixels
		scan_info.time_increment = 1f;		//TimeIncrement in Pixels
		scan_info.time_increment_unit = TimeUnit::TIME_SECOND;		//TimeIncrementUnit in Pixels
		scan_info.tile_pixel_size_width = 256;		//TileWidth in Pixels
		scan_info.tile_pixel_size_height = 512;		//TileHeight in Pixels
		scan_info.significant_bits = 14;			//SignificantBits in Pixels
		scan_info.pixel_type = PixelType::PIXEL_UINT16;		//Type in Pixels
		//we only support dimension_order "XYZTC" now. This parameter can be missing, the default dimension_order is "XYZTC".
		const char* dimension_order = "XYZTC";
		memcpy_s(scan_info.dimension_order, NAME_LEN, dimension_order, strlen(dimension_order));
		ome_add_scan(hdl, plate_info.id, scan_info);
* 5. Add Channels :
		ome::ChannelInfo channel_info = { 0 };
		channel_info.id = 0;	//why 0? see Channel ID, separate with colon(:), first number indicate the channel id.
		channel_info.sample_per_pixel = 1;
		const wchar_t* channel_name = L"Green";
		wmemcpy_s(channel_info.name, NAME_LEN, channel_name, wcslen(channel_name));
		ome_add_channel(hdl, plate_info.id, scan_info.id, channel_info);
* 6. Add ScanRegion :
		ome::ScanRegionInfo scan_region_info = { 0 };
		scan_region_info.id = 0;	//why 0? see RegionID is WellSampleRef.
		scan_region_info.start_physical_x = 8619f;	//why 8619? see PositionX in WellSample.
		scan_region_info.start_physical_y = 9828f;	//PositionY in WellSample.
		scan_region_info.start_physical_z = 0f;		//PositionZ in WellSample
		scan_region_info.start_unit_x = DistanceUnit::DISTANCE_MICROMETER;
		scan_region_info.start_unit_y = DistanceUnit::DISTANCE_MICROMETER;
		scan_region_info.start_unit_z = DistanceUnit::DISTANCE_MICROMETER;
		scan_region_info.pixel_size_x = 850;		//why 850 see SizeX in Pixels;
		scan_region_info.pixel_size_y = 1346;		//SizeY in Pixels
		scan_region_info.pixel_size_z = 1;			//SizeZ in Pixels
		ome_add_scan_region(hdl, plate_info.id, scan_info.id, well_info.id, scan_region_info);
		//size_t of ome::ScanRegionInfo no need to be set, it will auto calculate when save data.
* 10. You may want to know how to generate the TiffData element?
		When you call ome_save_tile_data() and ome_close_file(), this library will generate it for you.
**/

namespace ome
{
	#define NAME_LEN 64

	enum ErrorCode {
		STATUS_OK = 0,

		TIFF_ERR_USELESS_HDL = -1,
		TIFF_ERR_READ_DATA_FROM_FILE_FAILED = -2,
		TIFF_ERR_OPEN_FILE_PARAMETER_ERROR = -3,
		TIFF_ERR_OPEN_FILE_FAILED = -4,
		TIFF_ERR_CLOSE_FILE_FAILED = -5,
		TIFF_ERR_NO_TIFF_FORMAT = -6,
		TIFF_ERR_BLOCK_SIZE_IN_IFD_IS_EMPTY = -7,
		TIFF_ERR_NEED_IFD_NOT_FOUND = -8,
		TIFF_ERR_BLOCK_OUT_OF_RANGE = -9,
		TIFF_ERR_NO_IFD_FOUND = -10,
		TIFF_ERR_FIRST_IFD_STRUCT_NOT_COMPLETE = -11,
		TIFF_ERR_BLOCK_OFFSET_OUT_OF_RANGE = -12,
		TIFF_ERR_TAG_NOT_FOUND = -13,
		TIFF_ERR_BAD_PARAMETER_VALUE = -14,
		TIFF_ERR_IFD_ALREADY_EXIST = -15,
		TIFF_ERR_IFD_OUT_OF_DIRECTORY = -16,
		TIFF_ERR_IFD_FULL = -17,
		TIFF_ERR_UNSUPPORTED_BITS_PER_SAMPLE = -18,
		TIFF_ERR_PREVIOUS_IFD_NOT_CLOSED = -19,
		TIFF_ERR_TAG_TYPE_INCORRECT = -20,
		TIFF_ERR_TAG_SIZE_INCORRECT = -21,
		TIFF_ERR_SEEK_FILE_POINTER_FAILED = -22,
		TIFF_ERR_ALLOC_MEMORY_FAILED = -23,
		TIFF_ERR_WRONG_OPEN_MODE = -24,
		TIFF_ERR_APPEND_TAG_NOT_ALLOWED = -25,
		TIFF_ERR_DUPLICATE_WRITE_NOT_ALLOWED = -26,
		TIFF_ERR_BIGENDIAN_NOT_SUPPORT = -27,

		ERR_FILE_PATH_ERROR = -101,
		ERR_HANDLE_NOT_EXIST = -102,
		ERR_PLATE_ID_EXIST = -103,
		ERR_BUFFER_IS_NULL = -105,
		ERR_PLATE_NOT_EXIST = -106,
		ERR_SCAN_NOT_EXIST = -107,
		ERR_MODIFY_NOT_ALLOWED = -108,
		ERR_READ_OME_XML_FAILED = -109,
		ERR_DECOMPRESS_JPEG_FAILED = -110,
		ERR_SCAN_ID_EXIST = -112,
		ERR_ROW_OUT_OF_RANGE = -114,
		ERR_COLUMN_OUT_OF_RANGE = -115,
		ERR_STRIDE_NOT_CORRECT = -116,
		ERR_COMPRESS_TYPE_NOTSUPPORT = -118,
		ERR_COMPRESS_LZW_FAILED = -119,
		ERR_DECOMPRESS_LZW_FAILED = -120,
		ERR_DECOMPRESS_LZ4_FAILED = -121,
		ERR_DECOMPRESS_ZLIB_FAILED = -122,
		ERR_TAG_CONDITION_NOT_MET = -123,
		ERR_P2D_SHIFT_FAILED = -124,
		ERR_P2D_RESIZE_FAILED = -125,
		ERR_CHANNEL_ID_EXIST = -126,
		ERR_XML_PARSE_FAILED = -127,
		ERR_ONLY_SUPPORT_ONE_PLATE = -128,
		ERR_WELL_NOT_EXIST = -129,
		ERR_WELL_SAMPLE_NOT_EXIST = -130,
		ERR_LZW_HORIZONTAL_DIFFERENCING = -131,
		ERR_NO_CHANNELS = -132,
		ERR_CANNOT_FIND_IMAGE = -133,
		ERR_NO_WELL_SAMPLE_MATCHS_REGION_ID = -134,
		ERR_CANNOT_FIND_TIFF_DATA_WITH_ZTC = -135,
		ERR_ADD_TIFF_DATA_REPEATE = -136,
		ERR_WELL_ID_EXIST = -137,
		ERR_SCAN_REGION_ID_EXIST = -138,
		ERR_NO_IMAGE_REF_IN_WELL_SAMPLE = -139,
		ERR_PLATE_ID_NOT_MATCHED = -140,
		ERR_PLATE_ACQUISITION_ID_EXIST = -141,
		ERR_IMAGE_ID_EXIST = -142,
		ERR_NO_PIXELS_IN_IMAGE = -143,
		ERR_SIGNIFICATION_BITS = -144,
		ERR_JEPG_ONLY_SUPPORT_8BITS = -145,
		ERR_RESIZE_NOT_SUPPORT_NOW = -146,
		ERR_BLOCK_SIZE_NOT_MATCHED = -147,
		ERR_TIFF_DATA_HAS_NO_UUID = -148,
		ERR_WELL_SAMPLE_REF_ID_EMPTY = -149,
		ERR_TIFF_DATA_CANNOT_PREDICT = -150,
		ERR_WELL_SAMPLE_ID_EXIST = -151,
		ERR_PARAMETER_INVALID = -152,
		ERR_SCANF_ASSIGNED_ERROR = -153,
		ERR_CHANNEL_NOT_EXIST = -154,
		ERR_REGION_NOT_EXIST = -155,
		ERR_PLATE_ACQUISITION_NOT_EXIST = -156,
		ERR_HORIZONTAL_ACC_FAILED = -157,
		ERR_OME_XML_SIZE_ZERO = -158,		
		ERR_PLATE_COLUMN_SIZE = -159,
		ERR_PLATE_ROW_SIZE = -160,
		ERR_PLATE_PHYSICAL_SIZE = -161,
		ERR_WELL_PHYSICAL_SIZE = -162,
		ERR_SCAN_TILE_SIZE = -163,
		ERR_CHANNEL_BIN_SIZE = -164,
		ERR_CHANNEL_SAMPLES_PER_PIXEL = -165,
		ERR_BOOT_IFD_ALREADY_EXIST = -166,
	};

	enum class DistanceUnit {
		DISTANCE_UNDEFINED = -1,
		DISTANCE_KILOMETER = 1,
		DISTANCE_METER = 2,
		DISTANCE_MILLIMETER = 3,
		DISTANCE_MICROMETER = 4,
		DISTANCE_NANOMETER = 5,
		DISTANCE_PICOMETER = 6,
	};

	enum class Shape {
		SHAPE_UNDEFINED = -1,
		SHAPE_RECTANGLE = 1,
		SHAPE_ELLIPSE = 2,
	};

	enum class TimeUnit {
		TIME_UNDEFINED = -1,
		TIME_SECOND = 1,
		TIME_MILLISECOND = 2,
		TIME_MICROSECOND = 3,
		TIME_NANOSECOND = 4,
		TIME_PICOSECOND = 5,
		TIME_FEMTOSECOND = 6,
	};

	enum class TiffTagDataType {
		TIFF_NOTYPE = 0,      /* placeholder */
		TIFF_BYTE = 1,        /* 8-bit unsigned integer */
		TIFF_ASCII = 2,       /* 8-bit bytes w/ last byte null */
		TIFF_SHORT = 3,       /* 16-bit unsigned integer */
		TIFF_LONG = 4,        /* 32-bit unsigned integer */
		TIFF_RATIONAL = 5,    /* 64-bit unsigned fraction */
		TIFF_SBYTE = 6,       /* !8-bit signed integer */
		TIFF_UNDEFINED = 7,   /* !8-bit untyped data */
		TIFF_SSHORT = 8,      /* !16-bit signed integer */
		TIFF_SLONG = 9,       /* !32-bit signed integer */
		TIFF_SRATIONAL = 10,  /* !64-bit signed fraction */
		TIFF_FLOAT = 11,      /* !32-bit IEEE floating point */
		TIFF_DOUBLE = 12,     /* !64-bit IEEE floating point */
		TIFF_IFD = 13,        /* %32-bit unsigned integer (offset) */
		TIFF_LONG8 = 16,      /* BigTIFF 64-bit unsigned integer */
		TIFF_SLONG8 = 17,     /* BigTIFF 64-bit signed integer */
		TIFF_IFD8 = 18        /* BigTIFF 64-bit unsigned integer (offset) */
	};

	enum class PixelType {
		PIXEL_UNDEFINED = -1,
		PIXEL_INT8 = 0,
		PIXEL_UINT8 = 1,
		PIXEL_UINT16 = 2,
		PIXEL_INT16 = 3,
		PIXEL_FLOAT32 = 4,
	};

	enum class OpenMode {
		CREATE_MODE = 1,
		READ_WRITE_MODE = 2,
		READ_ONLY_MODE = 3,
	};

	enum class CompressionMode {
		COMPRESSIONMODE_NONE = 0,
		//COMPRESSIONMODE_LZ4 = 1,
		COMPRESSIONMODE_LZW = 2,
		//COMPRESSIONMODE_JPEG = 3,
		//COMPRESSIONMODE_ZIP = 4,
	};

	////User can define any custom tag id between (CustomTag_First, CustomTag_Last), CustomTag_First and CustomTag_Last are not valid tag id.
	//enum class CustomTag
	//{
	//	CustomTag_First = 10600,
	//	CustomTag_Last = 10699,
	//};

	struct OmeRect {
		uint32_t x;
		uint32_t y;
		uint32_t width;
		uint32_t height;
	};

	struct OmeSize {
		uint32_t width;
		uint32_t height;
	};

	struct WellInfo
	{
		float position_x;
		float position_y;
		float width;
		float height;
		uint32_t id;
		uint16_t row_index;
		uint16_t column_index;
		Shape well_shape;
	};

	struct PlateInfo
	{
		float width;
		float height;
		uint32_t id;
		uint16_t row_size;
		uint16_t column_size;
		DistanceUnit physicalsize_unit_x;
		DistanceUnit physicalsize_unit_y;
		wchar_t name[NAME_LEN];
	};

	struct ChannelInfo
	{
		uint32_t id;
		uint32_t samples_per_pixel;
		uint32_t bin_size;
		wchar_t name[NAME_LEN];
	};

	struct ScanRegionInfo
	{
		uint32_t id;
		uint32_t pixel_size_x;
		uint32_t pixel_size_y;
		uint32_t pixel_size_z;
		uint32_t size_t;

		float start_physical_x;
		float start_physical_y;
		float start_physical_z;
		DistanceUnit start_unit_x;
		DistanceUnit start_unit_y;
		DistanceUnit start_unit_z;
	};

	struct ScanInfo
	{
		uint32_t id;

		float pixel_physical_size_x;            // A pixel's width
		float pixel_physical_size_y;            // A pixel's height
		float pixel_physical_size_z;            // Z step length
		float time_increment;                   // Time series' increment

		DistanceUnit pixel_physical_uint_x;     // Unit of "pixel_physical_size_x"
		DistanceUnit pixel_physical_uint_y;     // Unit of "pixel_physical_size_y"
		DistanceUnit pixel_physical_uint_z;     // Unit of "pixel_physical_size_z"
		TimeUnit time_increment_unit;           // Unit of "time_increment"

		uint32_t tile_pixel_size_width;         // Raw data's horizontal pixel count in a tile
		uint32_t tile_pixel_size_height;        // Raw data's vertical pixel count in a tile
		uint32_t significant_bits;              // Actual bits the image used

		PixelType pixel_type;                   // A pixel's type
		char dimension_order[NAME_LEN];        // Now only support "XYZTC", user defined value will be overwritten.
	};

	struct FrameInfo
	{
		uint32_t plate_id;   // After add_plate, the plate_id is useful.
		uint32_t scan_id;    // After add_scan, the scan_id is useful.
		uint32_t region_id;  // After add_scan_region, the region_id is useful.
		uint32_t c_id;       // After add_channel, the c_id is useful.
		uint32_t z_id;       // must start from 0
		uint32_t t_id;       // must start from 0
	};
}

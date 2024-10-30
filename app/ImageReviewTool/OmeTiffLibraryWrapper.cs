using System.Runtime.InteropServices;

namespace ImageReviewTool
{
    public class OmeTiffLibraryWrapper
    {
        public const int NAME_LEN = 64;

        public enum ErrorCode
        {
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
            ERR_BUFFER_SIZE_ERROR = -104,
            ERR_BUFFER_IS_NULL = -105,
            ERR_PLATE_NOT_EXIST = -106,
            ERR_SCAN_NOT_EXIST = -107,
            ERR_MODIFY_NOT_ALLOWED = -108,
            ERR_READ_OME_XML_FAILED = -109,
            ERR_DECOMPRESS_JPEG_FAILED = -110,
            ERR_SAVE_BLOCK_FAILED = -111,
            ERR_SCAN_ID_EXIST = -112,
            ERR_GET_OMEXML_FAILED = -113,
            ERR_ROW_OUT_OF_RANGE = -114,
            ERR_COLUMN_OUT_OF_RANGE = -115,
            ERR_STRIDE_NOT_CORRECT = -116,
            ERR_DATA_TYPE_NOTSUPPORT = -117,
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
        };

        public enum DistanceUnit
        {
            DISTANCE_UNDEFINED = -1,
            DISTANCE_KILOMETER = 1,
            DISTANCE_METER = 2,
            DISTANCE_MILLIMETER = 3,
            DISTANCE_MICROMETER = 4,
            DISTANCE_NANOMETER = 5,
            DISTANCE_PICOMETER = 6,
        };

        public enum Shape
        {
            SHAPE_UNDEFINED = -1,
            SHAPE_RECTANGLE = 1,
            SHAPE_ELLIPSE = 2,
        };

        public enum TimeUnit
        {
            TIME_UNDEFINED = -1,
            TIME_SECOND = 1,
            TIME_MILLISECOND = 2,
            TIME_MICROSECOND = 3,
            TIME_NANOSECOND = 4,
            TIME_PICOSECOND = 5,
            TIME_FEMTOSECOND = 6,
        };

        public enum TiffTagDataType
        {
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

        public enum PixelType
        {
            PIXEL_UNDEFINED = -1,
            PIXEL_INT8 = 0,
            PIXEL_UINT8 = 1,
            PIXEL_UINT16 = 2,
            PIXEL_INT16 = 3,
            PIXEL_FLOAT32 = 4,
        };

        public enum OpenMode
        {
            CREATE_MODE = 1,
            READ_WRITE_MODE = 2,
            READ_ONLY_MODE = 3,
        };

        public enum CompressionMode
        {
            COMPRESSIONMODE_NONE = 0,
            //COMPRESSIONMODE_LZ4 = 1,
            COMPRESSIONMODE_LZW = 2,
            //COMPRESSIONMODE_JPEG = 3,
            //COMPRESSIONMODE_ZIP = 4,
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct OmeRect
        {
            public uint x;
            public uint y;
            public uint width;
            public uint height;
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct OmeSize
        {
            public uint width;
            public uint height;
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct WellInfo
        {
            private float position_x;
            private float position_y;
            private float width;
            private float height;
            private uint id;
            private ushort row_index;
            private ushort column_index;
            private Shape well_shape;

            //properties
            public float PositionX { get => position_x; set => position_x = value; }
            public float PositionY { get => position_y; set => position_y = value; }
            public float Width { get => width; set => width = value; }
            public float Height { get => height; set => height = value; }
            public uint Id { get => id; set => id = value; }
            public ushort RowIndex { get => row_index; set => row_index = value; }
            public ushort ColumnIndex { get => column_index; set => column_index = value; }
            public Shape WellShape { get => well_shape; set => well_shape = value; }
        };

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        public struct PlateInfo
        {
            private float width;
            private float height;
            private uint id;
            private ushort row_size;
            private ushort column_size;
            private DistanceUnit physicalsize_unit_x;
            private DistanceUnit physicalsize_unit_y;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = NAME_LEN)]
            private string name;

            //properties
            public float Width { get => width; set => width = value; }
            public float Height { get => height; set => height = value; }
            public uint Id { get => id; set => id = value; }
            public ushort RowSize { get => row_size; set => row_size = value; }
            public ushort ColumnSize { get => column_size; set => column_size = value; }
            public DistanceUnit PhysicalSizeUnitX { get => physicalsize_unit_x; set => physicalsize_unit_x = value; }
            public DistanceUnit PhysicalSizeUnitY { get => physicalsize_unit_y; set => physicalsize_unit_y = value; }
            public string Name { get => name; set => name = value; }
        };

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        public struct ChannelInfo
        {
            private uint id;
            private uint samples_per_pixel;
            private uint bin_size;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = NAME_LEN)]
            private string name;

            //properties
            public uint Id { get => id; set => id = value; }
            public uint SamplesPerPixel { get => samples_per_pixel; set => samples_per_pixel = value; }
            public uint BinSize { get => bin_size; set => bin_size = value; }
            public string Name { get => name; set => name = value; }
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct ScanRegionInfo
        {
            private uint id;
            private uint pixel_size_x;
            private uint pixel_size_y;
            private uint pixel_size_z;
            private uint size_t;

            private float start_physical_x;
            private float start_physical_y;
            private float start_physical_z;
            private DistanceUnit start_unit_x;
            private DistanceUnit start_unit_y;
            private DistanceUnit start_unit_z;

            //properties
            public uint Id { get => id; set => id = value; }
            public uint PixelSizeX { get => pixel_size_x; set => pixel_size_x = value; }
            public uint PixelSizeY { get => pixel_size_y; set => pixel_size_y = value; }
            public uint PixelSizeZ { get => pixel_size_z; set => pixel_size_z = value; }
            public uint SizeT { get => size_t; set => size_t = value; }
            public float StartPhysicalX { get => start_physical_x; set => start_physical_x = value; }
            public float StartPhysicalY { get => start_physical_y; set => start_physical_y = value; }
            public float StartPhysicalZ { get => start_physical_z; set => start_physical_z = value; }
            public DistanceUnit StartUnitX { get => start_unit_x; set => start_unit_x = value; }
            public DistanceUnit StartUnitY { get => start_unit_y; set => start_unit_y = value; }
            public DistanceUnit StartUnitZ { get => start_unit_z; set => start_unit_z = value; }
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct ScanInfo
        {
            private uint id;

            private float pixel_physical_size_x;            // A pixel's width
            private float pixel_physical_size_y;            // A pixel's height
            private float pixel_physical_size_z;            // Z step length
            private float time_increment;                   // Time series' increment

            private DistanceUnit pixel_physical_uint_x;     // Unit of "pixel_physical_size_x"
            private DistanceUnit pixel_physical_uint_y;     // Unit of "pixel_physical_size_y"
            private DistanceUnit pixel_physical_uint_z;     // Unit of "pixel_physical_size_z"
            private TimeUnit time_increment_unit;           // Unit of "time_increment"

            private uint tile_pixel_size_width;             // Raw data's horizontal pixel count in a tile
            private uint tile_pixel_size_height;            // Raw data's vertical pixel count in a tile
            private uint significant_bits;                  // Actual bits the image used

            private PixelType pixel_type;                   // A pixel's type
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = NAME_LEN)]
            private string dimension_order;                // Now only support "XYZTC", user defined value will be overwritten.

            //properties
            public uint Id { get => id; set => id = value; }
            public float PixelPhysicalSizeX { get => pixel_physical_size_x; set => pixel_physical_size_x = value; }
            public float PixelPhysicalSizeY { get => pixel_physical_size_y; set => pixel_physical_size_y = value; }
            public float PixelPhysicalSizeZ { get => pixel_physical_size_z; set => pixel_physical_size_z = value; }
            public float TimeIncrement { get => time_increment; set => time_increment = value; }
            public DistanceUnit PixelPhysicalUnitX { get => pixel_physical_uint_x; set => pixel_physical_uint_x = value; }
            public DistanceUnit PixelPhysicalUnitY { get => pixel_physical_uint_y; set => pixel_physical_uint_y = value; }
            public DistanceUnit PixelPhysicalUnitZ { get => pixel_physical_uint_z; set => pixel_physical_uint_z = value; }
            public TimeUnit TimeIncrementUnit { get => time_increment_unit; set => time_increment_unit = value; }
            public uint TilePixelSizeWidth { get => tile_pixel_size_width; set => tile_pixel_size_width = value; }
            public uint TilePixelSizeHeight { get => tile_pixel_size_height; set => tile_pixel_size_height = value; }
            public uint SignificatBits { get => significant_bits; set => significant_bits = value; }
            public PixelType PixelType { get => pixel_type; set => pixel_type = value; }
            public string DimensionOrder { get => dimension_order; set => dimension_order = value; }
        };

        [StructLayout(LayoutKind.Sequential)]
        public struct FrameInfo
        {
            public uint plate_id;   // After add_plate, the plate_id is useful.
            public uint scan_id;    // After add_scan, the scan_id is useful.
            public uint region_id;  // After add_scan_region, the region_id is useful.
            public uint c_id;       // After add_channel, the c_id is useful.
            public uint z_id;       // must start from 0
            public uint t_id;       // must start from 0
        };

#if DEBUG
        private const string OmeTiffLibraryPath = "ome_tiff_libraryd.dll";
#else
        private const string OmeTiffLibraryPath = "ome_tiff_library.dll";
#endif

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_open_file", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern int ome_open_file(string file_name, OpenMode mode, CompressionMode cm = CompressionMode.COMPRESSIONMODE_NONE);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_close_file", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ome_close_file(int handle);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_add_plate", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ome_add_plate(int handle, PlateInfo plate_info);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_get_plates", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ome_get_plates(int handle, [In, Out] PlateInfo[] plate_infos);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_get_plates_num", CallingConvention = CallingConvention.Cdecl)]
        public static extern uint ome_get_plates_num(int handle);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_add_well", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ome_add_well(int handle, uint plate_id, WellInfo well_info);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_get_wells", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ome_get_wells(int handle, uint plate_id, [In, Out] WellInfo[] well_infos);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_get_wells_num", CallingConvention = CallingConvention.Cdecl)]
        public static extern uint ome_get_wells_num(int handle, uint plate_id);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_add_scan", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ome_add_scan(int handle, uint plate_id, ScanInfo scan_info);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_get_scans", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ome_get_scans(int handle, uint plate_id, [In, Out] ScanInfo[] scan_infos);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_get_scans_num", CallingConvention = CallingConvention.Cdecl)]
        public static extern uint ome_get_scans_num(int handle, uint plate_id);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_add_channel", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ome_add_channel(int handle, uint plate_id, uint scan_id, ChannelInfo channel_info);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_get_channels", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ome_get_channels(int handle, uint plate_id, uint scan_id, [In, Out] ChannelInfo[] channel_infos);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_get_channels_num", CallingConvention = CallingConvention.Cdecl)]
        public static extern uint ome_get_channels_num(int handle, uint plate_id, uint scan_id);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_remove_channel", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ome_remove_channel(int handle, uint plate_id, uint scan_id, uint channel_id);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_add_scan_region", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ome_add_scan_region(int handle, uint plate_id, uint scan_id, uint well_id, ScanRegionInfo scan_region_info);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_get_scan_regions", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ome_get_scan_regions(int handle, uint plate_id, uint scan_id, uint well_id, [In, Out] ScanRegionInfo[] scan_region_infos);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_get_scan_regions_num", CallingConvention = CallingConvention.Cdecl)]
        public static extern uint ome_get_scan_regions_num(int handle, uint plate_id, uint scan_id, uint well_id);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_save_tile_data", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ome_save_tile_data(int handle, IntPtr image_data, FrameInfo frame_info, uint row, uint column, uint stride = 0);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_purge_frame", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ome_purge_frame(int handle, FrameInfo frame_info);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_get_raw_data", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ome_get_raw_data(int handle, FrameInfo frame_info, OmeRect src_rect, IntPtr image_data, uint stride = 0);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_get_raw_tile_data", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ome_get_raw_tile_data(int handle, FrameInfo frame_info, uint row, uint column, IntPtr image_data, uint stride = 0);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_get_tag", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ome_get_tag(int handle, FrameInfo frame_info, ushort tag_id, ref TiffTagDataType tag_type, ref uint tag_count, IntPtr tag_value);

        [DllImport(OmeTiffLibraryPath, EntryPoint = "ome_set_tag", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ome_set_tag(int handle, FrameInfo frame_info, ushort tag_id, TiffTagDataType tag_type, uint tag_count, IntPtr tag_value);
    }
}

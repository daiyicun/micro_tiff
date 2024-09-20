using ImageReviewTool.ImageTool.Materials;
using ImageReviewTool.ImageTool.Tools;
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace ImageReviewTool
{
    public class FlimData
    {
        public int Index { get; }
        public int Data { get; }
        public SolidColorBrush BackgroundBrush { get; }
        public SolidColorBrush ForegroundBrush { get; }
        public FlimData(int index, int data)
        {
            Index = index;
            Data = data;
            if (data == 0)
            {
                ForegroundBrush = new SolidColorBrush(Colors.Black);
                BackgroundBrush = new SolidColorBrush(Colors.White);
            }
            else
            {
                ForegroundBrush = new SolidColorBrush(Colors.White);
                BackgroundBrush = new SolidColorBrush(Colors.Red);
            }
        }

        public override string ToString()
        {
            return $"{Index} : {Data}";
        }
    }

    internal class ImageData
    {
        internal byte[] OriginalData { get; set; } = null;
        internal Rect Rect { get; }
        internal int SamplesPerPixel { get; }
        internal OmeTiffLibraryWrapper.PixelType PixelType { get; }
        internal int DataBytes { get; }
        internal ImageData(Rect rect, int samplesPerPixel, OmeTiffLibraryWrapper.PixelType pixelType)
        {
            Rect = rect;
            SamplesPerPixel = samplesPerPixel;
            PixelType = pixelType;
            DataBytes = 1;
            switch (pixelType)
            {
                case OmeTiffLibraryWrapper.PixelType.PIXEL_UNDEFINED:
                case OmeTiffLibraryWrapper.PixelType.PIXEL_INT8:
                case OmeTiffLibraryWrapper.PixelType.PIXEL_UINT8:
                {
                    DataBytes = 1;
                    break;
                }
                case OmeTiffLibraryWrapper.PixelType.PIXEL_UINT16:                    
                case OmeTiffLibraryWrapper.PixelType.PIXEL_INT16:
                    DataBytes = 2;
                    break;
                case OmeTiffLibraryWrapper.PixelType.PIXEL_FLOAT32:
                    DataBytes = 4;
                    break;
            }
        }
    }

    internal class FrameDetail
    {
        internal OmeTiffLibraryWrapper.ScanInfo ScanInfo { get; set; }
        internal OmeTiffLibraryWrapper.ChannelInfo ChannelInfo { get; set; }
        internal OmeTiffLibraryWrapper.ScanRegionInfo ScanRegionInfo { get; set; }
        internal OmeTiffLibraryWrapper.FrameInfo FrameInfo { get; set; }
    }

    public class ScanDetail : BindableBase
    {
        public event EventHandler LoadImageDataEvent;

        public OmeTiffLibraryWrapper.ScanInfo BaseInfo { get; }

        public ScanDetail(OmeTiffLibraryWrapper.ScanInfo baseInfo)
        {
            BaseInfo = baseInfo;
        }

        private readonly List<OmeTiffLibraryWrapper.ChannelInfo> _channelInfos = new();
        public IList<OmeTiffLibraryWrapper.ChannelInfo> ChannelInfos => _channelInfos;

        private OmeTiffLibraryWrapper.ChannelInfo? _selectedChannel;
        public OmeTiffLibraryWrapper.ChannelInfo? SelectedChannel
        {
            get => _selectedChannel;
            set
            {
                if (SetProperty(ref _selectedChannel, value))
                    LoadImageDataEvent?.Invoke(this, EventArgs.Empty);
            }
        }

        private readonly List<OmeTiffLibraryWrapper.ScanRegionInfo> _scanRegionInfos = new();
        public IList<OmeTiffLibraryWrapper.ScanRegionInfo> ScanRegionInfos => _scanRegionInfos;

        private OmeTiffLibraryWrapper.ScanRegionInfo? _selectedScanRegion;
        public OmeTiffLibraryWrapper.ScanRegionInfo? SelectedScanRegion
        {
            get => _selectedScanRegion;
            set
            {
                if (SetProperty(ref _selectedScanRegion, value))
                {
                    _zIndex = Math.Min(_zIndex, _selectedScanRegion.Value.PixelSizeZ - 1);
                    OnPropertyChanged(nameof(ZIndex));
                    _tIndex = Math.Min(_tIndex, _selectedScanRegion.Value.SizeT - 1);
                    OnPropertyChanged(nameof(TIndex));
                    LoadImageDataEvent?.Invoke(this, EventArgs.Empty);
                }
            }
        }

        private uint _zIndex = 0;
        public uint ZIndex
        {
            get => _zIndex;
            set
            {
                if (SelectedScanRegion == null)
                    return;
                var target = Math.Min(value, _selectedScanRegion.Value.PixelSizeZ - 1);
                if (SetProperty(ref _zIndex, target))
                    LoadImageDataEvent?.Invoke(this, EventArgs.Empty);
            }
        }

        private uint _tIndex = 0;
        public uint TIndex
        {
            get => _tIndex;
            set
            {
                if (SelectedScanRegion == null)
                    return;
                var target = Math.Min(value, _selectedScanRegion.Value.SizeT - 1);
                if (SetProperty(ref _tIndex, target))
                    LoadImageDataEvent?.Invoke(this, EventArgs.Empty);
            }
        }
    }

    public class PlateDetail : BindableBase
    {
        public event EventHandler LoadImageDataEvent;

        public OmeTiffLibraryWrapper.PlateInfo BaseInfo { get; }

        public PlateDetail(OmeTiffLibraryWrapper.PlateInfo baseInfo)
        {
            BaseInfo = baseInfo;
        }

        private readonly List<OmeTiffLibraryWrapper.WellInfo> _wellInfos = new();
        public IList<OmeTiffLibraryWrapper.WellInfo> WellInfos => _wellInfos;

        private readonly List<ScanDetail> _scanDetails = new();
        public IList<ScanDetail> ScanDetails => _scanDetails;

        private OmeTiffLibraryWrapper.WellInfo? _selectedWell;
        public OmeTiffLibraryWrapper.WellInfo? SelectedWell
        {
            get => _selectedWell;
            set => SetProperty(ref _selectedWell, value);
        }

        private ScanDetail _selectedScan = null;
        public ScanDetail SelectedScan
        {
            get => _selectedScan;
            set
            {
                var beforeScan = _selectedScan;
                if (SetProperty(ref _selectedScan, value))
                {
                    if (beforeScan != null)
                        beforeScan.LoadImageDataEvent -= Scan_LoadImageDataEvent;
                    if (_selectedScan != null)
                        _selectedScan.LoadImageDataEvent += Scan_LoadImageDataEvent;
                }
            }
        }

        private void Scan_LoadImageDataEvent(object sender, EventArgs e)
        {
            LoadImageDataEvent?.Invoke(this, EventArgs.Empty);
        }
    }

    public class MainWindowViewModel : BindableBase
    {
        private int _fileHandle = -1;

        public bool IsValid => _fileHandle > -1 && Plates.Count > 0;

        private bool _isOpening = false;
        public bool IsOpening
        {
            get => _isOpening;
            set => SetProperty(ref _isOpening, value);
        }

        private bool _isShiftTo8Bits = true;
        public bool IsShiftTo8Bits
        {
            get => _isShiftTo8Bits;
            set
            {
                if (SetProperty(ref _isShiftTo8Bits, value) && IsValid)
                {
                    Plate_LoadImageDataEvent(SelectedPlate, EventArgs.Empty);
                }
            }
        }

        private Size? _currentImageSize = null;

        private int _pixelPointX;
        public int PixelPointX
        {
            get => _pixelPointX;
            set
            {
                if (_currentImageSize == null)
                    return;
                var target = (int)Math.Clamp(value, 0, _currentImageSize.Value.Width - 1);
                if (SetProperty(ref _pixelPointX, target))
                {
                    UpdateFlimData(new Point(_pixelPointX, _pixelPointY));
                }
            }
        }

        private int _pixelPointY;
        public int PixelPointY
        {
            get => _pixelPointY;
            set
            {
                if (_currentImageSize == null)
                    return;
                var target = (int)Math.Clamp(value, 0, _currentImageSize.Value.Height - 1);
                if (SetProperty(ref _pixelPointY, target))
                {
                    UpdateFlimData(new Point(_pixelPointX, _pixelPointY));
                }
            }
        }

        public Point? PhysicalPoint { get; private set; }

        public bool IsPointInImage => PhysicalPoint != null;


        public ToolBase CurrentTool { get; } = new ToolDragger();
        public ImageTool.Coordinate Coordinate { get; } = new ImageTool.Coordinate();

        private readonly ObservableCollection<ImageViewModel> _images = new();
        public ObservableCollection<ImageViewModel> Images => _images;

        private readonly List<ImageData> _datas = new List<ImageData>();

        private readonly ObservableCollection<FlimData> _flimData = new();
        public ObservableCollection<FlimData> FlimData => _flimData;

        public int Intensity { get; private set; }

        private readonly ObservableCollection<PlateDetail> _plates = new();
        public ObservableCollection<PlateDetail> Plates => _plates;

        private PlateDetail _selectedPlate = null;
        public PlateDetail SelectedPlate
        {
            get => _selectedPlate;
            set
            {
                var beforePlate = _selectedPlate;
                if (SetProperty(ref _selectedPlate, value))
                {
                    if (beforePlate != null)
                        beforePlate.LoadImageDataEvent -= Plate_LoadImageDataEvent;
                    if (_selectedPlate != null)
                        _selectedPlate.LoadImageDataEvent += Plate_LoadImageDataEvent;
                }
            }
        }

        private readonly System.Collections.Concurrent.ConcurrentQueue<FrameDetail> _frameInfoQueue = new();

        public MainWindowViewModel()
        {
            (CurrentTool as ToolDragger).ViewPixelAtPoint += UpdateFlimData;
            UpdateImage();
        }

        private void UpdateFlimData(Point pixelPoint)
        {
            FlimData.Clear();
            Intensity = 0;
            var target = _datas.Find(data => data.Rect.Contains(pixelPoint.X, pixelPoint.Y));
            if (target != null && target.OriginalData != null)
            {
                _pixelPointX = (int)pixelPoint.X;
                _pixelPointY = (int)pixelPoint.Y;
                var physical = Coordinate.GetPhysicalPointFromScreen(pixelPoint);
                PhysicalPoint = new Point(Math.Round(physical.X, 3), Math.Round(physical.Y, 3));

                var offsetX = (int)(_pixelPointX - target.Rect.X);
                var offsetY = (int)(_pixelPointY - target.Rect.Y);

                var stride = (int)target.Rect.Width * target.SamplesPerPixel * target.DataBytes;
                var dataOffset = stride * offsetY + offsetX * target.SamplesPerPixel;
                for (int i = 0; i < target.SamplesPerPixel; i++)
                {
                    int valueTemp = 0;
                    if (target.PixelType == OmeTiffLibraryWrapper.PixelType.PIXEL_UINT16)
                    {
                        valueTemp = BitConverter.ToUInt16(target.OriginalData, dataOffset + i * target.DataBytes);
                    }
                    if (target.PixelType == OmeTiffLibraryWrapper.PixelType.PIXEL_UINT8)
                    {
                        valueTemp = target.OriginalData[dataOffset + i];
                    }
                    Intensity += valueTemp;
                    FlimData.Add(new FlimData(i, valueTemp));
                }
            }
            else
            {
                PhysicalPoint = null;
            }
            OnPropertyChanged(nameof(PixelPointX));
            OnPropertyChanged(nameof(PixelPointY));
            OnPropertyChanged(nameof(PhysicalPoint));
            OnPropertyChanged(nameof(IsPointInImage));
            OnPropertyChanged(nameof(Intensity));
        }

        private static double GetMultiplyWithUnit(OmeTiffLibraryWrapper.DistanceUnit sourceUnit, OmeTiffLibraryWrapper.DistanceUnit destinationUnit = OmeTiffLibraryWrapper.DistanceUnit.DISTANCE_MICROMETER)
        {
            if (sourceUnit == destinationUnit)
                return 1;

            return Math.Pow(10d, (int)sourceUnit - (int)destinationUnit);
        }

        private bool _isLoading = false;
        private void UpdateImage()
        {
            Task.Run(() =>
            {
                while (true)
                {
                    if (!_isLoading && _frameInfoQueue.TryDequeue(out var frameDetail))
                    {
                        try
                        {
                            _isLoading = true;
                            var scanInfo = frameDetail.ScanInfo;
                            var scanRegionInfo = frameDetail.ScanRegionInfo;
                            var channelInfo = frameDetail.ChannelInfo;

                            var imageWidth = scanRegionInfo.PixelSizeX;
                            var imageHeight = scanRegionInfo.PixelSizeY;
                            var tileWidth = scanInfo.TilePixelSizeWidth;
                            var tileHeight = scanInfo.TilePixelSizeHeight;

                            var startMultiplyX = GetMultiplyWithUnit(scanRegionInfo.StartUnitX);
                            var startMultiplyY = GetMultiplyWithUnit(scanRegionInfo.StartUnitY);
                            var widthMultiply = GetMultiplyWithUnit(scanInfo.PixelPhysicalUnitX);
                            var heightMultiply = GetMultiplyWithUnit(scanInfo.PixelPhysicalUnitY);

                            var size = new Size(imageWidth, imageHeight);
                            _currentImageSize = size;
                            var rect = new Rect
                            {
                                X = scanRegionInfo.StartPhysicalX * startMultiplyX,
                                Y = scanRegionInfo.StartPhysicalY * startMultiplyY,
                                Width = imageWidth * scanInfo.PixelPhysicalSizeX * widthMultiply,
                                Height = imageHeight * scanInfo.PixelPhysicalSizeY * heightMultiply
                            };

                            if (size != Coordinate.PixelSize)
                                Coordinate.PixelSize = size;
                            if (rect != Coordinate.PhysicalRect)
                                Coordinate.PhysicalRect = rect;

                            Application.Current.Dispatcher.Invoke(() => Coordinate.AutoFit());

                            var dataBytes = 1;
                            PixelFormat pixelFormat = PixelFormats.Gray8;
                            switch (scanInfo.PixelType)
                            {
                                case OmeTiffLibraryWrapper.PixelType.PIXEL_UNDEFINED:
                                case OmeTiffLibraryWrapper.PixelType.PIXEL_INT8:
                                    return;
                                case OmeTiffLibraryWrapper.PixelType.PIXEL_UINT8:
                                {
                                    dataBytes = 1;
                                    pixelFormat = PixelFormats.Gray8;
                                    break;
                                }
                                case OmeTiffLibraryWrapper.PixelType.PIXEL_UINT16:
                                    dataBytes = 2;
                                    pixelFormat = PixelFormats.Gray16;
                                    break;
                                case OmeTiffLibraryWrapper.PixelType.PIXEL_INT16:
                                case OmeTiffLibraryWrapper.PixelType.PIXEL_FLOAT32:
                                    return;
                            }

                            var columnCount = (int)Math.Ceiling((double)imageWidth / tileWidth);
                            var rowCount = (int)Math.Ceiling((double)imageHeight / tileHeight);

                            var multiTileWidth = tileWidth;
                            var multiTileHeight = tileHeight;
                            var totalCount = columnCount * rowCount;
                            //make the image count less than 200, to improve ImageCanvas performance
                            while (totalCount > 200)
                            {
                                if (columnCount > rowCount)
                                {
                                    multiTileWidth += tileWidth;
                                    columnCount = (int)Math.Ceiling((double)imageWidth / multiTileWidth);
                                }
                                else
                                {
                                    multiTileHeight += tileHeight;
                                    rowCount = (int)Math.Ceiling((double)imageHeight / multiTileHeight);
                                }
                                totalCount = columnCount * rowCount;
                            }

                            var bytesPerPixel = channelInfo.SamplesPerPixel * dataBytes;

                            _images.Clear();
                            _datas.Clear();

                            Parallel.For(0, totalCount, new ParallelOptions() { MaxDegreeOfParallelism = 5 }, index =>
                            {
                                var r = index / columnCount;
                                var c = index % columnCount;

                                var y = (int)(r * multiTileHeight);
                                var h = (int)(imageHeight - y >= multiTileHeight ? multiTileHeight : imageHeight - y);

                                var x = (int)(c * multiTileWidth);
                                var w = (int)(imageWidth - x >= multiTileWidth ? multiTileWidth : imageWidth - x);

                                int count = w * h;
                                int stride = w * dataBytes;

                                byte[] imageBuffer = new byte[count * bytesPerPixel];

                                OmeTiffLibraryWrapper.OmeRect omeRect = new()
                                {
                                    x = x,
                                    y = y,
                                    width = w,
                                    height = h,
                                };

                                unsafe
                                {
                                    fixed (byte* bufferPointer = imageBuffer)
                                    {
                                        var ptr = new IntPtr(bufferPointer);
                                        if (_fileHandle < 0)
                                            return;

                                        int status = OmeTiffLibraryWrapper.ome_get_raw_data(_fileHandle, frameDetail.FrameInfo, omeRect, ptr);
                                        if (status != 0)
                                            return;
                                    }
                                }

                                var imageRect = new Rect
                                {
                                    X = x,
                                    Y = y,
                                    Height = h,
                                    Width = w
                                };

                                var isFlimData = channelInfo.SamplesPerPixel > 1;
                                if (isFlimData)
                                {
                                    ImageData data = new(imageRect, (int)channelInfo.SamplesPerPixel, scanInfo.PixelType)
                                    {
                                        OriginalData = new byte[count * bytesPerPixel]
                                    };
                                    Array.Copy(imageBuffer, data.OriginalData, count * bytesPerPixel);
                                    _datas.Add(data);

                                    int maxValue = (1 << (dataBytes * 8)) - 1;
                                    for (int i = 0; i < count; i++)
                                    {
                                        if (scanInfo.PixelType == OmeTiffLibraryWrapper.PixelType.PIXEL_UINT16)
                                        {
                                            uint summary = 0;
                                            for (int j = 0; j < channelInfo.SamplesPerPixel; j++)
                                            {
                                                ushort data_i = BitConverter.ToUInt16(imageBuffer, (i * (int)channelInfo.SamplesPerPixel + j) * dataBytes);
                                                summary += data_i;
                                            }
                                            var clampValue = Math.Clamp(summary, 0, maxValue);
                                            var bytes = BitConverter.GetBytes((ushort)clampValue);
                                            imageBuffer[i * dataBytes] = bytes[0];
                                            imageBuffer[i * dataBytes + 1] = bytes[1];
                                        }
                                        else
                                        {
                                            uint summary = 0;
                                            for (int j = 0; j < channelInfo.SamplesPerPixel; j++)
                                            {
                                                summary += imageBuffer[(i * channelInfo.SamplesPerPixel + j) * dataBytes];
                                            }
                                            var clampValue = Math.Clamp(summary, 0, maxValue);
                                            imageBuffer[i] = (byte)clampValue;
                                        }
                                    }
                                }

                                if (_isShiftTo8Bits && scanInfo.PixelType == OmeTiffLibraryWrapper.PixelType.PIXEL_UINT16)
                                {
                                    for (int i = 0; i < count; i++)
                                    {
                                        ushort data_i = BitConverter.ToUInt16(imageBuffer, i * dataBytes);
                                        imageBuffer[i] = (byte)(data_i >> ((int)scanInfo.SignificatBits - 8));
                                    }

                                    pixelFormat = PixelFormats.Gray8;
                                    stride = w;
                                }

                                BitmapSource bitmap = BitmapSource.Create(w, h, 96, 96, pixelFormat, null, imageBuffer, stride);
                                _images.Add(new ImageViewModel { Image = bitmap, Rect = imageRect });
                            });

                            Application.Current.Dispatcher.Invoke(() => UpdateFlimData(new Point(_pixelPointX, _pixelPointY)));
                        }
                        finally
                        {
                            _isLoading = false;
                        }
                    }
                    else
                    {
                        Thread.Sleep(1);
                    }
                }
            });
        }

        private void Plate_LoadImageDataEvent(object sender, EventArgs e)
        {
            if (sender is PlateDetail plate && plate.SelectedScan?.SelectedScanRegion != null && plate.SelectedScan?.SelectedChannel != null)
            {
                var scan = plate.SelectedScan;
                var scanRegion = scan.SelectedScanRegion.Value;
                var channel = scan.SelectedChannel.Value;
                OmeTiffLibraryWrapper.FrameInfo frameInfo = new()
                {
                    plate_id = plate.BaseInfo.Id,
                    scan_id = scan.BaseInfo.Id,
                    region_id = scanRegion.Id,
                    c_id = channel.Id,
                    z_id = scan.ZIndex,
                    t_id = scan.TIndex,
                };

                var frameDetail = new FrameDetail
                {
                    ScanInfo = scan.BaseInfo,
                    ChannelInfo = channel,
                    ScanRegionInfo = scanRegion,
                    FrameInfo = frameInfo,
                };
                _frameInfoQueue.Clear();
                _frameInfoQueue.Enqueue(frameDetail);
            }
        }

        public RelayCommand LoadCommand => new(OnLoadFile);

        private void ClearAll()
        {
            OmeTiffLibraryWrapper.ome_close_file(_fileHandle);
            _fileHandle = -1;
            _images.Clear();
            _plates.Clear();
            SelectedPlate = null;
            _datas.Clear();
            _flimData.Clear();
            PhysicalPoint = null;
            Intensity = 0;
            OnPropertyChanged(nameof(Intensity));
            OnPropertyChanged(nameof(PhysicalPoint));
            OnPropertyChanged(nameof(IsPointInImage));
            OnPropertyChanged(nameof(IsValid));
        }

        private async void OnLoadFile(object obj)
        {
            ClearAll();
            Microsoft.Win32.OpenFileDialog tiffDialog = new() { Filter = "OME-Tiff|*.tif;*.btf;*.tiff", Multiselect = false };
            if (tiffDialog.ShowDialog() == true)
            {
                await ReadOmeTiff(tiffDialog.FileName);
            }
        }

        private Task ReadOmeTiff(string fullPath)
        {
            return Task.Run(() =>
            {
                IsOpening = true;
                int status = 0;
                do
                {
                    int hdl = OmeTiffLibraryWrapper.ome_open_file(fullPath, OmeTiffLibraryWrapper.OpenMode.READ_ONLY_MODE);
                    if (hdl < 0)
                    {
                        status = hdl;
                        break;
                    }

                    _fileHandle = hdl;

                    uint plate_size = OmeTiffLibraryWrapper.ome_get_plates_num(hdl);

                    OmeTiffLibraryWrapper.PlateInfo[] plate_infos = new OmeTiffLibraryWrapper.PlateInfo[plate_size];
                    status = OmeTiffLibraryWrapper.ome_get_plates(hdl, plate_infos);
                    if (status < 0)
                    {
                        break;
                    }

                    for (uint p = 0; p < plate_size; p++)
                    {
                        var plate_info = plate_infos[p];
                        PlateDetail plateDetail = new(plate_info);
                        Application.Current.Dispatcher.Invoke(() => _plates.Add(plateDetail));

                        uint well_size = OmeTiffLibraryWrapper.ome_get_wells_num(hdl, plate_info.Id);
                        OmeTiffLibraryWrapper.WellInfo[] well_infos = new OmeTiffLibraryWrapper.WellInfo[well_size];
                        status = OmeTiffLibraryWrapper.ome_get_wells(hdl, plate_info.Id, well_infos);
                        if (status < 0)
                        {
                            break;
                        }

                        uint scan_size = OmeTiffLibraryWrapper.ome_get_scans_num(hdl, plate_info.Id);
                        OmeTiffLibraryWrapper.ScanInfo[] scan_infos = new OmeTiffLibraryWrapper.ScanInfo[scan_size];
                        status = OmeTiffLibraryWrapper.ome_get_scans(hdl, plate_info.Id, scan_infos);
                        if (status < 0)
                        {
                            break;
                        }

                        for (uint i = 0; i < scan_size; i++)
                        {
                            var scan_info = scan_infos[i];
                            ScanDetail scanDetail = new(scan_info);
                            plateDetail.ScanDetails.Add(scanDetail);

                            uint channel_size = OmeTiffLibraryWrapper.ome_get_channels_num(hdl, plate_info.Id, scan_info.Id);
                            OmeTiffLibraryWrapper.ChannelInfo[] channel_infos = new OmeTiffLibraryWrapper.ChannelInfo[channel_size];
                            status = OmeTiffLibraryWrapper.ome_get_channels(hdl, plate_info.Id, scan_info.Id, channel_infos);
                            if (status < 0)
                            {
                                break;
                            }
                            foreach (var channel_info in channel_infos)
                            {
                                scanDetail.ChannelInfos.Add(channel_info);
                            }

                            for (uint w = 0; w < well_size; w++)
                            {
                                var well_info = well_infos[w];
                                plateDetail.WellInfos.Add(well_info);

                                uint scan_region_size = OmeTiffLibraryWrapper.ome_get_scan_regions_num(hdl, plate_info.Id, scan_info.Id, well_info.Id);
                                OmeTiffLibraryWrapper.ScanRegionInfo[] scan_region_infos = new OmeTiffLibraryWrapper.ScanRegionInfo[scan_region_size];
                                status = OmeTiffLibraryWrapper.ome_get_scan_regions(hdl, plate_info.Id, scan_info.Id, well_info.Id, scan_region_infos);
                                if (status < 0)
                                {
                                    break;
                                }

                                foreach (var scan_region in scan_region_infos)
                                {
                                    scanDetail.ScanRegionInfos.Add(scan_region);
                                }
                            }
                        }
                    }
                } while (false);
                if (status != 0)
                {
                    MessageBox.Show($"Failed to load ome-tiff with error code : {status}");
                }
                IsOpening = false;
                OnPropertyChanged(nameof(IsValid));
            });
        }
    }
}

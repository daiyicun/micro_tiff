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

    public class ImageViewModelEx : ImageViewModel
    {
        internal byte[] OriginalData { get; set; } = null;
        internal byte[] FlimSumData { get; set; } = null;
        internal int BinSize { get; }
        internal OmeTiffLibraryWrapper.PixelType PixelType { get; }
        internal int DataBytes { get; }

        internal ImageViewModelEx(Rect rect, int binSize, OmeTiffLibraryWrapper.PixelType pixelType)
        {
            Rect = rect;
            BinSize = binSize;
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

        internal bool Contains(Point point)
        {
            if (point.X >= Rect.X && point.X <= Rect.X + Rect.Width - 1 &&
                point.Y >= Rect.Y && point.Y <= Rect.Y + Rect.Height - 1)
                return true;
            return false;
        }
    }

    public enum ImageChangeType
    {
        Plate,
        Scan,
        Channel,
        Region,
        Z,
        T
    }


    internal class FrameDetail
    {
        internal OmeTiffLibraryWrapper.ScanInfo ScanInfo { get; set; }
        internal OmeTiffLibraryWrapper.ChannelInfo ChannelInfo { get; set; }
        internal OmeTiffLibraryWrapper.ScanRegionInfo ScanRegionInfo { get; set; }
        internal OmeTiffLibraryWrapper.FrameInfo FrameInfo { get; set; }
        internal bool IsFrameSizeOrTileChanged { get; set; }
    }

    public class ScanDetail : BindableBase
    {
        //EventArg indicate if the image's size or tile is changed
        public event EventHandler<ImageChangeType> LoadImageDataEvent;

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
                    LoadImageDataEvent?.Invoke(this, ImageChangeType.Channel);
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
                    LoadImageDataEvent?.Invoke(this, ImageChangeType.Region);
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
                    LoadImageDataEvent?.Invoke(this, ImageChangeType.Z);
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
                    LoadImageDataEvent?.Invoke(this, ImageChangeType.T);
            }
        }
    }

    public class PlateDetail : BindableBase
    {
        //EventArg indicate if the image's size or tile is changed
        public event EventHandler<ImageChangeType> LoadImageDataEvent;

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
                    {
                        _selectedScan.LoadImageDataEvent += Scan_LoadImageDataEvent;
                        var channel = _selectedScan.ChannelInfos.FirstOrDefault();
                        var region = _selectedScan.ScanRegionInfos.FirstOrDefault();
                        bool autoTriggerEvent = true;
                        if (Equals(_selectedScan.SelectedChannel, channel))
                            autoTriggerEvent = false;
                        _selectedScan.SelectedChannel = channel;

                        if (Equals(_selectedScan.SelectedScanRegion, region))
                            autoTriggerEvent = false;
                        _selectedScan.SelectedScanRegion = region;
                        if (!autoTriggerEvent)
                            LoadImageDataEvent?.Invoke(this, ImageChangeType.Scan);
                    }
                }
            }
        }

        private void Scan_LoadImageDataEvent(object sender, ImageChangeType type)
        {
            LoadImageDataEvent?.Invoke(this, type);
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

        private bool _isAutoBC = false;
        public bool IsAutoBC
        {
            get => _isAutoBC;
            set
            {
                if (SetProperty(ref _isAutoBC, value) && IsValid)
                {
                    if (_isAutoBC)
                    {
                        _minValue = ImageMinValue;
                        _maxValue = ImageMaxValue;
                        OnPropertyChanged(nameof(MinValue));
                        OnPropertyChanged(nameof(MaxValue));
                        UpdateImageSource(ImageMinValue, ImageMaxValue);
                    }
                    else
                        UpdateImageSource(null, null);
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

        public double ImageMinValue { get; private set; } = double.NaN;
        public double ImageMaxValue { get; private set; } = double.NaN;

        public double MaxLimit { get; private set; } = double.NaN;

        private bool _isLoading = false;
        public bool IsLoading
        {
            get => _isLoading;
            set => SetProperty(ref _isLoading, value);
        }

        private double _minValue = double.NaN;
        public double MinValue
        {
            get => _minValue;
            set
            {
                if (value < 0 || value > MaxLimit || value >= _maxValue )
                    return;
                var target = Math.Round(value);
                SetProperty(ref _minValue, target);
            }
        }

        private double _maxValue = double.NaN;
        public double MaxValue
        {
            get => _maxValue;
            set
            {
                if (value < 0 || value > MaxLimit || value <= _minValue)
                    return;
                var target = Math.Round(value);
                SetProperty(ref _maxValue, target);
            }
        }

        public bool IsFlimData { get; private set; }

        public Point? PhysicalPoint { get; private set; }

        public bool IsPointInImage => PhysicalPoint != null;

        public ToolBase CurrentTool { get; } = new ToolDragger();
        public ImageTool.Coordinate Coordinate { get; } = new ImageTool.Coordinate();

        private readonly ObservableCollection<ImageViewModelEx> _images = new();
        public ObservableCollection<ImageViewModelEx> Images => _images;

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
                if (beforePlate != null)
                    beforePlate.LoadImageDataEvent -= Plate_LoadImageDataEvent;
                if (value != null)
                    value.LoadImageDataEvent += Plate_LoadImageDataEvent;

                SetProperty(ref _selectedPlate, value);
            }
        }

        private readonly System.Collections.Concurrent.ConcurrentQueue<FrameDetail> _frameInfoQueue = new();

        public RelayCommand LoadCommand => new(OnLoadFile);
        public RelayCommand ApplyMinMaxCommand => new(OnApplyMinMax);

        public MainWindowViewModel()
        {
            (CurrentTool as ToolDragger).ViewPixelAtPoint += UpdateFlimData;
            UpdateImage();
        }

        private void OnApplyMinMax(object obj)
        {
            UpdateImageSource(MinValue, MaxValue);
        }

        private void UpdateFlimData(Point pixelPoint)
        {
            if (_selectedPlate == null ||
                _selectedPlate.SelectedScan == null ||
                _selectedPlate.SelectedScan.SelectedChannel == null)
                return;

            FlimData.Clear();
            Intensity = 0;
            var target = _images.FirstOrDefault(data => data.Contains(pixelPoint));
            if (target != null && target.OriginalData != null)
            {
                _pixelPointX = (int)pixelPoint.X;
                _pixelPointY = (int)pixelPoint.Y;
                var physical = Coordinate.GetPhysicalPointFromPixel(pixelPoint);
                PhysicalPoint = new Point(Math.Round(physical.X, 3), Math.Round(physical.Y, 3));

                var offsetX = (int)(_pixelPointX - target.Rect.X);
                var offsetY = (int)(_pixelPointY - target.Rect.Y);

                var stride = (int)target.Rect.Width * target.BinSize * target.DataBytes;
                var dataOffset = stride * offsetY + offsetX * target.BinSize * target.DataBytes;
                for (int i = 0; i < target.BinSize; i++)
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

        private CancellationTokenSource _cancelSource = new CancellationTokenSource();

        private void UpdateImageSource(double? min, double? max)
        {
            _cancelSource.Cancel();
            _cancelSource.Dispose();
            _cancelSource = new CancellationTokenSource();
            var token = _cancelSource.Token;
            if (min == null || max == null || (min.Value < ImageMinValue && max.Value > ImageMaxValue))
            {
                Task.Run(() =>
                {
                    Parallel.For(0, _images.Count, new ParallelOptions() { CancellationToken = token }, (index, state) =>
                    {
                        if (token.IsCancellationRequested)
                        {
                            state.Stop();
                            return;
                        }
                        var img = _images[index];
                        byte[] usedData = img.OriginalData;
                        if (img.BinSize > 1)
                        {
                            usedData = img.FlimSumData;
                        }

                        Application.Current?.Dispatcher.Invoke(() =>
                        {
                            var wbmp = (img.Image as WriteableBitmap);
                            wbmp.Lock();

                            Int32Rect rect = new(0, 0, (int)img.Rect.Width, (int)img.Rect.Height);
                            wbmp.WritePixels(rect, usedData, rect.Width * img.DataBytes, 0);

                            wbmp.Unlock();
                        }, System.Windows.Threading.DispatcherPriority.Render);
                    });
                }, token);
            }
            else
            {
                Task.Run(() =>
                {
                    Parallel.For(0, _images.Count, new ParallelOptions() { CancellationToken = token }, (index, state) =>
                    {
                        if (token.IsCancellationRequested)
                        {
                            state.Stop();
                            return;
                        }
                        var img = _images[index];
                        byte[] usedData = img.OriginalData;
                        if (img.BinSize > 1)
                        {
                            usedData = img.FlimSumData;
                        }
                        var pixelCount = usedData.Length / img.DataBytes;
                        Array finnalData;

                        var upper = (1 << (img.DataBytes * 8)) - 1;
                        var m_value = upper / (max.Value - min.Value);
                        var a_value = 0 - min.Value * m_value;

                        if (img.PixelType == OmeTiffLibraryWrapper.PixelType.PIXEL_UINT16)
                        {
                            ushort[] tempData = new ushort[pixelCount];

                            for (int i = 0; i < pixelCount; i++)
                            {
                                ushort data_i = BitConverter.ToUInt16(usedData, i * img.DataBytes);
                                tempData[i] = (ushort)Math.Clamp(data_i * m_value + a_value, 0d, upper);
                            }
                            finnalData = tempData;

                        }
                        else
                        {
                            byte[] tempData = new byte[pixelCount];

                            for (int i = 0; i < pixelCount; i++)
                            {
                                tempData[i] = (byte)Math.Clamp(usedData[i] * m_value + a_value, 0d, upper);
                            }
                            finnalData = tempData;
                        }

                        if (token.IsCancellationRequested)
                        {
                            state.Stop();
                            return;
                        }

                        Application.Current?.Dispatcher.Invoke(() =>
                        {
                            var wbmp = (img.Image as WriteableBitmap);
                            wbmp.Lock();

                            Int32Rect rect = new(0, 0, (int)img.Rect.Width, (int)img.Rect.Height);
                            wbmp.WritePixels(rect, finnalData, rect.Width * img.DataBytes, 0);

                            wbmp.Unlock();
                        }, System.Windows.Threading.DispatcherPriority.Render);
                    });
                }, token);
            }
        }

        private void UpdateImage()
        {
            Task.Run(() =>
            {
                while (true)
                {
                    if (!IsLoading && _frameInfoQueue.TryDequeue(out var frameDetail))
                    {
                        try
                        {
                            IsLoading = true;
                            ImageMinValue = double.NaN;
                            ImageMaxValue = double.NaN;

                            var scanInfo = frameDetail.ScanInfo;
                            var scanRegionInfo = frameDetail.ScanRegionInfo;
                            var channelInfo = frameDetail.ChannelInfo;

                            var binSize = channelInfo.BinSize;
                            var pixelType = scanInfo.PixelType;

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
                                Width = size.Width * scanInfo.PixelPhysicalSizeX * widthMultiply,
                                Height = size.Height * scanInfo.PixelPhysicalSizeY * heightMultiply
                            };

                            if (size != Coordinate.PixelSize)
                                Coordinate.PixelSize = size;
                            if (rect != Coordinate.PhysicalRect)
                                Coordinate.PhysicalRect = rect;

                            Application.Current.Dispatcher.Invoke(() => Coordinate.AutoFit());

                            var dataBytes = 1;
                            PixelFormat pixelFormat = PixelFormats.Gray8;
                            switch (pixelType)
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
                                if (columnCount >= rowCount)
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
                            var bytesPerElement = bytesPerPixel * binSize;

                            if (frameDetail.IsFrameSizeOrTileChanged)
                                _images.Clear();

                            object locker = new();
                            Parallel.For(0, totalCount, index =>
                            //for (int index = 0; index < totalCount; index++)
                            {
                                var r = index / columnCount;
                                var c = index % columnCount;

                                var y = (uint)(r * multiTileHeight);
                                var h = imageHeight - y >= multiTileHeight ? multiTileHeight : imageHeight - y;

                                var x = (uint)(c * multiTileWidth);
                                var w = imageWidth - x >= multiTileWidth ? multiTileWidth : imageWidth - x;

                                uint count = w * h;
                                byte[] imageBuffer = new byte[count * bytesPerElement];

                                //use function ome_get_raw_tile_data
                                if (multiTileWidth == tileWidth && multiTileHeight == tileHeight && index % 2 == 1)
                                {
                                    unsafe
                                    {
                                        fixed (byte* bufferPointer = imageBuffer)
                                        {
                                            var ptr = new IntPtr(bufferPointer);
                                            if (_fileHandle < 0)
                                                return;

                                            int status = OmeTiffLibraryWrapper.ome_get_raw_tile_data(_fileHandle, frameDetail.FrameInfo, (uint)r, (uint)c, ptr, (uint)(w * bytesPerElement));
                                            if (status != 0)
                                                return;
                                        }
                                    }
                                }
                                //use function ome_get_raw_rect
                                else
                                {
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

                                            int status = OmeTiffLibraryWrapper.ome_get_raw_data(_fileHandle, frameDetail.FrameInfo, omeRect, ptr, (uint)(w * bytesPerElement));
                                            if (status != 0)
                                                return;
                                        }
                                    }
                                }

                                var imageRect = new Rect
                                {
                                    X = x,
                                    Y = y,
                                    Height = h,
                                    Width = w
                                };

                                var isFlimData = binSize > 1;
                                IsFlimData = isFlimData;
                                OnPropertyChanged(nameof(IsFlimData));

                                ImageViewModelEx vm = null;
                                lock (locker)
                                {
                                    vm = _images.FirstOrDefault(f => f.Rect.Equals(imageRect) && f.BinSize == (int)binSize && f.PixelType == pixelType);
                                }

                                if (vm != null)
                                {
                                    Array.Copy(imageBuffer, vm.OriginalData, count * bytesPerElement);
                                }
                                else
                                {
                                    vm = new(imageRect, (int)binSize, scanInfo.PixelType)
                                    {
                                        OriginalData = new byte[count * bytesPerElement]
                                    };
                                    if (isFlimData)
                                        vm.FlimSumData = new byte[count * bytesPerPixel];
                                    Array.Copy(imageBuffer, vm.OriginalData, count * bytesPerElement);
                                    Application.Current?.Dispatcher.Invoke(() =>
                                    {
                                        WriteableBitmap wbmp = new((int)imageRect.Width, (int)imageRect.Height, 96, 96, pixelFormat, null);
                                        vm.Image = wbmp;
                                    });
                                    lock (locker)
                                    {
                                        _images.Add(vm);
                                    }
                                }

                                if (isFlimData)
                                {
                                    int maxValue = (1 << (dataBytes * 8)) - 1;
                                    for (int i = 0; i < count; i++)
                                    {
                                        if (pixelType == OmeTiffLibraryWrapper.PixelType.PIXEL_UINT16)
                                        {
                                            uint summary = 0;
                                            for (int j = 0; j < binSize; j++)
                                            {
                                                ushort data_i = BitConverter.ToUInt16(imageBuffer, (i * (int)binSize + j) * dataBytes);
                                                summary += data_i;
                                            }
                                            var clampValue = Math.Clamp(summary, 0, maxValue);
                                            if (double.IsNaN(ImageMinValue) || ImageMinValue > clampValue)
                                                ImageMinValue = clampValue;
                                            if (double.IsNaN(ImageMaxValue) || ImageMaxValue < clampValue)
                                                ImageMaxValue = clampValue;
                                            var bytes = BitConverter.GetBytes((ushort)clampValue);
                                            vm.FlimSumData[i * dataBytes] = bytes[0];
                                            vm.FlimSumData[i * dataBytes + 1] = bytes[1];
                                        }
                                        else
                                        {
                                            uint summary = 0;
                                            for (int j = 0; j < binSize; j++)
                                            {
                                                summary += imageBuffer[(i * binSize + j) * dataBytes];
                                            }
                                            var clampValue = Math.Clamp(summary, 0, maxValue);
                                            if (double.IsNaN(ImageMinValue) || ImageMinValue > clampValue)
                                                ImageMinValue = clampValue;
                                            if (double.IsNaN(ImageMaxValue) || ImageMaxValue < clampValue)
                                                ImageMaxValue = clampValue;
                                            vm.FlimSumData[i] = (byte)clampValue;
                                        }
                                    }
                                }
                                else
                                {
                                    if (pixelType == OmeTiffLibraryWrapper.PixelType.PIXEL_UINT16)
                                    {
                                        for (int i = 0; i < count; i++)
                                        {
                                            ushort data_i = BitConverter.ToUInt16(imageBuffer, i * dataBytes);
                                            if (double.IsNaN(ImageMinValue) || ImageMinValue > data_i)
                                                ImageMinValue = data_i;
                                            if (double.IsNaN(ImageMaxValue) || ImageMaxValue < data_i)
                                                ImageMaxValue = data_i;
                                        }
                                    }
                                    else
                                    {
                                        ImageMinValue = imageBuffer.Min();
                                        ImageMaxValue = imageBuffer.Max();
                                    }
                                }
                            });
                            //}

                            if (_isAutoBC)
                            {
                                _minValue = ImageMinValue;
                                _maxValue = ImageMaxValue;
                                OnPropertyChanged(nameof(MinValue));
                                OnPropertyChanged(nameof(MaxValue));
                                UpdateImageSource(ImageMinValue, ImageMaxValue);
                            }
                            else
                                UpdateImageSource(null, null);

                            Application.Current.Dispatcher.Invoke(() => UpdateFlimData(new Point(_pixelPointX, _pixelPointY)));
                        }
                        finally
                        {
                            IsLoading = false;
                        }
                    }
                    else
                    {
                        Thread.Sleep(1);
                    }
                }
            });
        }

        private void Plate_LoadImageDataEvent(object sender, ImageChangeType type)
        {
            if (sender is PlateDetail plate && plate.SelectedScan != null)
            {
                var scan = plate.SelectedScan;
                if (type == ImageChangeType.Plate || type == ImageChangeType.Scan || double.IsNaN(MaxLimit))
                {
                    MaxLimit = (1 << (int)scan.BaseInfo.SignificatBits) - 1;
                    OnPropertyChanged(nameof(MaxLimit));
                    _minValue = 0;
                    _maxValue = MaxLimit;
                    OnPropertyChanged(nameof(MinValue));
                    OnPropertyChanged(nameof(MaxValue));
                }
                if (plate.SelectedScan.SelectedChannel != null)
                {
                    var channel = scan.SelectedChannel.Value;
                    if (plate.SelectedScan.SelectedScanRegion != null)
                    {
                        var scanRegion = scan.SelectedScanRegion.Value;
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
                        if (type == ImageChangeType.T || type == ImageChangeType.Z)
                            frameDetail.IsFrameSizeOrTileChanged = false;
                        else
                            frameDetail.IsFrameSizeOrTileChanged = true;
                        _frameInfoQueue.Clear();
                        _frameInfoQueue.Enqueue(frameDetail);
                    }
                }
            }
        }

        private void ClearAll()
        {
            OmeTiffLibraryWrapper.ome_close_file(_fileHandle);
            _fileHandle = -1;
            _images.Clear();
            _plates.Clear();
            SelectedPlate = null;
            _flimData.Clear();
            PhysicalPoint = null;
            Intensity = 0;
            ImageMinValue = double.NaN;
            ImageMaxValue = double.NaN;
            MaxLimit = double.NaN;
            OnPropertyChanged(nameof(MaxLimit));
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
                if (!IsValid)
                    return;
                //Auto select one
                SelectedPlate = Plates.FirstOrDefault();
                if (SelectedPlate != null)
                {
                    SelectedPlate.SelectedScan = SelectedPlate.ScanDetails.FirstOrDefault();
                    if (SelectedPlate.SelectedScan != null)
                    {
                        SelectedPlate.SelectedScan.SelectedScanRegion = SelectedPlate.SelectedScan.ScanRegionInfos.FirstOrDefault();
                        SelectedPlate.SelectedScan.SelectedChannel = SelectedPlate.SelectedScan.ChannelInfos.FirstOrDefault();
                    }
                }
            });
        }
    }
}

using System.Windows;

namespace ImageReviewTool.ImageTool
{
    public delegate void ReDrawEventHandler(object sender, EventArgs e);

    public class Coordinate : ICloneable
    {
        public event ReDrawEventHandler ReDraw;
        /// <summary>
        /// PhysicalSize of Content
        /// </summary>
        public Rect PhysicalRect { get => _physicalRect; set { _physicalRect = value; UpdateScale(); } }
        /// <summary>
        /// PixelSize of Content
        /// </summary>
        public Size PixelSize { get => _pixelSize; set { _pixelSize = value; UpdateScale(); } }
        /// <summary>
        /// Physical display area of content
        /// </summary>
        public virtual Rect DisplayPhysicalArea { get => _displayPhysicalArea; set { _displayPhysicalArea = value; UpdateScale(); } }
        /// <summary>
        /// Size on the screen
        /// </summary>
        public Size SizeOnScreen
        {
            get => _sizeOnScreen;
            set
            {
                var loadedFlag = (_sizeOnScreen.Width == 0 || _sizeOnScreen.Height == 0) && (value.Width != 0 && value.Height != 0);
                _sizeOnScreen = value;
                if (loadedFlag)
                    AutoFit();
                else
                    UpdateScale();
            }
        }

        public Thickness Padding
        {
            get => _padding;
            set
            {
                _padding = value;
                UpdateScale();
            }
        }

        private Size SizeOnScreenWithPadding
        {
            get
            {
                if (SizeOnScreen.Width == 0 || SizeOnScreen.Height == 0)
                    return SizeOnScreen;
                var width = SizeOnScreen.Width - _padding.Left - _padding.Right;
                var height = SizeOnScreen.Height - _padding.Top - _padding.Bottom;
                return new Size(width < 0 ? 0 : width, height < 0 ? 0 : height);
            }
        }

        private double _xScale;
        private double _yScale;
        private Rect _physicalRect;
        private Size _pixelSize;
        private Rect _displayPhysicalArea;
        private Size _sizeOnScreen;
        private Thickness _padding;

        public double Scale => Math.Max(_xScale, _yScale);

        #region rect convert
        /// <summary>
        /// Get the position on screen
        /// </summary>
        /// <param name="rectInPhysical">physical position</param>
        /// <returns>position on screen</returns>
        public Rect GetScreenRectFromPhysical(Rect rectInPhysical)
        {
            double xVisualScale = SizeOnScreenWithPadding.Width / DisplayPhysicalArea.Width;
            double yVisualScale = SizeOnScreenWithPadding.Height / DisplayPhysicalArea.Height;
            var x = (rectInPhysical.X - DisplayPhysicalArea.X) * xVisualScale;
            var y = (rectInPhysical.Y - DisplayPhysicalArea.Y) * yVisualScale;
            var width = rectInPhysical.Width * xVisualScale;
            var height = rectInPhysical.Height * yVisualScale;
            return new Rect(x + _padding.Left, y + _padding.Top, width, height);
        }

        public Rect GetPhysicalRectFromScreen(Rect rectOnScreen)
        {
            rectOnScreen.X -= _padding.Left;
            rectOnScreen.Y -= _padding.Top;
            double xVisualScale = SizeOnScreenWithPadding.Width / DisplayPhysicalArea.Width;
            double yVisualScale = SizeOnScreenWithPadding.Height / DisplayPhysicalArea.Height;
            var x = rectOnScreen.X / xVisualScale + DisplayPhysicalArea.X;
            var y = rectOnScreen.Y / yVisualScale + DisplayPhysicalArea.Y;
            var width = rectOnScreen.Width / xVisualScale;
            var height = rectOnScreen.Height / yVisualScale;
            return new Rect(x, y, width, height);
        }

        public Rect GetPixelRectFromPhysical(Rect rectInPhysical)
        {
            var xScale = PixelSize.Width / PhysicalRect.Width;
            var yScale = PixelSize.Height / PhysicalRect.Height;
            var x = (rectInPhysical.X - PhysicalRect.X) * xScale;
            var y = (rectInPhysical.Y - PhysicalRect.Y) * yScale;
            var width = rectInPhysical.Width * xScale;
            var height = rectInPhysical.Height * yScale;
            return new Rect(x, y, width, height);
        }

        public Rect GetPhysicalRectFromPixel(Rect rectInPixel)
        {
            var xScale = PixelSize.Width / PhysicalRect.Width;
            var yScale = PixelSize.Height / PhysicalRect.Height;
            var x = rectInPixel.X / xScale + PhysicalRect.X;
            var y = rectInPixel.Y / yScale + PhysicalRect.Y;
            var width = rectInPixel.Width / xScale;
            var height = rectInPixel.Height / yScale;
            return new Rect(x, y, width, height);
        }

        public Rect GetScreenRectFromPixel(Rect rectInPixel)
        {
            var rectInPhysical = GetPhysicalRectFromPixel(rectInPixel);
            return GetScreenRectFromPhysical(rectInPhysical);
        }

        public Rect GetPixelRectFromScreen(Rect rectOnScreen)
        {
            var rectInPhysical = GetPhysicalRectFromScreen(rectOnScreen);
            return GetPixelRectFromPhysical(rectInPhysical);
        }
        #endregion

        #region point convert
        public Point GetPixelPointFromPhysical(Point pointInPhysical)
        {
            var xScale = PixelSize.Width / PhysicalRect.Width;
            var yScale = PixelSize.Height / PhysicalRect.Height;
            var x = (pointInPhysical.X - PhysicalRect.X) * xScale;
            var y = (pointInPhysical.Y - PhysicalRect.Y) * yScale;
            return new Point(x, y);
        }

        public Point GetPhysicalPointFromScreen(Point pointOnScreen)
        {
            pointOnScreen.X -= _padding.Left;
            pointOnScreen.Y -= _padding.Top;
            var xScale = SizeOnScreenWithPadding.Width / DisplayPhysicalArea.Width;
            var yScale = SizeOnScreenWithPadding.Height / DisplayPhysicalArea.Height;
            var x = pointOnScreen.X / xScale + DisplayPhysicalArea.X;
            var y = pointOnScreen.Y / yScale + DisplayPhysicalArea.Y;
            return new Point(x, y);
        }

        public Point GetPhysicalPointFromPixel(Point pointInPixel)
        {
            var xScale = PixelSize.Width / PhysicalRect.Width;
            var yScale = PixelSize.Height / PhysicalRect.Height;
            var x = pointInPixel.X / xScale + PhysicalRect.X;
            var y = pointInPixel.Y / yScale + PhysicalRect.Y;
            return new Point(x, y);
        }

        public Point GetScreenPointFromPixel(Point pointInPixel)
        {
            var pointInPhysical = GetPhysicalPointFromPixel(pointInPixel);
            return GetScreenPointFromPhysical(pointInPhysical);
        }

        public Point GetScreenPointFromPhysical(Point pointInPhysical)
        {
            var xScale = SizeOnScreenWithPadding.Width / DisplayPhysicalArea.Width;
            var yScale = SizeOnScreenWithPadding.Height / DisplayPhysicalArea.Height;
            var x = (pointInPhysical.X - DisplayPhysicalArea.X) * xScale;
            var y = (pointInPhysical.Y - DisplayPhysicalArea.Y) * yScale;
            return new Point(x + _padding.Left, y + Padding.Top);
        }

        public Point GetPixelPointFromScreen(Point pointOnScreen)
        {
            pointOnScreen.X -= _padding.Left;
            pointOnScreen.Y -= _padding.Top;
            var pointInPhysical = GetPhysicalPointFromScreen(pointOnScreen);
            return GetPixelPointFromPhysical(pointInPhysical);
        }
        #endregion

        public void RaiseReDraw()
        {
            ReDraw?.Invoke(this, new EventArgs());
        }

        public void AutoFit()
        {
            var rateOfScreen = SizeOnScreenWithPadding.Width / SizeOnScreenWithPadding.Height;

            var rateOfPhysical = PixelSize.Width / PixelSize.Height;
            if (rateOfScreen > rateOfPhysical)
            {
                var height = PixelSize.Height;
                var width = rateOfScreen * height;
                var x = (PixelSize.Width - width) / 2;
                var y = 0;
                DisplayPhysicalArea = GetPhysicalRectFromPixel(new Rect(x, y, width, height));
            }
            else if (rateOfScreen < rateOfPhysical)
            {
                var width = PixelSize.Width;
                var height = width / rateOfScreen;
                var y = (PixelSize.Height - height) / 2;
                var x = 0;
                DisplayPhysicalArea = GetPhysicalRectFromPixel(new Rect(x, y, width, height));
            }
            else
            {
                DisplayPhysicalArea = GetPhysicalRectFromPixel(new Rect(PixelSize));
            }

            UpdateScale();
            SetScale(new Point(SizeOnScreen.Width / 2, SizeOnScreen.Height / 2), Scale);
        }

        public void AspectRatio(bool isAspectRatio)
        {
            var displayArea = DisplayPhysicalArea;
            var displayCenter = new Point(displayArea.X + displayArea.Width / 2, displayArea.Y + displayArea.Height / 2);

            if (isAspectRatio)
            {
                var rateOfScreen = SizeOnScreenWithPadding.Width / SizeOnScreenWithPadding.Height;
                var rateOfPhysical = PhysicalRect.Width / PhysicalRect.Height;
                if (rateOfScreen > rateOfPhysical)
                {
                    var height = PhysicalRect.Height;
                    var width = rateOfScreen * height;
                    var x = (PhysicalRect.Width - width) / 2;
                    var y = 0;
                    DisplayPhysicalArea = new Rect(x, y, width, height);
                }
                else if (rateOfScreen < rateOfPhysical)
                {
                    var width = PhysicalRect.Width;
                    var height = width / rateOfScreen;
                    var y = (PhysicalRect.Height - height) / 2;
                    var x = 0;
                    DisplayPhysicalArea = new Rect(x, y, width, height);
                }
                else
                {
                    DisplayPhysicalArea = PhysicalRect;
                }

                UpdateScale();
                SetScale(new Point(SizeOnScreen.Width / 2, SizeOnScreen.Height / 2), Scale);
            }
            else
            {
                AutoFit();
            }

            if (PhysicalRect.Width > PhysicalRect.Height)
            {
                var ratioHeight = DisplayPhysicalArea.Height / DisplayPhysicalArea.Width * displayArea.Width;
                DisplayPhysicalArea = new Rect(displayCenter.X - displayArea.Width / 2, displayCenter.Y - ratioHeight / 2, displayArea.Width, ratioHeight);
            }
            else
            {
                var ratioWidth = DisplayPhysicalArea.Width / DisplayPhysicalArea.Height * displayArea.Height;
                DisplayPhysicalArea = new Rect(displayCenter.X - ratioWidth / 2, displayCenter.Y - displayArea.Height / 2, ratioWidth, displayArea.Height);
            }

            ReDraw?.Invoke(this, new EventArgs());
        }

        private void UpdateScale()
        {
            var displayRectInPixel = GetPixelRectFromPhysical(DisplayPhysicalArea);
            _xScale = SizeOnScreenWithPadding.Width / displayRectInPixel.Width;
            _yScale = SizeOnScreenWithPadding.Height / displayRectInPixel.Height;
            UpdateMinScale();
        }

        protected virtual void UpdateMinScale()
        {
            var widthRate = SizeOnScreenWithPadding.Width / PixelSize.Width;
            var heightRate = SizeOnScreenWithPadding.Height / PixelSize.Height;
            _minScale = Math.Min(widthRate, heightRate);
            _minScale = Math.Min(1, _minScale);
        }

        public void MoveInPixel(Vector vector)
        {
            double visualScale = SizeOnScreenWithPadding.Width / DisplayPhysicalArea.Width;
            var physicalVector = new Vector(vector.X / visualScale, vector.Y / visualScale);
            var topLeft = DisplayPhysicalArea.TopLeft + physicalVector;
            var bottomRight = DisplayPhysicalArea.BottomRight + physicalVector;
            var displayPhysicalArea = new Rect(topLeft, bottomRight);
            var displayPhysicalAreaInScreen = GetScreenRectFromPhysical(displayPhysicalArea);
            var imageAreaInScreen = GetScreenRectFromPhysical(PhysicalRect);
            if (displayPhysicalAreaInScreen.IntersectsWith(imageAreaInScreen) == false)
                return;
            var yPoints = new List<double>() { imageAreaInScreen.Top, imageAreaInScreen.Bottom, displayPhysicalAreaInScreen.Top, displayPhysicalAreaInScreen.Bottom };
            yPoints.Sort();
            var xPoints = new List<double>() { imageAreaInScreen.Left, imageAreaInScreen.Right, displayPhysicalAreaInScreen.Left, displayPhysicalAreaInScreen.Right };
            xPoints.Sort();
            if (yPoints[2] - yPoints[1] < 8 || xPoints[2] - xPoints[1] < 8)
                return;
            DisplayPhysicalArea = displayPhysicalArea;
            ReDraw?.Invoke(this, new EventArgs());
        }

        public void MoveInPhysical(Vector vector)
        {
            var topLeft = DisplayPhysicalArea.TopLeft + vector;
            var bottomRight = DisplayPhysicalArea.BottomRight + vector;
            var displayPhysicalArea = new Rect(topLeft, bottomRight);
            var displayPhysicalAreaInScreen = GetScreenRectFromPhysical(displayPhysicalArea);
            var imageAreaInScreen = GetScreenRectFromPhysical(PhysicalRect);
            if (displayPhysicalAreaInScreen.IntersectsWith(imageAreaInScreen) == false)
                return;
            var yPoints = new List<double>() { imageAreaInScreen.Top, imageAreaInScreen.Bottom, displayPhysicalAreaInScreen.Top, displayPhysicalAreaInScreen.Bottom };
            yPoints.Sort();
            var xPoints = new List<double>() { imageAreaInScreen.Left, imageAreaInScreen.Right, displayPhysicalAreaInScreen.Left, displayPhysicalAreaInScreen.Right };
            xPoints.Sort();
            if (yPoints[2] - yPoints[1] < 8 || xPoints[2] - xPoints[1] < 8)
                return;
            DisplayPhysicalArea = displayPhysicalArea;
            ReDraw?.Invoke(this, new EventArgs());
        }

        private const double ScaleRate = 1.1;
        private const double MaxScale = 10;
        protected double _minScale = 1;
        protected double MinScale => _minScale;
        private void Zoom(Point pointOnScreen, bool isZoomIn)
        {
            var scaleRate = ScaleRate;
            if (isZoomIn)
            {
                var tmpScale = Math.Max(_xScale, _yScale);
                if (tmpScale * scaleRate > MaxScale)
                {
                    scaleRate = MaxScale / tmpScale;
                }
                _xScale *= scaleRate;
                _yScale *= scaleRate;
            }
            else
            {

                var tmpScale = Math.Min(_xScale, _yScale);
                if (tmpScale / scaleRate < MinScale)
                {
                    scaleRate = tmpScale / MinScale;
                }
                _xScale /= scaleRate;
                _yScale /= scaleRate;
            }
            SetScale(pointOnScreen, Math.Max(_xScale, _yScale));
        }

        /// <summary>
        /// Set the scale with a center point
        /// </summary>
        /// <param name="pointOnScreen">point in screen coordinate</param>
        /// <param name="xScale">x scale of screen / pixel</param>
        /// <param name="yScale">y scale of screen / pixel</param>
        private void SetScale(Point pointOnScreen, double xScale, double yScale)
        {
            var actualPoint = GetPhysicalPointFromScreen(pointOnScreen);

            var tempRect = GetPhysicalRectFromPixel(new Rect(0, 0, SizeOnScreenWithPadding.Width / xScale, SizeOnScreenWithPadding.Height / yScale));
            var width = tempRect.Width;
            var height = tempRect.Height;
            var xVisualScale = SizeOnScreenWithPadding.Width / width;
            var yVisualScale = SizeOnScreenWithPadding.Height / height;
            var x = actualPoint.X - (pointOnScreen.X - _padding.Left) / xVisualScale;
            var y = actualPoint.Y - (pointOnScreen.Y - _padding.Top) / yVisualScale;
            DisplayPhysicalArea = new Rect(x, y, width, height);
            ReDraw?.Invoke(this, new EventArgs());
        }

        public void SetScale(Point pointOnScreen, double scale)
        {
            if (scale > MaxScale)
                scale = MaxScale;
            if (scale < MinScale)
                scale = MinScale;
            if (_xScale > _yScale)
            {
                var rate = _xScale / scale;
                _yScale = _yScale / rate;
                _xScale = scale;
            }
            else
            {
                var rate = _yScale / scale;
                _xScale = _xScale / rate;
                _yScale = scale;
            }
            SetScale(pointOnScreen, _xScale, _yScale);
        }

        public void ZoomIn(Point pointOnScreen)
        {
            Zoom(pointOnScreen, true);
        }

        public void ZoomOut(Point pointOnScreen)
        {
            Zoom(pointOnScreen, false);
        }

        public Point CoercePoint(Point p)
        {
            return CoercePointInRect(p, PhysicalRect);
        }
        public Point CoercePointInDisplayArea(Point p)
        {
            return CoercePointInRect(p, DisplayPhysicalArea);
        }

        private Point CoercePointInRect(Point p, Rect rect)
        {
            if (p.X > rect.Right)
                p.X = rect.Right;
            else if (p.X < rect.Left)
                p.X = rect.Left;

            if (p.Y > rect.Bottom)
                p.Y = rect.Bottom;
            else if (p.Y < rect.Top)
                p.Y = rect.Top;

            return p;
        }

        public Rect CoerceRect(Rect rect)
        {
            return CoerceRectInArea(rect, PhysicalRect);
        }

        public Rect CoerceRectInDisplayArea(Rect rect)
        {
            return CoerceRectInArea(rect, DisplayPhysicalArea);
        }

        private Rect CoerceRectInArea(Rect rect, Rect area)
        {
            if (rect.Width > area.Width)
                rect.Width = area.Width;
            if (rect.Height > area.Height)
                rect.Height = area.Height;
            if (rect.Right > area.Right)
                rect.X = area.Right - rect.Width;
            if (rect.X < area.Left)
                rect.X = area.Left;
            if (rect.Bottom > area.Bottom)
                rect.Y = area.Bottom - rect.Height;
            if (rect.Y < area.Top)
                rect.Y = area.Top;
            return rect;
        }

        public object Clone()
        {
            return new Coordinate()
            {
                _physicalRect = new Rect(this._physicalRect.X, this._physicalRect.Y, this._physicalRect.Width, this._physicalRect.Height),
                _pixelSize = new Size(this._pixelSize.Width, this._pixelSize.Height),
                _displayPhysicalArea = new Rect(this._displayPhysicalArea.X, this._displayPhysicalArea.Y, this._displayPhysicalArea.Width, this._displayPhysicalArea.Height),
                _sizeOnScreen = new Size(this._sizeOnScreen.Width, this._sizeOnScreen.Height),
                _xScale = this._xScale,
                _yScale = this._yScale,
                _padding = this._padding
            };
        }
    }
}

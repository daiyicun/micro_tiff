using System.Windows;
using System.Windows.Input;

namespace ImageReviewTool.ImageTool.Tools
{
    public class ToolDragger : ToolBase, IPixelInfoViewer
    {
        private Point point;

        private Action<Point> ViewPixelAtPointHandler;
        public event Action<Point> ViewPixelAtPoint
        {
            add { ViewPixelAtPointHandler += value; }
            remove { ViewPixelAtPointHandler -= value; }
        }

        public override Cursor GetCursor()
        {
            return Cursors.Hand;
        }

        public override void MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (sender is CanvasBase canvasBase)
            {
                if (e.LeftButton == MouseButtonState.Pressed && e.ClickCount == 2)
                    canvasBase.Coordinate.AutoFit();
                else if (e.LeftButton == MouseButtonState.Pressed || e.MiddleButton == MouseButtonState.Pressed)
                {
                    point = e.GetPosition(canvasBase);
                    canvasBase.CaptureMouse();
                }
            }
            else
                throw new ArgumentException("sender must be CanvasBase type");
            base.MouseDown(sender, e);
        }

        private void RiseViewPixelAtPointEvent(CanvasBase canvasBase, MouseEventArgs e)
        {
            var physicalPoint = canvasBase.Coordinate.GetPhysicalPointFromScreen(e.GetPosition(canvasBase));
            if (canvasBase.Coordinate.PhysicalRect.Contains(physicalPoint))
            {
                var pixelPoint = canvasBase.Coordinate.GetPixelPointFromPhysical(physicalPoint);
                pixelPoint.X = Math.Clamp(pixelPoint.X, 0, canvasBase.Coordinate.PixelSize.Width - 1);
                pixelPoint.Y = Math.Clamp(pixelPoint.Y, 0, canvasBase.Coordinate.PixelSize.Height - 1);
                ViewPixelAtPointHandler?.Invoke(pixelPoint);
            }
        }

        public override void MouseMove(object sender, MouseEventArgs e)
        {
            if (sender is CanvasBase canvasBase)
            {
                RiseViewPixelAtPointEvent(canvasBase, e);
                if (canvasBase.IsMouseCaptured && ((e.LeftButton == MouseButtonState.Pressed && e.RightButton == MouseButtonState.Released && e.MiddleButton == MouseButtonState.Released)
                                                   || (e.LeftButton == MouseButtonState.Released && e.RightButton == MouseButtonState.Released && e.MiddleButton == MouseButtonState.Pressed)))
                {
                    var tPoint = e.GetPosition(canvasBase);
                    if (new Rect(canvasBase.RenderSize).Contains(tPoint))
                    {
                        var v = point - tPoint;
                        canvasBase.Coordinate?.MoveInPixel(v);
                        point = tPoint;
                    }
                }
            }
            else
                throw new ArgumentException("sender must be CanvasBase type");
            base.MouseMove(sender, e);
        }

        public override void MouseUp(object sender, MouseButtonEventArgs e)
        {
            if (sender is CanvasBase canvasBase)
            {
                canvasBase.ReleaseMouseCapture();
            }
            else
                throw new ArgumentException("sender must be CanvasBase type");
            base.MouseUp(sender, e);
        }

        public override void MouseWheel(object sender, MouseWheelEventArgs e)
        {
            if (sender is CanvasBase canvasBase)
            {
                if (e.Delta > 0)
                    canvasBase.Coordinate.ZoomIn(e.GetPosition(canvasBase));
                else
                    canvasBase.Coordinate.ZoomOut(e.GetPosition(canvasBase));
                RiseViewPixelAtPointEvent(canvasBase, e);
            }
            else
                throw new ArgumentException("sender must be CanvasBase type");
            base.MouseWheel(sender, e);
        }

        public ToolDragger()
        {
            _descriptionToolTip = new System.Windows.Controls.ToolTip() { Content = "Dragger" };
        }
    }

}

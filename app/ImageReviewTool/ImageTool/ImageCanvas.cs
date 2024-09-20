using ImageReviewTool.ImageTool.Tools;

namespace ImageReviewTool.ImageTool
{
    public class ImageCanvas : CanvasBase
    {
        public ImageCanvas()
        {
            AvailableTools ??= new List<Type>();
            AvailableTools.Add(typeof(ToolDragger));
        }

        protected override void ConnectTool()
        {
            base.ConnectTool();
            PreviewMouseDown += ImageCanvas_MouseDown;
            PreviewMouseMove += ImageCanvas_MouseMove;
            PreviewMouseUp += ImageCanvas_MouseUp;
            MouseLeave += ImageCanvas_MouseLeave;
            PreviewMouseWheel += ImageCanvas_MouseWheel;
        }

        protected override void DisconnectTool()
        {
            base.DisconnectTool();
            PreviewMouseDown -= ImageCanvas_MouseDown;
            PreviewMouseMove -= ImageCanvas_MouseMove;
            PreviewMouseUp -= ImageCanvas_MouseUp;
            MouseLeave -= ImageCanvas_MouseLeave;
            PreviewMouseWheel -= ImageCanvas_MouseWheel;
        }

        private void ImageCanvas_MouseWheel(object sender, System.Windows.Input.MouseWheelEventArgs e)
        {
            CurrentTool?.MouseWheel(sender, e);
        }

        private void ImageCanvas_MouseLeave(object sender, System.Windows.Input.MouseEventArgs e)
        {
            CurrentTool?.MouseLeave(sender, e);
        }

        private void ImageCanvas_MouseUp(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            CurrentTool?.MouseUp(sender, e);
        }

        private void ImageCanvas_MouseDown(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            CurrentTool?.MouseDown(sender, e);
        }

        private void ImageCanvas_MouseMove(object sender, System.Windows.Input.MouseEventArgs e)
        {
            CurrentTool?.MouseMove(sender, e);
        }
    }

}

using System.Windows;

namespace ImageReviewTool.ImageTool.Tools
{
    public interface IPixelInfoViewer
    {
        event Action<Point> ViewPixelAtPoint;
    }
}

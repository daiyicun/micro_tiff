using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace ImageReviewTool.ImageTool.Materials
{
    public class ImageViewModel : GraphicViewModelBase
    {
        private ImageSource _image;
        private Rect _rect;

        public ImageSource Image
        {
            get => _image;
            set
            {
                if (value != null && !value.IsFrozen && !(value is WriteableBitmap))
                    value.Freeze();
                SetProperty(ref _image, value);
            }
        }
        /// <summary>
        /// Image position. Note this is in pixel
        /// </summary>
        public Rect Rect { get => _rect; set => SetProperty(ref _rect, value); }
    }
}

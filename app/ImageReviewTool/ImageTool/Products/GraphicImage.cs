using ImageReviewTool.ImageTool.Materials;
using System.Windows;
using System.Windows.Data;
using System.Windows.Media;

namespace ImageReviewTool.ImageTool.Products
{
    public class GraphicImage : GraphicBase
    {

        public static readonly DependencyProperty RectProperty = DependencyProperty.Register("Rect", typeof(Rect), typeof(GraphicImage), new PropertyMetadata(default(Rect), RectPropertyChangedCallback));

        /// <summary>
        /// get or set position of image on the coordinate, not in pixel
        /// </summary>
        public Rect Rect
        {
            get { return (Rect)GetValue(RectProperty); }
            set { SetValue(RectProperty, value); }
        }

        private static void RectPropertyChangedCallback(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            (d as GraphicImage).Draw();
        }


        public static readonly DependencyProperty ImageProperty = DependencyProperty.Register("Image", typeof(ImageSource), typeof(GraphicImage), new PropertyMetadata(default(ImageSource), ImagePropertyChangedCallback));

        /// <summary>
        /// get or set the image which need to draw on screen
        /// </summary>
        public ImageSource Image
        {
            get { return (ImageSource)GetValue(ImageProperty); }
            set { SetValue(ImageProperty, value); }
        }

        private static void ImagePropertyChangedCallback(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            (d as GraphicImage).Draw();
        }

        /// <summary>
        /// Drawing image on screen
        /// </summary>
        /// <param name="drawingContext"></param>
        protected override void OnDraw(DrawingContext drawingContext)
        {
            var rect = Coordinate.GetScreenRectFromPixel(Rect);
            var topLeft = rect.TopLeft;
            var bottomRight = rect.BottomRight;
            rect = new Rect(new Point(Math.Floor(topLeft.X), Math.Floor(topLeft.Y)),
                            new Point(Math.Ceiling(bottomRight.X), Math.Ceiling(bottomRight.Y)));
            drawingContext.DrawImage(Image, rect);
        }

        protected override void SetContextBinding()
        {
            var binding = new Binding(nameof(ImageViewModel.Rect))
            {
                Source = DataContext,
                Mode = BindingMode.TwoWay
            };
            SetBinding(RectProperty, binding);
            var binding1 = new Binding(nameof(ImageViewModel.Image))
            {
                Source = DataContext,
                Mode = BindingMode.TwoWay
            };
            SetBinding(ImageProperty, binding1);
        }
    }
}

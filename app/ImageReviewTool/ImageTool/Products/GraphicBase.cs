using System.Windows;
using System.Windows.Data;
using System.Windows.Media;

namespace ImageReviewTool.ImageTool.Products
{
    public class GraphicBase : DrawingVisual
    {
        public Coordinate Coordinate { get; set; }

        public GraphicBase()
        {

        }

        public void Draw()
        {
            if (Coordinate != null)
                using (var drawingContent = RenderOpen())
                {
                    OnDraw(drawingContent);
                }
        }

        /// <summary>
        /// Drawing <see cref="GraphicBase"/> following some logic
        /// </summary>
        /// <param name="drawingContext"></param>
        protected virtual void OnDraw(DrawingContext drawingContext)
        {

        }

        public static readonly DependencyProperty DataContextProperty = DependencyProperty.Register("DataContext", typeof(object), typeof(GraphicBase), new PropertyMetadata(default(object), DataContextPropertyChangedCallback));

        public object DataContext
        {
            get => GetValue(DataContextProperty);
            set => SetValue(DataContextProperty, value);
        }

        private static void DataContextPropertyChangedCallback(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            (d as GraphicBase).SetContextBinding();
        }

        protected virtual void SetContextBinding()
        {

        }

        public void SetBinding(DependencyProperty property, BindingBase binding)
        {
            BindingOperations.SetBinding(this, property, binding);
        }

        public virtual Geometry GetClip()
        {
            return null;
        }


        public static readonly DependencyProperty IsHitTestVisibleProperty = DependencyProperty.Register("IsHitTestVisible", typeof(bool), typeof(GraphicBase), new PropertyMetadata(true));

        public bool IsHitTestVisible
        {
            get { return (bool)GetValue(IsHitTestVisibleProperty); }
            set { SetValue(IsHitTestVisibleProperty, value); }
        }

        protected override HitTestResult HitTestCore(PointHitTestParameters hitTestParameters)
        {
            if (IsHitTestVisible)
                return base.HitTestCore(hitTestParameters);
            return null;
        }
    }

}

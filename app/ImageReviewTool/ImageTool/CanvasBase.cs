using ImageReviewTool.ImageTool.Materials;
using ImageReviewTool.ImageTool.Products;
using ImageReviewTool.ImageTool.Tools;
using System.Collections.Concurrent;
using System.Collections.Specialized;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Media;

namespace ImageReviewTool.ImageTool
{
    public enum SizeChangedBehavior
    {
        NoChange,
        FitToCanvas,
        KeepTheView
    }

    public class CanvasBase : Panel, IScrollInfo
    {
        private readonly ConcurrentDictionary<GraphicViewModelBase, Visual> _itemsContainer = new ConcurrentDictionary<GraphicViewModelBase, Visual>();
        protected readonly VisualCollection VisualCollection;
        public CanvasBase()
        {
            VisualCollection = new VisualCollection(this);
            CompositionTarget.Rendering += CompositionTarget_Rendering;
            SizeChanged += CanvasBase_SizeChanged;
            IsVisibleChanged += CanvasBase_IsVisibleChanged;
            Focusable = true;
            RenderOptions.SetBitmapScalingMode(this, BitmapScalingMode.NearestNeighbor);
            if (GetValue(SizeChangedBehaviorProperty) == null)
                SetCurrentValue(SizeChangedBehaviorProperty, SizeChangedBehavior.NoChange);
        }

        private void CompositionTarget_Rendering(object sender, EventArgs e)
        {
            var clip = new RectangleGeometry(new Rect(new Point(0, 0), RenderSize));
            var graphicsNotDeleted = new HashSet<GraphicViewModelBase>();
            while (_oldGraphics.TryDequeue(out GraphicViewModelBase g))
            {
                if (_itemsContainer.TryGetValue(g, out Visual visual))
                {
                    GraphicRemoving(visual as GraphicBase);
                    VisualCollection.Remove(visual);
                    _itemsContainer.TryRemove(g, out _);
                }
                else
                {
                    graphicsNotDeleted.Add(g);
                }
            }
            while (_newGraphics.TryDequeue(out GraphicViewModelBase item))
            {
                if (graphicsNotDeleted.Contains(item))
                {
                    graphicsNotDeleted.Remove(item);
                    continue;
                }
                if (_itemsContainer.ContainsKey(item))
                    continue;
                var visual = GraphicFactory.GetGraphic(item);
                if (visual != null)
                {
                    visual.Coordinate = Coordinate;
                    visual.Clip = visual.GetClip() ?? clip;
                    visual.Draw();
                    GraphicAdding(visual);
                    VisualCollection.Add(visual);
                    _itemsContainer.TryAdd(item, visual);
                }
            }
        }

        public void UpdateVisuals()
        {
            var clip = new RectangleGeometry(new Rect(new Point(0, 0), RenderSize));
            while (_newGraphics.TryDequeue(out GraphicViewModelBase item))
            {
                if (_itemsContainer.ContainsKey(item))
                    continue;
                var visual = GraphicFactory.GetGraphic(item);
                if (visual != null)
                {
                    visual.Coordinate = Coordinate;
                    visual.Clip = visual.GetClip() ?? clip;
                    visual.Draw();
                    GraphicAdding(visual);
                    VisualCollection.Add(visual);
                    _itemsContainer.TryAdd(item, visual);
                }
            }
        }

        protected virtual void GraphicAdding(GraphicBase graphic)
        {

        }

        protected virtual void GraphicRemoving(GraphicBase graphic)
        {

        }

        private void UpdateClip()
        {
            var clip = new RectangleGeometry(new Rect(new Point(0, 0), RenderSize));
            foreach (var visual in VisualCollection)
            {
                var g = (GraphicBase)visual;
                g.Clip = g.GetClip() ?? clip;
            }
        }

        private void CanvasBase_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            UpdateClip();
        }

        private void CanvasBase_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            UpdateClip();
        }

        protected override int VisualChildrenCount => Children.Count > 0 ? VisualCollection.Count + Children.Count : VisualCollection.Count;
        protected override Visual GetVisualChild(int index)
        {
            if (Children.Count > 0 && index < Children.Count)
            {
                return Children[index];
            }
            return VisualCollection[index - Children.Count];
        }

        public static readonly DependencyProperty SizeChangedBehaviorProperty =
            DependencyProperty.RegisterAttached("SizeChangedBehavior", typeof(SizeChangedBehavior?), typeof(CanvasBase), new PropertyMetadata(null, SizeChangedBehaviorPropertyChangedCallback));

        public static SizeChangedBehavior? GetSizeChangedBehavior(CanvasBase canvas)
        {
            return (SizeChangedBehavior?)canvas.GetValue(SizeChangedBehaviorProperty);
        }

        public static void SetSizeChangedBehavior(CanvasBase canvas, SizeChangedBehavior? behavior)
        {
            canvas.SetValue(SizeChangedBehaviorProperty, behavior);
        }

        private static void SizeChangedBehaviorPropertyChangedCallback(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is CanvasBase canvas)
            {
                if (canvas.ScrollOwner != null && GetSizeChangedBehavior(canvas) == SizeChangedBehavior.FitToCanvas)
                {
                    canvas.SizeChanged -= canvas.Canvas_SizeChanged;
                    canvas.ScrollOwner.SizeChanged -= canvas.Canvas_SizeChanged;
                    canvas.ScrollOwner.SizeChanged += canvas.Canvas_SizeChanged;
                }
                else
                {
                    if (canvas.ScrollOwner != null)
                        canvas.ScrollOwner.SizeChanged -= canvas.Canvas_SizeChanged;
                    canvas.SizeChanged -= canvas.Canvas_SizeChanged;
                    canvas.SizeChanged += canvas.Canvas_SizeChanged;
                }
            }
            else
                throw new ArgumentException("Only available for CanvasBase");
        }

        private void Canvas_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            if (Coordinate != null)
            {
                var sizeChangedBehavior = GetSizeChangedBehavior(this);
                switch (sizeChangedBehavior)
                {
                    case SizeChangedBehavior.FitToCanvas:
                        Coordinate.SizeOnScreen = RenderSize;
                        Coordinate.AutoFit();
                        break;
                    case SizeChangedBehavior.NoChange:
                    {
                        var widthScale = Coordinate.SizeOnScreen.Width / RenderSize.Width;
                        var heightScale = Coordinate.SizeOnScreen.Height / RenderSize.Height;
                        Coordinate.SizeOnScreen = RenderSize;
                        if (widthScale != 0 && heightScale != 0)
                            Coordinate.DisplayPhysicalArea = new Rect(Coordinate.DisplayPhysicalArea.TopLeft,
                                new Size(Coordinate.DisplayPhysicalArea.Width / widthScale, Coordinate.DisplayPhysicalArea.Height / heightScale));
                        Coordinate.RaiseReDraw();
                    }
                    break;
                    case SizeChangedBehavior.KeepTheView:
                    {
                        var center = new Point((Coordinate.DisplayPhysicalArea.Right + Coordinate.DisplayPhysicalArea.Left) / 2, (Coordinate.DisplayPhysicalArea.Top + Coordinate.DisplayPhysicalArea.Bottom) / 2);
                        var originalRatio = Coordinate.DisplayPhysicalArea.Width / Coordinate.DisplayPhysicalArea.Height;
                        var tmpScreenPhysicalRect = Coordinate.GetPhysicalRectFromScreen(new Rect(RenderSize));
                        var newRatio = tmpScreenPhysicalRect.Width / tmpScreenPhysicalRect.Height;
                        if (newRatio < originalRatio)
                        {
                            if (Coordinate.SizeOnScreen.Width > RenderSize.Width)
                            {
                                var height = Coordinate.DisplayPhysicalArea.Width / newRatio;
                                var rect = new Rect(Coordinate.DisplayPhysicalArea.X, center.Y - height / 2, Coordinate.DisplayPhysicalArea.Width, height);
                                Coordinate.DisplayPhysicalArea = rect;
                            }
                            else
                            {
                                var width = Coordinate.DisplayPhysicalArea.Height * newRatio;
                                var rect = new Rect(center.X - width / 2, Coordinate.DisplayPhysicalArea.Y, width, Coordinate.DisplayPhysicalArea.Height);
                                Coordinate.DisplayPhysicalArea = rect;
                            }
                        }
                        else if (newRatio > originalRatio)
                        {
                            if (Coordinate.SizeOnScreen.Height > RenderSize.Height)
                            {
                                var width = Coordinate.DisplayPhysicalArea.Height * newRatio;
                                var rect = new Rect(center.X - width / 2, Coordinate.DisplayPhysicalArea.Y, width, Coordinate.DisplayPhysicalArea.Height);
                                Coordinate.DisplayPhysicalArea = rect;
                            }
                            else
                            {
                                var height = Coordinate.DisplayPhysicalArea.Width / newRatio;
                                var rect = new Rect(Coordinate.DisplayPhysicalArea.X, center.Y - height / 2, Coordinate.DisplayPhysicalArea.Width, height);
                                Coordinate.DisplayPhysicalArea = rect;
                            }
                        }
                        Coordinate.SizeOnScreen = RenderSize;
                        Coordinate.SetScale(new Point(RenderSize.Width / 2, RenderSize.Height / 2), Coordinate.Scale);
                        Coordinate.RaiseReDraw();
                    }
                    break;
                }
            }
        }

        public static readonly DependencyProperty ItemsProperty = DependencyProperty.Register("Items", typeof(IEnumerable<GraphicViewModelBase>), typeof(CanvasBase), new PropertyMetadata(default(IEnumerable<GraphicViewModelBase>), ItemsPropertyChangedCallback));

        public IEnumerable<GraphicViewModelBase> Items
        {
            get => (IEnumerable<GraphicViewModelBase>)GetValue(ItemsProperty);
            set => SetValue(ItemsProperty, value);
        }

        private static void ItemsPropertyChangedCallback(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var canvasBase = d as CanvasBase;
            if (e.OldValue is INotifyCollectionChanged oCollection)
                oCollection.CollectionChanged -= canvasBase.Items_CollectionChanged;
            if (e.NewValue is INotifyCollectionChanged collection)
                collection.CollectionChanged += canvasBase.Items_CollectionChanged;
            if (e.NewValue is IEnumerable<GraphicViewModelBase> newValue)
            {
                foreach (var v in newValue)
                    canvasBase._newGraphics.Enqueue(v);
            }
            if (e.OldValue is IEnumerable<GraphicViewModelBase> oldValue)
            {
                foreach (var v in oldValue)
                    canvasBase._oldGraphics.Enqueue(v);
            }
        }

        private void Items_CollectionChanged(object sender, NotifyCollectionChangedEventArgs e)
        {
            if (e.Action == NotifyCollectionChangedAction.Reset)
            {
                while (!_newGraphics.IsEmpty)
                    _newGraphics.TryDequeue(out GraphicViewModelBase _);
                foreach (GraphicViewModelBase gvm in _itemsContainer.Keys)
                    _oldGraphics.Enqueue(gvm);
                return;
            }
            if (e.NewItems != null)
            {
                foreach (GraphicViewModelBase gvm in e.NewItems)
                {
                    _newGraphics.Enqueue(gvm);
                }
            }
            if (e.OldItems != null)
            {
                foreach (GraphicViewModelBase gvm in e.OldItems)
                {
                    _oldGraphics.Enqueue(gvm);
                }
            }
        }

        private readonly ConcurrentQueue<GraphicViewModelBase> _oldGraphics = new ConcurrentQueue<GraphicViewModelBase>();
        private readonly ConcurrentQueue<GraphicViewModelBase> _newGraphics = new ConcurrentQueue<GraphicViewModelBase>();

        public static readonly DependencyProperty CoordinateProperty = DependencyProperty.Register("Coordinate", typeof(Coordinate), typeof(CanvasBase), new PropertyMetadata(default(Coordinate), CoordinatePropertyChangedCallback));

        public Coordinate Coordinate
        {
            get => (Coordinate)GetValue(CoordinateProperty);
            set => SetValue(CoordinateProperty, value);
        }

        private static void CoordinatePropertyChangedCallback(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (!(d is CanvasBase canvasBase) || canvasBase.Coordinate == null)
                return;
            canvasBase.Coordinate.ReDraw += canvasBase.Coordinate_ReDraw;
            canvasBase.Coordinate.SizeOnScreen = canvasBase.RenderSize;

            foreach (var visual1 in canvasBase.VisualCollection)
            {
                var visual = (GraphicBase)visual1;
                visual.Coordinate = canvasBase.Coordinate;
            }

            canvasBase.Coordinate.AutoFit();
        }

        protected virtual void Coordinate_ReDraw(object sender, EventArgs e)
        {
            var clip = new RectangleGeometry(new Rect(RenderSize));
            foreach (var visual1 in VisualCollection)
            {
                var visual = (GraphicBase)visual1;
                visual.Clip = visual.GetClip() ?? clip;
                visual.Draw();
            }
            ScrollOwner?.InvalidateScrollInfo();
        }

        private Visibility _horizontalScrollbarVisibility = Visibility.Collapsed;
        private Visibility _verticalScrollbarVisibility = Visibility.Collapsed;
        private void ScrollOwner_ScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            if (ScrollOwner.ComputedHorizontalScrollBarVisibility != _horizontalScrollbarVisibility || ScrollOwner.ComputedVerticalScrollBarVisibility != _verticalScrollbarVisibility)
            {
                if (ScrollOwner.ComputedHorizontalScrollBarVisibility == Visibility.Collapsed && ScrollOwner.ComputedVerticalScrollBarVisibility == Visibility.Collapsed)
                    Coordinate.AutoFit();
                _horizontalScrollbarVisibility = ScrollOwner.ComputedHorizontalScrollBarVisibility;
                _verticalScrollbarVisibility = ScrollOwner.ComputedVerticalScrollBarVisibility;
            }
        }

        public static readonly DependencyProperty CurrentToolProperty = DependencyProperty.Register("CurrentTool", typeof(ToolBase), typeof(CanvasBase), new PropertyMetadata(default(ToolBase), CurrentToolPropertyChangedCallback));

        public ToolBase CurrentTool
        {
            get => (ToolBase)GetValue(CurrentToolProperty);
            set => SetValue(CurrentToolProperty, value);
        }

        private bool _isEnableTool;

        protected virtual void ConnectTool()
        {
            _isEnableTool = true;
        }

        protected virtual void DisconnectTool()
        {
            _isEnableTool = false;
        }

        private static void CurrentToolPropertyChangedCallback(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is CanvasBase canvasBase)
            {
                if (canvasBase.CurrentTool != null && canvasBase.AvailableTools != null &&
                    canvasBase.AvailableTools.Contains(canvasBase.CurrentTool.GetType()))
                {
                    canvasBase.Cursor = canvasBase.CurrentTool.GetCursor();
                    if (!canvasBase._isEnableTool)
                        canvasBase.ConnectTool();
                }
                else
                {
                    canvasBase.Cursor = null;
                    if (canvasBase._isEnableTool)
                        canvasBase.DisconnectTool();
                }
            }
        }


        public static readonly DependencyProperty GraphicFactoryProperty = DependencyProperty.Register("GraphicFactory", typeof(IGraphicFactory), typeof(CanvasBase), new PropertyMetadata(new GraphicFactory(), GraphicFactoryPropertyChangedCallback));

        public IGraphicFactory GraphicFactory
        {
            get { return (IGraphicFactory)GetValue(GraphicFactoryProperty); }
            set { SetValue(GraphicFactoryProperty, value); }
        }

        private static void GraphicFactoryPropertyChangedCallback(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var canvasBase = d as CanvasBase;
            canvasBase?.Coordinate?.AutoFit();
        }

        public static readonly DependencyProperty AvailableToolsProperty = DependencyProperty.Register("AvailableTools", typeof(IList<Type>), typeof(CanvasBase), new PropertyMetadata(default(IList<Type>), AvailableToolsPropertyChangedCallbace));

        private static void AvailableToolsPropertyChangedCallbace(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is CanvasBase canvasBase)
            {
                if (canvasBase.CurrentTool != null && canvasBase.AvailableTools != null &&
                    canvasBase.AvailableTools.Contains(canvasBase.CurrentTool.GetType()))
                {
                    canvasBase.Cursor = canvasBase.CurrentTool.GetCursor();
                    if (!canvasBase._isEnableTool)
                        canvasBase.ConnectTool();
                }
                else
                {
                    canvasBase.Cursor = null;
                    if (canvasBase._isEnableTool)
                        canvasBase.DisconnectTool();
                }
            }
        }

        public IList<Type> AvailableTools
        {
            get { return (IList<Type>)GetValue(AvailableToolsProperty); }
            set { SetValue(AvailableToolsProperty, value); }
        }

        protected override Size MeasureOverride(Size availableSize)
        {
            var result = new Size();
            foreach (UIElement element in InternalChildren)
            {
                element.Measure(availableSize);
                result.Width = Math.Max(element.DesiredSize.Width, result.Width);
                result.Height = Math.Max(element.DesiredSize.Height, result.Height);
            }
            return result;
        }

        protected override Size ArrangeOverride(Size finalSize)
        {
            foreach (UIElement element in InternalChildren)
            {
                element.Arrange(new Rect(finalSize));
            }
            return finalSize;
        }

        #region IScrollInfo

        public bool CanVerticallyScroll { get; set; } = true;
        public bool CanHorizontallyScroll { get; set; } = true;

        public double ExtentWidth
        {
            get
            {
                if (Coordinate == null)
                    return 0;
                var right = Math.Max(Coordinate.PhysicalRect.Right, Coordinate.DisplayPhysicalArea.Right);
                var left = Math.Min(Coordinate.PhysicalRect.Left, Coordinate.DisplayPhysicalArea.Left);
                return right - left;
            }
        }

        public double ExtentHeight
        {
            get
            {
                if (Coordinate == null)
                    return 0;
                var bottom = Math.Max(Coordinate.PhysicalRect.Bottom, Coordinate.DisplayPhysicalArea.Bottom);
                var top = Math.Min(Coordinate.PhysicalRect.Top, Coordinate.DisplayPhysicalArea.Top);
                return bottom - top;
            }
        }

        public double ViewportWidth => Coordinate == null ? 0 : Coordinate.DisplayPhysicalArea.Width;

        public double ViewportHeight => Coordinate == null ? 0 : Coordinate.DisplayPhysicalArea.Height;

        public double HorizontalOffset
        {
            get
            {
                if (Coordinate == null)
                    return 0;
                var result = Coordinate.DisplayPhysicalArea.Left - Coordinate.PhysicalRect.Left;
                if (result < 0)
                    return 0;
                return result;
            }
        }

        public double VerticalOffset
        {
            get
            {
                if (Coordinate == null)
                    return 0;
                var result = Coordinate.DisplayPhysicalArea.Top - Coordinate.PhysicalRect.Top;
                if (result < 0)
                    return 0;
                return result;
            }
        }

        private ScrollViewer _scrollOwner;
        public ScrollViewer ScrollOwner
        {
            get => _scrollOwner;
            set
            {
                if (_scrollOwner != null)
                    _scrollOwner.ScrollChanged -= ScrollOwner_ScrollChanged;
                _scrollOwner = value;
                if (_scrollOwner != null)
                {
                    _scrollOwner.ScrollChanged += ScrollOwner_ScrollChanged;
                    _scrollOwner.Focusable = false;
                }
                if (_scrollOwner != null && GetSizeChangedBehavior(this) == SizeChangedBehavior.FitToCanvas)
                {
                    SizeChanged -= Canvas_SizeChanged;
                    _scrollOwner.SizeChanged += Canvas_SizeChanged;
                }
            }
        }

        public void LineUp()
        {
            if (Coordinate == null)
                return;

            var stepSize = Coordinate.DisplayPhysicalArea.Height / 10;
            Coordinate.MoveInPhysical(new Vector(0, -stepSize));
            ScrollOwner?.InvalidateScrollInfo();
        }

        public void LineDown()
        {
            if (Coordinate == null)
                return;

            var stepSize = Coordinate.DisplayPhysicalArea.Height / 10;
            Coordinate.MoveInPhysical(new Vector(0, stepSize));
            ScrollOwner?.InvalidateScrollInfo();
        }

        public void LineLeft()
        {
            if (Coordinate == null)
                return;

            var stepSize = Coordinate.DisplayPhysicalArea.Width / 10;
            Coordinate.MoveInPhysical(new Vector(-stepSize, 0));
            ScrollOwner?.InvalidateScrollInfo();
        }

        public void LineRight()
        {
            if (Coordinate == null)
                return;

            var stepSize = Coordinate.DisplayPhysicalArea.Width / 10;
            Coordinate.MoveInPhysical(new Vector(stepSize, 0));
            ScrollOwner?.InvalidateScrollInfo();
        }

        public void PageUp()
        {
            if (Coordinate == null)
                return;

            var stepSize = Coordinate.DisplayPhysicalArea.Height;
            Coordinate.MoveInPhysical(new Vector(0, -stepSize));
            ScrollOwner?.InvalidateScrollInfo();
        }

        public void PageDown()
        {
            if (Coordinate == null)
                return;

            var stepSize = Coordinate.DisplayPhysicalArea.Height;
            Coordinate.MoveInPhysical(new Vector(0, stepSize));
            ScrollOwner?.InvalidateScrollInfo();
        }

        public void PageLeft()
        {
            if (Coordinate == null)
                return;

            var stepSize = Coordinate.DisplayPhysicalArea.Width;
            Coordinate.MoveInPhysical(new Vector(-stepSize, 0));
            ScrollOwner?.InvalidateScrollInfo();
        }

        public void PageRight()
        {
            if (Coordinate == null)
                return;

            var stepSize = Coordinate.DisplayPhysicalArea.Width;
            Coordinate.MoveInPhysical(new Vector(stepSize, 0));
            ScrollOwner?.InvalidateScrollInfo();
        }

        public void MouseWheelUp()
        {
            ScrollOwner?.InvalidateScrollInfo();
        }

        public void MouseWheelDown()
        {
            ScrollOwner?.InvalidateScrollInfo();
        }

        public void MouseWheelLeft()
        {
            ScrollOwner?.InvalidateScrollInfo();
        }

        public void MouseWheelRight()
        {
            ScrollOwner?.InvalidateScrollInfo();
        }

        public void SetHorizontalOffset(double offset)
        {
            if (Coordinate == null)
                return;
            Coordinate.MoveInPhysical(new Vector(offset - HorizontalOffset, 0));
        }

        public void SetVerticalOffset(double offset)
        {
            if (Coordinate == null)
                return;
            Coordinate.MoveInPhysical(new Vector(0, offset - VerticalOffset));
        }

        public Rect MakeVisible(Visual visual, Rect rectangle)
        {
            return new Rect();
        }
        #endregion
    }
}

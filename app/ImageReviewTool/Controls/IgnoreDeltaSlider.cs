using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Input;

namespace ImageReviewTool.Controls
{
    public class IgnoreDeltaSlider : Slider
    {
        public event EventHandler DragCompletedEvent;

        public IgnoreDeltaSlider()
        {
            Style = new Style(GetType(), this.FindResource(typeof(Slider)) as Style);
        }

        public int FinalValue
        {
            get { return (int)GetValue(FinalValueProperty); }
            set { SetValue(FinalValueProperty, value); }
        }

        public static readonly DependencyProperty FinalValueProperty =
            DependencyProperty.Register(
                "FinalValue", typeof(int), typeof(IgnoreDeltaSlider),
                new FrameworkPropertyMetadata(0,
                    FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnFinalValueChanged));

        private static void OnFinalValueChanged(DependencyObject sender, DependencyPropertyChangedEventArgs e)
        {
            int result;
            if (int.TryParse(e.NewValue.ToString(), out result))
            {
                if (Math.Abs(((IgnoreDeltaSlider)sender).Value - result) > 0.01)
                {
                    ((IgnoreDeltaSlider)sender).Value = result;
                }
            }
        }

        public bool IsDragging { get; protected set; }
        protected override void OnThumbDragCompleted(DragCompletedEventArgs e)
        {
            IsDragging = false;
            base.OnThumbDragCompleted(e);
            OnValueChanged(Value, Value);
            DragCompletedEvent?.Invoke(this, EventArgs.Empty);
        }

        protected override void OnPreviewMouseLeftButtonUp(MouseButtonEventArgs e)
        {
            base.OnPreviewMouseLeftButtonUp(e);
            if (!IsDragging)
            {
                OnValueChanged(Value, Value);
                DragCompletedEvent?.Invoke(this, EventArgs.Empty);
            }
        }

        protected override void OnThumbDragStarted(DragStartedEventArgs e)
        {
            IsDragging = true;
            base.OnThumbDragStarted(e);
        }

        private bool _isUpdatingLimit = false;
        protected override void OnMaximumChanged(double oldMaximum, double newMaximum)
        {
            _isUpdatingLimit = true;
            base.OnMaximumChanged(oldMaximum, newMaximum);
        }

        protected override void OnMinimumChanged(double oldMinimum, double newMinimum)
        {
            _isUpdatingLimit = true;
            base.OnMinimumChanged(oldMinimum, newMinimum);
        }

        protected override void OnValueChanged(double oldValue, double newValue)
        {
            if (_isUpdatingLimit)
            {
                _isUpdatingLimit = false;
                return;
            }
            if (!IsDragging)
            {
                base.OnValueChanged(oldValue, newValue);
                if (FinalValue != (int)Math.Round(Value, 0))
                {
                    FinalValue = (int)Math.Round(Value, 0);
                }
            }
        }
    }

}

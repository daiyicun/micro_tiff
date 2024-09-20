using System.Windows.Controls;
using System.Windows.Input;

namespace ImageReviewTool.ImageTool.Tools
{
    public abstract class ToolBase : ICloneable
    {
        public bool IsHandleMouseEvents { get; set; } = false;
        public virtual void MouseMove(object sender, MouseEventArgs e)
        {
            if (IsHandleMouseEvents)
                e.Handled = IsHandleMouseEvents;
        }
        public virtual void MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (IsHandleMouseEvents)
                e.Handled = IsHandleMouseEvents;
        }
        public virtual void MouseUp(object sender, MouseButtonEventArgs e)
        {
            if (IsHandleMouseEvents)
                e.Handled = IsHandleMouseEvents;
        }
        public virtual void MouseWheel(object sender, MouseWheelEventArgs e)
        {
            if (IsHandleMouseEvents)
                e.Handled = IsHandleMouseEvents;
        }
        public virtual void MouseLeave(object sender, MouseEventArgs e) { }
        public abstract Cursor GetCursor();
        public ToolBase Clone()
        {
            return (ToolBase)(this as ICloneable).Clone();
        }
        object ICloneable.Clone()
        {
            return MemberwiseClone();
        }

        protected ToolTip _descriptionToolTip;
        public ToolTip DescriptionToolTip { get { return _descriptionToolTip; } }
    }
}

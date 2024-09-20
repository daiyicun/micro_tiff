namespace ImageReviewTool.ImageTool.Materials
{
    public abstract class GraphicViewModelBase : BindableBase, ICloneable
    {
        public virtual object Clone()
        {
            return MemberwiseClone();
        }
    }
}

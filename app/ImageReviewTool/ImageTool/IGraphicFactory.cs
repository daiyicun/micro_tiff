using ImageReviewTool.ImageTool.Materials;
using ImageReviewTool.ImageTool.Products;

namespace ImageReviewTool.ImageTool
{
    public interface IGraphicFactory
    {
        GraphicBase GetGraphic(GraphicViewModelBase vm);
    }
}

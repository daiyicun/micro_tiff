using ImageReviewTool.ImageTool.Materials;
using ImageReviewTool.ImageTool.Products;

namespace ImageReviewTool.ImageTool
{
    public class GraphicFactory : IGraphicFactory
    {
        public virtual GraphicBase GetGraphic(GraphicViewModelBase vm)
        {
            if (vm is ImageViewModel imageViewModel)
            {
                return new GraphicImage { Image = imageViewModel.Image, Rect = imageViewModel.Rect, DataContext = imageViewModel };
            }
            return null;
        }
    }
}

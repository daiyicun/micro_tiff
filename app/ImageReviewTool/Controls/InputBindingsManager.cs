using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Media;

namespace ImageReviewTool.Controls
{
    public static class InputBindingsManager
    {

        public static readonly DependencyProperty UpdatePropertySourceWhenEnterPressedProperty = DependencyProperty.RegisterAttached(
                "UpdatePropertySourceWhenEnterPressed", typeof(DependencyProperty), typeof(InputBindingsManager), new PropertyMetadata(null, OnUpdatePropertySourceWhenEnterPressedPropertyChanged));

        static InputBindingsManager()
        {

        }

        public static void SetUpdatePropertySourceWhenEnterPressed(DependencyObject dp, DependencyProperty value)
        {
            dp.SetValue(UpdatePropertySourceWhenEnterPressedProperty, value);
        }

        public static DependencyProperty GetUpdatePropertySourceWhenEnterPressed(DependencyObject dp)
        {
            return (DependencyProperty)dp.GetValue(UpdatePropertySourceWhenEnterPressedProperty);
        }

        private static void OnUpdatePropertySourceWhenEnterPressedPropertyChanged(DependencyObject dp, DependencyPropertyChangedEventArgs e)
        {
            UIElement element = dp as UIElement;

            if (element == null)
            {
                return;
            }

            if (e.OldValue != null)
            {
                element.PreviewKeyDown -= HandlePreviewKeyDown;
            }

            if (e.NewValue != null)
            {
                element.PreviewKeyDown += HandlePreviewKeyDown;
            }
        }

        static void HandlePreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                DoUpdateSource(e.Source);

                var element = (e.OriginalSource as UIElement);
                if (element.IsFocused)
                {
                    var parent = VisualTreeHelperExtension.FindParent(element, CheckParentFocusable);
                    if (parent != null)
                    {
                        (parent as UIElement).Focus();
                        Keyboard.ClearFocus();
                    }
                }
            }
        }

        private static bool CheckParentFocusable(DependencyObject obj)
        {
            if (obj is ContentControl element && element.Focusable)
            {
                return true;
            }
            return false;
        }

        static void DoUpdateSource(object source)
        {
            if (source is DependencyObject obj)
            {
                var dp = GetUpdatePropertySourceWhenEnterPressed(obj);
                if (dp != null)
                {
                    BindingExpression binding = BindingOperations.GetBindingExpression(obj, dp);
                    binding?.UpdateSource();
                }
            }
        }
    }

    public static class VisualTreeHelperExtension
    {
        public static DependencyObject FindParent(DependencyObject currentObj, Func<DependencyObject, bool> checkMethod)
        {
            var parent = VisualTreeHelper.GetParent(currentObj);
            if (parent == null)
                return null;
            if (checkMethod(parent))
                return parent;
            else
                return FindParent(parent, checkMethod);
        }
    }
}

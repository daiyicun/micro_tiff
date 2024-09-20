using System.Globalization;
using System.Windows;
using System.Windows.Data;

namespace ImageReviewTool.Converters
{
    public class NullOrEmptyToBoolConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            var result = false;
            do
            {
                if (object.Equals(value, null))
                    break;

                if (value is uint uintValue && uintValue <= 1)
                    break;

                if (value is IEnumerable<object> enumerable && !enumerable.Any())
                    break;

                result = true;
            } while (false);

            if (targetType == typeof(bool))
                return result;
            if (targetType == typeof(Visibility))
                return result ? Visibility.Visible : Visibility.Collapsed;

            return value;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}

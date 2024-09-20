using System.Globalization;
using System.Windows.Data;

namespace ImageReviewTool.Converters
{
    public class CountToMaxIndexConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is uint uintValue)
            {
                return uintValue - 1;
            }
            return 0;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}

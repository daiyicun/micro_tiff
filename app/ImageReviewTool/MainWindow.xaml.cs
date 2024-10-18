using System.Windows;
using System.Windows.Controls;

namespace ImageReviewTool
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private readonly System.Timers.Timer timer = new(200) { AutoReset = true };
        public MainWindow()
        {
            InitializeComponent();
            this.DataContext = new MainWindowViewModel();
            SizeChanged += MainWindow_SizeChanged;
            timer.Elapsed += Timer_Elapsed;
        }

        private void Timer_Elapsed(object sender, System.Timers.ElapsedEventArgs e)
        {
            Application.Current.Dispatcher.Invoke(()=> ReviewCanvas.Coordinate.AutoFit());
            timer.Enabled = false;
        }

        private void MainWindow_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            timer.Enabled = false;
            timer.Enabled = true;
        }

        private void Slider_MouseWheel(object sender, System.Windows.Input.MouseWheelEventArgs e)
        {
            if (sender is Slider slider)
            {
                if (e.Delta > 0)
                {
                    slider.Value += slider.SmallChange;
                }
                else
                {
                    slider.Value -= slider.SmallChange;
                }
            }
        }
    }
}
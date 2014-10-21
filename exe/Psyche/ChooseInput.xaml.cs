using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using PsycheInterop;
using System.Threading;

namespace Psyche
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class ChooseInput : Window
    {
        public ChooseInput()
        {
            InitializeComponent();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            new Thread(EnumerateCameras).Start();
        }

        private void EnumerateCameras()
        {
            Thread.CurrentThread.IsBackground = true;
            int i = 0;
            while (true)
            {
                using (Capture c = new Capture(i))
                {
                    var frame = c.GetNextFrame();

                    if (frame.Width == 0)
                        break;

                    var b = frame.CreateWriteableBitmap();
                    frame.UpdateWriteableBitmap(b);
                    b.Freeze();

                    Dispatcher.Invoke(() => {
                        int idx = i;
                        Image img = new Image();
                        img.Source = b;

                        Button btn = new Button();
                        btn.Content = img;
                        btn.Padding = new Thickness(10);
                        btn.Width = 200;
                        btn.Height = 200;

                        btn.Click += (s, e) => {
                            Console.WriteLine("Select camera " + idx);
                            MainWindow window = new MainWindow(idx);
                            window.Show();
                            Close();
                        };

                        camerasPanel.Children.Add(btn);
                    });

                }
                i++;
            }
        }
    }
}

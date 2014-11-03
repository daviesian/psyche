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
            //MainWindow window = new MainWindow(1);
            //window.Show();
            //Close();

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
                        img.Margin = new Thickness(30);
                        camerasPanel.ColumnDefinitions.Add(new ColumnDefinition());
                        img.SetValue(Grid.ColumnProperty, camerasPanel.Children.Count-1);
                        img.SetValue(Grid.RowProperty, 1);
                        camerasPanel.Children.Add(img);

                        img.MouseDown += (s, e) => {
                            Console.WriteLine("Select camera " + idx);
                            MainWindow window = new MainWindow(idx);
                            window.Show();
                            Close();
                        };
                    });

                }
                i++;
            }

            Dispatcher.Invoke(() =>
            {
                // If there's only one camera, just go ahead and use it.
                if (i == 1)
                {
                    MainWindow window = new MainWindow(0);
                    window.Show();
                    Close();
                }
            });
        }
    }
}

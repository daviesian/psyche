using OpenCVWrappers;
using PsycheInterop;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace Psyche
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private Capture capture;
        private WriteableBitmap latestImg;

        public MainWindow(int device)
        {
            InitializeComponent();
            capture = new Capture(device);

            new Thread(CaptureLoop).Start();
        }

        public MainWindow(string videoFile)
        {
            InitializeComponent();
            throw new MissingMethodException("Cannot open video files yet");
        }

        private void CaptureLoop()
        {
            Thread.CurrentThread.IsBackground = true;

            while (true)
            {
                var frame = capture.GetNextFrame();

                Dispatcher.Invoke(() =>
                {
                    if (latestImg == null)
                        latestImg = frame.CreateWriteableBitmap();

                    frame.UpdateWriteableBitmap(latestImg);

                    captureView.Source = latestImg;
                });

            }
        }
    }
}

using OpenCVWrappers;
using PsycheInterop;
using PsycheInterop.CLMTracker;
using System;
using System.Collections.Generic;
using System.Diagnostics;
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

        #region High-Resolution Timing
        static DateTime startTime;
        static Stopwatch sw = new Stopwatch();
        static MainWindow()
        {
            startTime = DateTime.Now;
            sw.Start();
        }
        public static DateTime CurrentTime
        {
            get { return startTime + sw.Elapsed; }
        }
        #endregion


        private Capture capture;
        private WriteableBitmap latestImg;
        private bool reset = false;
        Point? resetPoint = null;

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

            CLMParameters clmParams = new CLMParameters();
            CLM clmModel = new CLM();
            float fx = 500, fy = 500, cx = 0, cy = 0;

            FaceAnalyser analyser = new FaceAnalyser();

            DateTime? startTime = CurrentTime;

            arousalPlot.AssocColor(0, Colors.Red);
            valencePlot.AssocColor(0, Colors.Blue);

            while (true)
            {

                //////////////////////////////////////////////
                // CAPTURE FRAME AND DETECT LANDMARKS
                //////////////////////////////////////////////

                var frame = capture.GetNextFrame();

                var grayFrame = capture.GetCurrentFrameGray();

                if (grayFrame == null)
                    continue;

                if (!startTime.HasValue)
                    startTime = CurrentTime;

                if (cx == 0 && cy == 0)
                {
                    cx = grayFrame.Width / 2f;
                    cy = grayFrame.Height / 2f;
                }

                if (reset)
                {
                    clmModel.Reset();
                    analyser.Reset();
                    reset = false;
                }

                if (resetPoint.HasValue)
                {
                    clmModel.Reset(resetPoint.Value.X, resetPoint.Value.Y);
                    analyser.Reset();
                    resetPoint = null;
                }

                bool detected = clmModel.DetectLandmarksInVideo(grayFrame, clmParams);

                if (detected)
                {
                    // TODO: Do this drawing in WPF, it'll look nicer.
                    clmModel.Draw(frame);
                    clmModel.DrawBox(frame, 0, 0, 255, 2, fx, fy, cx, cy);
                }

                //////////////////////////////////////////////
                // Analyse frame and detect AUs
                //////////////////////////////////////////////

                analyser.AddNextFrame(grayFrame, clmModel, (CurrentTime - startTime.Value).TotalSeconds);

                Dictionary<String, double> aus = analyser.GetCurrentAUs();
                double arousal = analyser.GetCurrentArousal();
                double valence = analyser.GetCurrentValence();
                double confidence = analyser.GetConfidence();


                try
                {
                    Dispatcher.Invoke(() =>
                    {
                        if (latestImg == null)
                            latestImg = frame.CreateWriteableBitmap();
                        
                        frame.UpdateWriteableBitmap(latestImg);

                        video.Source = latestImg;

                        Dictionary<int, double> arousalDict = new Dictionary<int, double>();
                        arousalDict[0] = arousal*0.5 + 0.5;
                        arousalPlot.AddDataPoint(new DataPoint() { Time = CurrentTime, values = arousalDict, Confidence = confidence });

                        Dictionary<int, double> valenceDict = new Dictionary<int, double>();
                        valenceDict[0] = valence * 0.5 + 0.5;
                        valencePlot.AddDataPoint(new DataPoint() { Time = CurrentTime, values = valenceDict, Confidence = confidence });

                        Dictionary<int, double> avDict = new Dictionary<int, double>();
                        avDict[0] = valence;
                        avDict[1] = arousal;
                        avPlot.AddDataPoint(new DataPoint() { Time = CurrentTime, values = avDict, Confidence = confidence });

                        auGraph.Update(aus, confidence);
                    });
                }
                catch (TaskCanceledException)
                {
                    // Quitting
                    break;
                }
            }
        }

        private void QuitButton_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }

        private void ResetButton_Click(object sender, RoutedEventArgs e)
        {
            reset = true;
        }

        private void video_MouseDown(object sender, MouseButtonEventArgs e)
        {
            var clickPos = e.GetPosition(video);

            resetPoint = new Point(clickPos.X / video.ActualWidth, clickPos.Y / video.ActualHeight);
        }

    }
}

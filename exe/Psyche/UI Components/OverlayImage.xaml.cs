﻿using System;
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

namespace Psyche
{
    /// <summary>
    /// Interaction logic for OverlayImage.xaml
    /// </summary>
    public partial class OverlayImage : Image
    {
        public OverlayImage()
        {
            InitializeComponent();
            OverlayLines = new List<Tuple<Point, Point>>();
            OverlayPoints = new List<Point>();
        }

        protected override void OnRender(DrawingContext dc)
        {
            base.OnRender(dc);

            if (OverlayLines == null)
                OverlayLines = new List<Tuple<Point, Point>>();

            if (OverlayPoints == null)
                OverlayPoints = new List<Point>();

            if (Source == null || !(Source is WriteableBitmap))
                return;

            var width = ((WriteableBitmap)Source).PixelWidth;
            var height = ((WriteableBitmap)Source).PixelHeight;

            foreach (var line in OverlayLines)
            {

                var p1 = new Point(ActualWidth * line.Item1.X / width, ActualHeight * line.Item1.Y / height);
                var p2 = new Point(ActualWidth * line.Item2.X / width, ActualHeight * line.Item2.Y / height);

                dc.DrawLine(new Pen(new SolidColorBrush(Color.FromArgb(200, (byte)(100 + (155 * (1-Confidence))), (byte)(100 + (155 * Confidence)), 100)), 2), p1, p2);
            } 
            
            foreach (var p in OverlayPoints)
            {

                var q = new Point(ActualWidth * p.X / width, ActualHeight * p.Y / height);

                dc.DrawEllipse(new SolidColorBrush(Color.FromArgb((byte)(200*Confidence),255,255,100)), null, q, 2, 2);
            }
        }

        public List<Tuple<Point, Point>> OverlayLines { get; set; }
        public List<Point> OverlayPoints { get; set; }

        public double Confidence { get; set; }
    }
}

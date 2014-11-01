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

namespace Psyche
{
    /// <summary>
    /// Interaction logic for BarGraph.xaml
    /// </summary>
    public partial class BarGraph : UserControl
    {
        public BarGraph()
        {
            InitializeComponent();
        }

        public void SetValue(double value)
        {
            bar.Height = bar.Height * 0.9 + (barContainer.ActualHeight * value) * 0.1;
        }

        public void SetConfidence(double confidence)
        {
            
            Color bTransparent = Colors.CadetBlue;
            bTransparent.A = 0;

            GradientStopCollection gs = new GradientStopCollection();
            gs.Add(new GradientStop(bTransparent, 0));
            gs.Add(new GradientStop(Colors.CadetBlue, 0.5));
            LinearGradientBrush g = new LinearGradientBrush(gs, new Point(0, 0), new Point(0, ActualHeight * (1 - confidence)));
            g.MappingMode = BrushMappingMode.Absolute;
            g.Freeze();
            bar.Fill = g;
        }

        public string Title
        {
            get { return (string)lblTitle.Content; }
            set { lblTitle.Content = value; }
        }
    }
}

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Psyche
{
    class FpsTracker
    {
        public TimeSpan HistoryLength { get; set; }
        public FpsTracker()
        {
            HistoryLength = TimeSpan.FromSeconds(2);
        }

        private Queue<DateTime> frameTimes = new Queue<DateTime>();

        private void DiscardOldFrames()
        {
            while (frameTimes.Count > 0 && (TimeSeriesPlot.CurrentTime - frameTimes.Peek()) > HistoryLength)
                frameTimes.Dequeue();
        }

        public void AddFrame()
        {
            frameTimes.Enqueue(TimeSeriesPlot.CurrentTime);
            DiscardOldFrames();
        }

        public double GetFPS()
        {
            DiscardOldFrames();

            if (frameTimes.Count == 0)
                return 0;

            return frameTimes.Count / (TimeSeriesPlot.CurrentTime - frameTimes.Peek()).TotalSeconds;
        }
    }
}

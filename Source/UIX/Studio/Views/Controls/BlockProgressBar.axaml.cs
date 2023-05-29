using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Avalonia.Media;
using Avalonia.Threading;

namespace Studio.Views.Controls
{
    public partial class BlockProgressBar : UserControl
    {
        /// <summary>
        /// Current job counter
        /// </summary>
        public int JobCount
        {
            get => _jobPeak;
            set
            {
                // Update peak
                _jobPeak = Math.Max(_jobPeak, value);

                // Reset peak, if the value exceeds the last count, this is a re-peak
                if (value == 0 || value > _jobCount)
                    _jobPeak = value;
                
                // Set value
                _jobCount = value;
                
                // Force redraw
                this.InvalidateMeasure();
                this.InvalidateArrange();
            }
        }

        public BlockProgressBar()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }

        /// <summary>
        /// Invoked on UI control arrangement
        /// </summary>
        /// <param name="finalSize"></param>
        /// <returns></returns>
        protected override Size ArrangeOverride(Size finalSize)
        {
            // Cache the size
            _size = finalSize;
            return base.ArrangeOverride(finalSize);
        }

        public override void Render(DrawingContext context)
        {
            // Standard number of blocks
            const int maxBlockY = 4;
            const int maxBlockX = 20;
            const int maxBlocks = maxBlockX * maxBlockY;

            // Default pixel padding
            const int pad = 1;

            // Dimensions of each block
            double blockWidth = _size.Width / maxBlockX - pad;
            double blockHeight = _size.Height / maxBlockY - pad;

            // Current completion
            float normalized = _jobCount / ((float)_jobPeak);

            // Draw all blocks
            for (int i = 0; i < maxBlocks; i++)
            {
                bool isIncomplete = i < (int)(maxBlocks * normalized) && i < _jobCount;
                
                // Position of block
                int x = (maxBlockX - 1) - i / maxBlockY;
                int y = i % maxBlockY;
                
                context.FillRectangle(isIncomplete ? _brushIncomplete : _brushCompleted, new Rect(
                    pad / 2.0 + x * (blockWidth + pad),
                    pad / 2.0 + y * (blockHeight + pad),
                    blockWidth,
                    blockHeight
                ));
            }
        }

        /// <summary>
        /// Cached size
        /// </summary>
        private Size _size;
        
        /// <summary>
        /// Default brushes
        /// </summary>
        private SolidColorBrush? _brushIncomplete = ResourceLocator.GetResource<SolidColorBrush>("DockApplicationAccentBrushHigh");
        private SolidColorBrush? _brushCompleted = ResourceLocator.GetResource<SolidColorBrush>("DockApplicationAccentBrushLow");

        /// <summary>
        /// Current number of jobs
        /// </summary>
        private int _jobCount = 0;

        /// <summary>
        /// Current peak number of jobs
        /// </summary>
        private int _jobPeak = 0;
    }
}
// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

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
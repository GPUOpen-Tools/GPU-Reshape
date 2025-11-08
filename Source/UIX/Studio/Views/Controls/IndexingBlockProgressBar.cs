// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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
using Avalonia.Media;
using ReactiveUI;
using Studio.ViewModels.Status;

namespace Studio.Views.Controls
{
    public class IndexingBlockProgressBar : Control
    {
        /// <summary>
        /// View model
        /// </summary>
        public IndexingStatusViewModel ViewModel
        {
            get => _viewModel;
            set
            {
                _viewModel = value;
                _viewModel.WhenAnyValue(x => x.JobCount).Subscribe(OnJobCount);
            }
        }

        public IndexingBlockProgressBar()
        {
            AffectsRender<IndexingBlockProgressBar>(BoundsProperty);
        }

        /// <summary>
        /// Invoked on UI control arrangement
        /// </summary>
        protected override Size ArrangeOverride(Size finalSize)
        {
            // Cache the size
            _size = finalSize;
            return base.ArrangeOverride(finalSize);
        }

        /// <summary>
        /// Invoked on renders
        /// </summary>
        public override void Render(DrawingContext context)
        {
            base.Render(context);
            
            // Dimensions of each block
            double blockWidth = _size.Width / MaxBlockX - BlockPadding;
            double blockHeight = _size.Height / MaxBlockY - BlockPadding;

            // Current completion
            float normalized = _jobCount / ((float)_jobPeak);

            // Draw all blocks
            for (int i = 0; i < MaxBlocks; i++)
            {
                bool isIncomplete = i < (int)(MaxBlocks * normalized) && i < _jobCount;
                
                // Position of block
                int x = (MaxBlockX - 1) - i / MaxBlockY;
                int y = i % MaxBlockY;

                context.FillRectangle(isIncomplete ? _brushIncomplete : _brushCompleted, new Rect(
                    BlockPadding / 2.0 + x * (blockWidth + BlockPadding),
                    BlockPadding / 2.0 + y * (blockHeight + BlockPadding),
                    blockWidth,
                    blockHeight
                ));
            }
        }

        /// <summary>
        /// Invoked on job count updates
        /// </summary>
        public void OnJobCount(int value)
        {
            // Update peak
            _jobPeak = Math.Max(_jobPeak, value);

            // Reset peak, if the value exceeds the last count, this is a re-peak
            if (value == 0 || value > _jobCount)
            {
                _jobPeak = value;
            }
                
            // Set value
            _jobCount = value;
                
            // Force redraw
            this.InvalidateMeasure();
            this.InvalidateArrange();
        }

        /// <summary>
        /// Cached size
        /// </summary>
        private Size _size;
        
        /// <summary>
        /// Default brushes
        /// </summary>
        private IBrush? _brushIncomplete = ResourceLocator.GetBrush("InfoMediumLowForeground");
        private IBrush? _brushCompleted  = ResourceLocator.GetBrush("DockApplicationAccentBrushLow");

        /// <summary>
        /// Current number of jobs
        /// </summary>
        private int _jobCount = 0;

        /// <summary>
        /// Current peak number of jobs
        /// </summary>
        private int _jobPeak = 0;

        /// <summary>
        /// Internal view model
        /// </summary>
        private IndexingStatusViewModel _viewModel;
        
        /// <summary>
        /// Maximum number of blocks vertically
        /// </summary>
        private const int MaxBlockY = 4;
        
        /// <summary>
        /// Maximum number of blocks horizontally
        /// </summary>
        private const int MaxBlockX = 20;
        
        /// <summary>
        /// Maximum number of blocks
        /// </summary>
        private const int MaxBlocks = MaxBlockX * MaxBlockY;
        
        /// <summary>
        /// Block padding
        /// </summary>
        private const int BlockPadding = 1;
    }
}
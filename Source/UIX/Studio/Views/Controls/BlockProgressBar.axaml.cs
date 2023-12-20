// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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
using System.Collections.Generic;
using System.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Avalonia.Media;
using ReactiveUI;
using Studio.Models.Diagnostic;
using Studio.ViewModels.Status;

namespace Studio.Views.Controls
{
    public partial class BlockProgressBar : UserControl
    {
        /// <summary>
        /// View model
        /// </summary>
        public InstrumentationStatusViewModel ViewModel
        {
            get => _viewModel;
            set
            {
                _viewModel = value;
                _viewModel.WhenAnyValue(x => x.JobCount).Subscribe(OnJobCount);
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

        /// <summary>
        /// Invoked on renders
        /// </summary>
        public override void Render(DrawingContext context)
        {
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

                // Select pending brush
                SolidColorBrush? pendingBlock = i < _brushes.Count ? _brushes[i] : _brushIncomplete;

                context.FillRectangle(isIncomplete ? pendingBlock : _brushCompleted, new Rect(
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
                
                // Remove previous brush distribution
                _brushes.Clear();
                
                // Re-distribute the colors
                if (ViewModel.Stage == InstrumentationStage.Pipeline)
                {
                    Distribute();
                }
            }
                
            // Set value
            _jobCount = value;
                
            // Force redraw
            this.InvalidateMeasure();
            this.InvalidateArrange();
        }

        /// <summary>
        /// Perform pseudo distribution
        /// </summary>
        private void Distribute()
        {
            // Limit block count
            int blockCount = Math.Min(MaxBlocks, ViewModel.JobCount);

            // Sum the job counts for normalization
            int sourcedJobCount = ViewModel.GraphicsCount + ViewModel.ComputeCount;
            if (sourcedJobCount == 0)
            {
                return;
            }

            // Normalize by stage
            int normalizedGraphics = (int)(ViewModel.GraphicsCount / (float)sourcedJobCount * blockCount);
            int normalizedCompute  = (int)(ViewModel.ComputeCount / (float)sourcedJobCount * blockCount);

            // Safety bound to normalized job count
            int totalNormalized = normalizedGraphics + normalizedCompute;
            normalizedGraphics += blockCount - totalNormalized;
            
            // Initial distribution
            _brushes.AddRange(Enumerable.Repeat(_brushGraphics, normalizedGraphics));
            _brushes.AddRange(Enumerable.Repeat(_brushCompute, normalizedCompute));

            // Ordering queries keys exactly once, sorting guaranteed to exit
            _brushes = _brushes.OrderBy(x => _random.Next()).ToList();
        }

        /// <summary>
        /// Cached size
        /// </summary>
        private Size _size;
        
        /// <summary>
        /// Default brushes
        /// </summary>
        private SolidColorBrush? _brushIncomplete = ResourceLocator.GetResource<SolidColorBrush>("DockApplicationAccentBrushHigh");
        private SolidColorBrush? _brushCompleted  = ResourceLocator.GetResource<SolidColorBrush>("DockApplicationAccentBrushLow");
        private SolidColorBrush? _brushGraphics   = ResourceLocator.GetResource<SolidColorBrush>("InstrumentationStageGraphics");
        private SolidColorBrush? _brushCompute    = ResourceLocator.GetResource<SolidColorBrush>("InstrumentationStageCompute");

        /// <summary>
        /// Current number of jobs
        /// </summary>
        private int _jobCount = 0;

        /// <summary>
        /// Current peak number of jobs
        /// </summary>
        private int _jobPeak = 0;

        /// <summary>
        /// Pending job distribution
        /// </summary>
        private List<SolidColorBrush?> _brushes = new();
        
        /// <summary>
        /// Random device
        /// </summary>
        private static Random _random = new();

        /// <summary>
        /// Internal view model
        /// </summary>
        private InstrumentationStatusViewModel _viewModel;
        
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
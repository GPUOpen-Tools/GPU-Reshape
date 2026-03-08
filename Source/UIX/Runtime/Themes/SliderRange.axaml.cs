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
using Avalonia.Controls.Primitives;
using Avalonia.Input;

namespace Studio.Views.Controls
{
    public partial class SliderRange : UserControl
    {
        /// <summary>
        /// Minimum value property
        /// </summary>
        public static readonly StyledProperty<float> MinimumProperty = AvaloniaProperty.Register<SliderRange, float>(nameof(Minimum));

        /// <summary>
        /// Maximum value property
        /// </summary>
        public static readonly StyledProperty<float> MaximumProperty = AvaloniaProperty.Register<SliderRange, float>(nameof(Maximum), 1.0f);

        /// <summary>
        /// Current lower value property
        /// </summary>
        public static readonly StyledProperty<float> LowerValueProperty = AvaloniaProperty.Register<SliderRange, float>(nameof(LowerValue));

        /// <summary>
        /// Current upper value property
        /// </summary>
        public static readonly StyledProperty<float> UpperValueProperty = AvaloniaProperty.Register<SliderRange, float>(nameof(UpperValue), 1.0f);

        /// <summary>
        /// Minimum value
        /// </summary>
        public float Minimum
        {
            get => GetValue(MinimumProperty);
            set => SetValue(MinimumProperty, value);
        }

        /// <summary>
        /// Maximum value
        /// </summary>
        public float Maximum
        {
            get => GetValue(MaximumProperty);
            set => SetValue(MaximumProperty, value);
        }

        /// <summary>
        /// Current lower value
        /// </summary>
        public float LowerValue
        {
            get => GetValue(LowerValueProperty);
            set => SetValue(LowerValueProperty, value);
        }

        /// <summary>
        /// Current upper value
        /// </summary>
        public float UpperValue
        {
            get => GetValue(UpperValueProperty);
            set => SetValue(UpperValueProperty, value);
        }

        /// <summary>
        /// Invoked on template apply
        /// </summary>
        protected override void OnApplyTemplate(TemplateAppliedEventArgs e)
        {
            base.OnApplyTemplate(e);
            
            // Get parts
            _partTrack = e.NameScope.Find<Border>("PART_Track")!;
            _partSelectedTrackRange = e.NameScope.Find<Border>("PART_SelectedTrackRange")!;
            _partLowerThumb = e.NameScope.Find<Thumb>("PART_LowerThumb")!;
            _partUpperThumb = e.NameScope.Find<Thumb>("PART_UpperThumb")!;

            // Subscribe to dragging
            _partLowerThumb.DragDelta += OnLowerThumbDrag;
            _partUpperThumb.DragDelta += OnUpperThumbDrag;

            // Bind to any property change
            this.GetObservable(BoundsProperty).Subscribe(_ => UpdateRange());
            this.GetObservable(LowerValueProperty).Subscribe(_ => UpdateRange());
            this.GetObservable(UpperValueProperty).Subscribe(_ => UpdateRange());
            this.GetObservable(MinimumProperty).Subscribe(_ => UpdateRange());
            this.GetObservable(MaximumProperty).Subscribe(_ => UpdateRange());
        }

        /// <summary>
        /// Invoked on dragging
        /// </summary>
        void OnLowerThumbDrag(object? s, VectorEventArgs e)
        {
            // Update margin
            double left = Math.Max(0, Math.Min(Bounds.Width - _partUpperThumb.Bounds.Width, _partLowerThumb.Margin.Left + e.Vector.X));
            _partLowerThumb.Margin = new Thickness(left, 0, 0, 0);

            _reEntryGuard = true;
            
            // Recompute the lower value
            double w = _partLowerThumb.Margin.Left / (Bounds.Width - _partLowerThumb.Bounds.Width);
            LowerValue = (float)(Minimum + w * (Maximum - Minimum));

            // Handle case where thumb > max
            if (_partLowerThumb.Margin.Left > _partUpperThumb.Margin.Left)
            {
                _partUpperThumb.Margin = _partLowerThumb.Margin;
            }
            
            UpdateSelectedRange();
            _reEntryGuard = false;
        }

        /// <summary>
        /// Invoked on dragging
        /// </summary>
        void OnUpperThumbDrag(object? s, VectorEventArgs e)
        {
            // Update margin
            double left = Math.Max(0, Math.Min(Bounds.Width - _partUpperThumb.Bounds.Width, _partUpperThumb.Margin.Left + e.Vector.X));
            _partUpperThumb.Margin = new Thickness(left, 0, 0, 0);

            _reEntryGuard = true;

            // Recompute the upper value
            double w = _partUpperThumb.Margin.Left / (Bounds.Width - _partUpperThumb.Bounds.Width);
            UpperValue = (float)(Minimum + w * (Maximum - Minimum));
            
            // Handle case where thumb < min
            if (_partUpperThumb.Margin.Left < _partLowerThumb.Margin.Left)
            {
                _partLowerThumb.Margin = _partUpperThumb.Margin;
            }
            
            UpdateSelectedRange();
            
            _reEntryGuard = false;
        }

        /// <summary>
        /// Update all range layouts
        /// </summary>
        void UpdateRange()
        {
            if (_reEntryGuard)
            {
                return;
            }

            // Safe denom
            double denom = Maximum - Minimum;
            if (denom < 1e-6f)
            {
                denom = 1;
            }
            
            // Compute the new weights
            double lowWeight = Math.Max(0, Math.Min(1, (LowerValue - Minimum) / denom));
            double highWeight = Math.Max(0, Math.Min(1, (UpperValue - Minimum) / denom));

            // Update margins from weights
            _partLowerThumb.Margin = new Thickness(Bounds.Width * lowWeight, 0, 0, 0);
            _partUpperThumb.Margin = new Thickness(Bounds.Width * highWeight - _partUpperThumb.Bounds.Width, 0, 0, 0);

            UpdateSelectedRange();
        }

        /// <summary>
        /// Update the selection range between the two thumbs
        /// </summary>
        void UpdateSelectedRange()
        {
            _partSelectedTrackRange.Margin = new Thickness(_partLowerThumb.Margin.Left + _partLowerThumb.Bounds.Width / 2, 0, 0, 0);
            _partSelectedTrackRange.Width = _partUpperThumb.Margin.Left - _partLowerThumb.Margin.Left;
        }

        /// <summary>
        /// Whole track control
        /// </summary>
        private Border _partTrack;
        
        /// <summary>
        /// Current selection track control
        /// </summary>
        private Border _partSelectedTrackRange;
        
        /// <summary>
        /// Lower thumb control
        /// </summary>
        private Thumb _partLowerThumb;
        
        /// <summary>
        /// Upper thumb control
        /// </summary>
        private Thumb _partUpperThumb;

        private bool _reEntryGuard;
    }
}
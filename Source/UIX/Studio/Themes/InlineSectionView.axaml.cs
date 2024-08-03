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

using System.Windows.Input;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Avalonia.Media;
using ReactiveUI;

namespace Studio.Views.Controls
{
    public partial class InlineSectionView : UserControl
    {
        /// <summary>
        /// Title style for the given view
        /// </summary>
        public static readonly StyledProperty<string> TitleProperty = AvaloniaProperty.Register<InlineSectionView, string>(nameof(Title));
        
        /// <summary>
        /// Expansion icon style for the given view
        /// </summary>
        public static readonly StyledProperty<StreamGeometry> ExpansionIconProperty = AvaloniaProperty.Register<InlineSectionView, StreamGeometry>(nameof(ExpansionIcon), ResourceLocator.GetIcon("ChevronUp")!);
        
        /// <summary>
        /// Expansion style for the given view
        /// </summary>
        public static readonly StyledProperty<bool> IsExpandedProperty = AvaloniaProperty.Register<InlineSectionView, bool>(nameof(IsExpanded), true);
        
        /// <summary>
        /// Expansion command style for the given view
        /// </summary>
        public static readonly StyledProperty<ICommand> ExpandCommandProperty = AvaloniaProperty.Register<InlineSectionView, ICommand>(nameof(ExpandCommand));

        /// <summary>
        /// Line border brush style for the given view
        /// </summary>
        public static readonly StyledProperty<IBrush> LineBorderBrushProperty = AvaloniaProperty.Register<InlineSectionView, IBrush>(nameof(LineBorderBrush));

        public InlineSectionView()
        {
            ExpandCommand = ReactiveCommand.Create(OnExpand);
        }

        protected override void OnPropertyChanged(AvaloniaPropertyChangedEventArgs change)
        {
            if (change.Property.Name == nameof(IsExpanded))
            {
                UpdateIcon();
            }
            
            // Pass down
            base.OnPropertyChanged(change);
        }

        /// <summary>
        /// Invoked on expansion
        /// </summary>
        private void OnExpand()
        {
            IsExpanded = !IsExpanded;
        }

        /// <summary>
        /// Update the icon
        /// </summary>
        private void UpdateIcon()
        {
            ExpansionIcon = IsExpanded ? ResourceLocator.GetIcon("ChevronUp")! : ResourceLocator.GetIcon("ChevronDown")!;
            LineBorderBrush = new SolidColorBrush(ResourceLocator.GetResource<Color>("InputBorderColor")!);
        }

        /// <summary>
        /// Title getter / setter
        /// </summary>
        public string Title
        {
            get => GetValue(TitleProperty);
            set => SetValue(TitleProperty, value);
        }
        
        /// <summary>
        /// Icon getter / setter
        /// </summary>
        public StreamGeometry ExpansionIcon
        {
            get => GetValue(ExpansionIconProperty);
            set => SetValue(ExpansionIconProperty, value);
        }
        
        /// <summary>
        /// Expansion getter / setter
        /// </summary>
        public bool IsExpanded
        {
            get => GetValue(IsExpandedProperty);
            set => SetValue(IsExpandedProperty, value);
        }

        /// <summary>
        /// Expand command getter / setter
        /// </summary>
        public ICommand ExpandCommand
        {
            get => GetValue(ExpandCommandProperty);
            set => SetValue(ExpandCommandProperty, value);
        }

        /// <summary>
        /// Line border brush getter / setter
        /// </summary>
        public IBrush LineBorderBrush
        {
            get => GetValue(LineBorderBrushProperty);
            set => SetValue(LineBorderBrushProperty, value);
        }
    }
}
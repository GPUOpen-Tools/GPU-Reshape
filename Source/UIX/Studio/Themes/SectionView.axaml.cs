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
    public partial class SectionView : UserControl
    {
        /// <summary>
        /// Title style for the given view
        /// </summary>
        public static readonly StyledProperty<string> TitleProperty = AvaloniaProperty.Register<SectionView, string>(nameof(Title));
        
        /// <summary>
        /// Expansion icon style for the given view
        /// </summary>
        public static readonly StyledProperty<StreamGeometry> ExpansionIconProperty = AvaloniaProperty.Register<SectionView, StreamGeometry>(nameof(ExpansionIcon), ResourceLocator.GetIcon("ChevronUp")!);
        
        /// <summary>
        /// Expansion style for the given view
        /// </summary>
        public static readonly StyledProperty<bool> IsExpandedProperty = AvaloniaProperty.Register<SectionView, bool>(nameof(IsExpanded), true);
        
        /// <summary>
        /// Inline content style for the given view
        /// </summary>
        public static readonly StyledProperty<bool> InlineContentProperty = AvaloniaProperty.Register<SectionView, bool>(nameof(InlineContent), false);
        
        /// <summary>
        /// Content margin style for the given view
        /// </summary>
        public static readonly StyledProperty<Thickness> ContentMarginProperty = AvaloniaProperty.Register<SectionView, Thickness>(nameof(ContentMargin), new Thickness(0, 35, 0, 0));
        
        /// <summary>
        /// Expansion command style for the given view
        /// </summary>
        public static readonly StyledProperty<ICommand> ExpandCommandProperty = AvaloniaProperty.Register<SectionView, ICommand>(nameof(ExpandCommand));

        public SectionView()
        {
            ExpandCommand = ReactiveCommand.Create(OnExpand);
        }
        
        protected override void OnPropertyChanged(AvaloniaPropertyChangedEventArgs change)
        {
            if (change.Property.Name == nameof(IsExpanded))
            {
                UpdateIcon();
            }

            // Update inlined state
            if (change.Property.Name == nameof(InlineContent))
            {
                if (InlineContent)
                {
                    ContentMargin = new Thickness();
                }
                else
                {
                    ContentMargin = new Thickness(0, 35, 0, 0);
                }
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
        /// Inline getter / setter
        /// </summary>
        public bool InlineContent 
        {
            get => GetValue(InlineContentProperty);
            set => SetValue(InlineContentProperty, value);
        }
        
        /// <summary>
        /// Content margin getter / setter
        /// </summary>
        public Thickness ContentMargin 
        {
            get => GetValue(ContentMarginProperty);
            set => SetValue(ContentMarginProperty, value);
        }

        /// <summary>
        /// Expand command getter / setter
        /// </summary>
        public ICommand ExpandCommand
        {
            get => GetValue(ExpandCommandProperty);
            set => SetValue(ExpandCommandProperty, value);
        }
    }
}
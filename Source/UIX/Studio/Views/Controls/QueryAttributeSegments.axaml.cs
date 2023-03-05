using System;
using System.Collections.Generic;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Markup.Xaml;
using Avalonia.Media.Immutable;
using DynamicData;
using Runtime;
using Runtime.Models.Query;

namespace Studio.Views.Controls
{
    public partial class QueryAttributeSegments : UserControl
    {
        /// <summary>
        /// Source query box
        /// </summary>
        public AutoCompleteBox QueryBox;

        /// <summary>
        /// Source decorators
        /// </summary>
        public SourceList<QueryAttributeDecorator> Decorators
        {
            set
            {
                value.Connect()
                    .OnItemRemoved(OnDecoratorRemoved)
                    .OnItemAdded(OnDecoratorAdded)
                    .Subscribe();
            }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public QueryAttributeSegments()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Invoked on decorator addition
        /// </summary>
        /// <param name="decorator"></param>
        private void OnDecoratorAdded(QueryAttributeDecorator decorator)
        {
            /**
             * NOTE: Workaround for odd internal ItemsControl behaviour with item removal,
             *       added manually instead of using generative models.
             */

            // Measure glyph begin
            double beginWidth = Math.Floor(TextFormatter.GetWidth(QueryBox.Text.Substring(0, decorator.Attribute.Offset), QueryBox.FontSize).Width);

            // Initial text block offset
            if (decorator.Attribute.Offset != 0)
            {
                beginWidth += 5;
            }

            // Measure glyph end
            double endWidth = Math.Floor(TextFormatter.GetWidth(QueryBox.Text.Substring(0, decorator.Attribute.Offset + decorator.Attribute.Length), QueryBox.FontSize).Width) + 5;

            // Create decorator control
            var control = new Border()
            {
                Background = new ImmutableSolidColorBrush(decorator.Color),
                Width = endWidth - beginWidth,
                Margin = new Thickness(beginWidth, 0, 0, 0),
                Height = 1.25,
                HorizontalAlignment = HorizontalAlignment.Left,
                VerticalAlignment = VerticalAlignment.Top
            };

            // Add
            SegmentPanel.Children.Add(control);
            _segmentControls.Add(decorator, control);
        }

        /// <summary>
        /// Invoked on decorator removal
        /// </summary>
        /// <param name="decorator"></param>
        private void OnDecoratorRemoved(QueryAttributeDecorator decorator)
        {
            SegmentPanel.Children.Remove(_segmentControls[decorator]);
            _segmentControls.Remove(decorator);
        }

        /// <summary>
        /// Tracked decorators
        /// </summary>
        private Dictionary<QueryAttributeDecorator, IControl> _segmentControls = new();
    }
}
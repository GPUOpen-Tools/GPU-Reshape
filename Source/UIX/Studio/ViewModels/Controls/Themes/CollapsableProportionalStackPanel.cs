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
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Diagnostics;
using System.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.Presenters;
using Dock.Avalonia.Controls;
using Dock.Model.Controls;
using Dock.Model.Core;
using Dock.Model.ReactiveUI.Controls;
using ReactiveUI;
using Orientation = Avalonia.Layout.Orientation;

namespace Studio.ViewModels.Controls.Themes
{
    /// <summary>
    /// Modified version of ProportionalStackPanel from https://github.com/wieslawsoltes/Dock/
    /// ! To be rewritten under a single coding style
    /// </summary>
    public class CollapsableProportionalStackPanel : ProportionalStackPanel
    {
        public new static readonly StyledProperty<Orientation> OrientationProperty = AvaloniaProperty.Register<CollapsableProportionalStackPanel, Orientation>(nameof(Orientation), Orientation.Vertical);

        /// <summary>
        /// Subscribe to all expansion states
        /// </summary>
        /// <param name="dock"></param>
        private void SubscribeExpansion(IProportionalDock dock)
        {
            foreach (var dockVisibleDockable in dock.VisibleDockables ?? new List<IDockable>())
            {
                switch (dockVisibleDockable)
                {
                    case ToolDock toolDock:
                        toolDock.WhenAnyValue(x => x.IsExpanded).Subscribe(_ => ChildExpansionStateChanged());
                        break;
                    case IProportionalDock proportionalDock:
                        SubscribeExpansion(proportionalDock);
                        break;
                }
            }
        }

        /// <summary>
        /// Invoked on child changes
        /// </summary>
        protected override void ChildrenChanged(object? sender, NotifyCollectionChangedEventArgs e)
        {
            base.ChildrenChanged(sender, e);

            switch (e.Action)
            {
                case NotifyCollectionChangedAction.Add:
                {
                    if (e.NewItems is not null)
                    {
                        foreach (var item in e.NewItems.OfType<Control>())
                        {
                            if (item.DataContext is IProportionalDock dock)
                            {
                                SubscribeExpansion(dock);
                            }
                        }
                    }
                    break;
                }
            }
        }

        /// <summary>
        /// Invoked on expansion changes
        /// </summary>
        private void ChildExpansionStateChanged()
        {
            InvalidateMeasure();
            InvalidateArrange();
        }

        /// <summary>
        /// FInd the closest target from a given element
        /// </summary>
        Control? FindClosestTarget(IList<Control> children, Control element)
        {
            int distance = int.MaxValue;

            int offset = children.IndexOf(element);

            Control? target = null;

            for (int j = offset + 1; j < children.Count; j++)
            {
                int dist = j - offset;
                if (dist < distance && !IsSplitter(children[j]))
                {
                    distance = dist;
                    target = children[j];
                    break;
                }
            }

            for (int j = offset - 1; j >= 0; j--)
            {
                int dist = offset - j;
                if (dist < distance && !IsSplitter(children[j]))
                {
                    distance = dist;
                    target = children[j];
                    break;
                }
            }

            return target;
        }

        /// <summary>
        /// Get the owning splitter
        /// </summary>
        private static ProportionalStackPanelSplitter? GetSplitter(Control? control)
        {
            if (control is ContentPresenter contentPresenter)
            {
                if (contentPresenter.Child is null)
                {
                    contentPresenter.UpdateChild();
                }

                control = contentPresenter.Child;
            }

            return control as ProportionalStackPanelSplitter;
        }

        /// <summary>
        /// Check if a control is a splitter
        /// </summary>
        private static bool IsSplitter(Control? control)
        {
            if (control is ContentPresenter contentPresenter)
            {
                if (contentPresenter.Child is null)
                {
                    contentPresenter.UpdateChild();
                }

                control = contentPresenter.Child;
            }

            return control is ProportionalStackPanelSplitter;
        }

        /// <summary>
        /// Set the proportion of a control
        /// </summary>
        private static void SetControlProportion(Control? control, double proportion)
        {
            if (control is ContentPresenter contentPresenter)
            {
                if (contentPresenter.Child is null)
                {
                    contentPresenter.UpdateChild();
                }

                if (contentPresenter.Child is not null)
                {
                    ProportionalStackPanelSplitter.SetProportion(contentPresenter.Child, proportion);
                }
            }
            else
            {
                if (control is not null)
                {
                    ProportionalStackPanelSplitter.SetProportion(control, proportion);
                }
            }
        }

        /// <summary>
        /// Get the proportion of a control
        /// </summary>
        private static double GetControlProportion(Control? control)
        {
            if (control is ContentPresenter contentPresenter)
            {
                if (contentPresenter.Child is null)
                {
                    contentPresenter.UpdateChild();
                }

                if (contentPresenter.Child is not null)
                {
                    return ProportionalStackPanelSplitter.GetProportion(contentPresenter.Child);
                }

                return double.NaN;
            }

            if (control is not null)
            {
                return ProportionalStackPanelSplitter.GetProportion(control);
            }

            return double.NaN;
        }

        /// <summary>
        /// Assign all proportions
        /// </summary>
        private void AssignProportions(Avalonia.Controls.Controls children, Size size)
        {
            // Handle expansion ranges
            foreach (Control control in children.Where(x => !IsSplitter(x)))
            {
                bool isExpanded = IsExpanded(control);

                // Get expansion state, create if needed
                ExpansionState? state;
                if (!_expansionStates.TryGetValue(control, out state))
                {
                    state = new ExpansionState()
                    {
                        IsExpanded = isExpanded,
                        Proportion = GetControlProportion(control)
                    };

                    _expansionStates.Add(control, state);
                }

                // New expansion state?
                if (state.IsExpanded != isExpanded)
                {
                    Control? target = FindClosestTarget(children, control);

                    // Was previously expanded?
                    if (state.IsExpanded)
                    {
                        // Cache proportion
                        state.Proportion = state.Proportion;

                        // Remeasure
                        control.Measure(size);

                        // Determine effective proportion
                        double proportion = 0;
                        switch (Orientation)
                        {
                            case Orientation.Horizontal:
                                proportion = control.DesiredSize.Width / size.Width;
                                break;

                            case Orientation.Vertical:
                                proportion = control.DesiredSize.Height / size.Height;
                                break;
                        }

                        // Set new proportion
                        Debug.Assert(!double.IsNaN(proportion));
                        SetControlProportion(control, proportion);
                    }
                    else
                    {
                        // Set cached proportion
                        Debug.Assert(!double.IsNaN(state.Proportion));
                        SetControlProportion(control, state.Proportion);
                    }

                    // If there's a target, mark for distribution
                    if (target != null)
                    {
                        SetControlProportion(target, double.NaN);
                    }

                    // Set new expansion state
                    state.IsExpanded = isExpanded;
                }
            }

            // Current proportion
            var assignedProportion = 0.0;
            
            // All unassigned ranges
            List<Tuple<Control, double>> unassigned = new();

            // Summarize range
            for (var i = 0; i < children.Count; i++)
            {
                var element = (Control)children[i];

                if (!IsSplitter(element))
                {
                    var proportion = GetControlProportion(element);

                    if (double.IsNaN(proportion))
                    {
                        unassigned.Add(new Tuple<Control, double>(element, 0));
                    }
                    else
                    {
                        assignedProportion += proportion;
                    }
                }
            }

            // Distribution?
            {
                var toAssign = assignedProportion;
                foreach (var control in unassigned)
                {
                    var element = (Control)control.Item1;
                    if (!IsSplitter(element))
                    {
                        var proportion = (1.0 - toAssign) / unassigned.Count;
                        Debug.Assert(!double.IsNaN(proportion + control.Item2));
                        SetControlProportion(element, proportion + control.Item2);
                        assignedProportion += (1.0 - toAssign) / unassigned.Count;
                    }
                }
            }

            // Expansion or contraction?
            if (assignedProportion < 1)
            {
                var numChildren = (double)children.Count(c => !IsSplitter(c));

                var toAdd = (1.0 - assignedProportion) / numChildren;

                foreach (var child in children.Where(c => !IsSplitter(c)))
                {
                    var proportion = GetControlProportion(child) + toAdd;
                    SetControlProportion(child, proportion);
                }
            }
            else if (assignedProportion > 1)
            {
                var numChildren = (double)children.Count(c => !IsSplitter(c));

                var toRemove = (assignedProportion - 1.0) / numChildren;

                foreach (var child in children.Where(c => !IsSplitter(c)))
                {
                    var proportion = GetControlProportion(child) - toRemove;
                    SetControlProportion(child, proportion);
                }
            }
        }

        /// <summary>
        /// Get the default splitter thickness
        /// </summary>
        private double GetTotalSplitterThickness(IList<Control> children)
        {
            return 0.0;
        }

        /// <summary>
        /// Check if a given control is expanded
        /// </summary>
        /// <param name="control"></param>
        /// <returns></returns>
        private bool IsExpanded(Control control)
        {
            if (control.DataContext is ProportionalDock dock)
            {
                if (dock.VisibleDockables?[0] is ToolDock toolDock)
                {
                    return toolDock.IsExpanded;
                }
            }

            return true;
        }

        /// <summary>
        /// Control measure
        /// </summary>
        protected override Size MeasureOverride(Size constraint)
        {
            var horizontal = Orientation == Orientation.Horizontal;

            if (constraint == Size.Infinity
                || (horizontal && double.IsInfinity(constraint.Width))
                || (!horizontal && double.IsInfinity(constraint.Height)))
            {
                throw new Exception("Proportional StackPanel cannot be inside a control that offers infinite space.");
            }

            var usedWidth = 0.0;
            var usedHeight = 0.0;
            var maximumWidth = 0.0;
            var maximumHeight = 0.0;
            var splitterThickness = GetTotalSplitterThickness(Children);

            AssignProportions(Children, constraint);

            // Measure each of the Children
            foreach (var control in Children)
            {
                var element = (Control)control;
                // Get the child's desired size
                var remainingSize = new Size(
                    Math.Max(0.0, constraint.Width - usedWidth - splitterThickness),
                    Math.Max(0.0, constraint.Height - usedHeight - splitterThickness));

                var proportion = GetControlProportion(element);

                bool isExpanded = IsExpanded(element);

                if (!IsSplitter(element) && isExpanded)
                {
                    if (isExpanded)
                    {
                        Debug.Assert(!double.IsNaN(proportion));

                        switch (Orientation)
                        {
                            case Orientation.Horizontal:
                                element.Measure(constraint.WithWidth(Math.Max(0,
                                    (constraint.Width - splitterThickness) * proportion)));
                                break;

                            case Orientation.Vertical:
                                element.Measure(constraint.WithHeight(Math.Max(0,
                                    (constraint.Height - splitterThickness) * proportion)));
                                break;
                        }
                    }
                    else
                    {
                        element.Measure(remainingSize);
                    }
                }
                else
                {
                    element.Measure(remainingSize);
                }

                var desiredSize = element.DesiredSize;

                // Decrease the remaining space for the rest of the children
                switch (Orientation)
                {
                    case Orientation.Horizontal:
                        maximumHeight = Math.Max(maximumHeight, usedHeight + desiredSize.Height);

                        if (!IsSplitter(element))
                        {
                            if (!isExpanded)
                            {
                                usedWidth += desiredSize.Width;
                            }
                            else
                            {
                                usedWidth += Math.Max(0, (constraint.Width - splitterThickness) * proportion);
                            }
                        }

                        break;
                    case Orientation.Vertical:
                        maximumWidth = Math.Max(maximumWidth, usedWidth + desiredSize.Width);

                        if (!IsSplitter(element))
                        {
                            if (!isExpanded)
                            {
                                usedHeight += desiredSize.Height;
                            }
                            else
                            {
                                usedHeight += Math.Max(0, (constraint.Height - splitterThickness) * proportion);
                            }
                        }

                        break;
                }
            }

            maximumWidth = Math.Max(maximumWidth, usedWidth);
            maximumHeight = Math.Max(maximumHeight, usedHeight);

            return new Size(maximumWidth, maximumHeight);
        }

        /// <summary>
        /// Control arrangement
        /// </summary>
        protected override Size ArrangeOverride(Size arrangeSize)
        {
            var left = 0.0;
            var top = 0.0;
            var right = 0.0;
            var bottom = 0.0;

            // Arrange each of the Children
            var splitterThickness = GetTotalSplitterThickness(Children);
            var index = 0;

            AssignProportions(Children, arrangeSize);

            foreach (var element in Children)
            {
                // Determine the remaining space left to arrange the element
                var remainingRect = new Rect(
                    left,
                    top,
                    Math.Max(0.0, arrangeSize.Width - left - right),
                    Math.Max(0.0, arrangeSize.Height - top - bottom));

                // Trim the remaining Rect to the docked size of the element
                // (unless the element should fill the remaining space because
                // of LastChildFill)
                if (index < Children.Count())
                {
                    var desiredSize = element.DesiredSize;
                    var proportion = GetControlProportion(element);

                    bool isExpanded = IsExpanded(element);

                    switch (Orientation)
                    {
                        case Orientation.Horizontal:
                        {
                            if (GetSplitter(element) is not {} splitter)
                            {
                                if (IsSplitter(element) || !isExpanded)
                                {
                                    left += desiredSize.Width;
                                    remainingRect = remainingRect.WithWidth(desiredSize.Width);
                                }
                                else
                                {
                                    Debug.Assert(!double.IsNaN(proportion));
                                    remainingRect = remainingRect.WithWidth(Math.Max(0,
                                        (arrangeSize.Width - splitterThickness) * proportion));
                                    left += Math.Max(0, (arrangeSize.Width - splitterThickness) * proportion);
                                }
                            }
                            else
                            {
                                remainingRect = remainingRect.WithX(left - splitter.Thickness).WithWidth(desiredSize.Width);
                            }

                            break;
                        }
                        case Orientation.Vertical:
                        {
                            if (GetSplitter(element) is not {} splitter)
                            {
                                if (IsSplitter(element) || !isExpanded)
                                {
                                    top += desiredSize.Height;
                                    remainingRect = remainingRect.WithHeight(desiredSize.Height);
                                }
                                else
                                {
                                    Debug.Assert(!double.IsNaN(proportion));
                                    remainingRect = remainingRect.WithHeight(Math.Max(0,
                                        (arrangeSize.Height - splitterThickness) * proportion));
                                    top += Math.Max(0, (arrangeSize.Height - splitterThickness) * proportion);
                                }
                            }
                            else
                            {
                                remainingRect = remainingRect.WithY(top - splitter.Thickness).WithHeight(desiredSize.Height);
                            }

                            break;
                        }
                    }
                }

                element.Arrange(remainingRect);
                index++;
            }

            return arrangeSize;
        }

        protected override void OnPropertyChanged(AvaloniaPropertyChangedEventArgs change)
        {
            base.OnPropertyChanged(change);

            if (change.Property == OrientationProperty)
            {
                InvalidateMeasure();
            }
        }

        class ExpansionState
        {
            public bool IsExpanded;
            public double Proportion;
        }

        private Dictionary<Control, ExpansionState> _expansionStates = new();
    }
}
using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Diagnostics;
using System.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.Presenters;
using Dock.Model.Controls;
using Dock.Model.Core;
using Dock.Model.ReactiveUI.Controls;
using Dock.ProportionalStackPanel;
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
                        foreach (var item in e.NewItems.OfType<IControl>())
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
        IControl? FindClosestTarget(IList<IControl> children, IControl element)
        {
            int distance = int.MaxValue;

            int offset = children.IndexOf(element);

            IControl? target = null;

            for (int j = offset + 1; j < children.Count; j++)
            {
                int dist = j - offset;
                if (dist < distance && children[j] is not ProportionalStackPanelSplitter)
                {
                    distance = dist;
                    target = children[j];
                    break;
                }
            }

            for (int j = offset - 1; j >= 0; j--)
            {
                int dist = offset - j;
                if (dist < distance && children[j] is not ProportionalStackPanelSplitter)
                {
                    distance = dist;
                    target = children[j];
                    break;
                }
            }

            return target;
        }

        /// <summary>
        /// Assign all proportions
        /// </summary>
        private void AssignProportions(IList<IControl> children, Size size)
        {
            // Handle expansion ranges
            foreach (IControl control in children.Where(x => x is not ProportionalStackPanelSplitter))
            {
                bool isExpanded = IsExpanded(control);

                // Get expansion state, create if needed
                ExpansionState? state;
                if (!_expansionStates.TryGetValue(control, out state))
                {
                    state = new ExpansionState()
                    {
                        IsExpanded = isExpanded,
                        Proportion = ProportionalStackPanelSplitter.GetProportion(control)
                    };

                    _expansionStates.Add(control, state);
                }

                // New expansion state?
                if (state.IsExpanded != isExpanded)
                {
                    IControl? target = FindClosestTarget(children, control);

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
                        ProportionalStackPanelSplitter.SetProportion(control, proportion);
                    }
                    else
                    {
                        // Set cached proportion
                        Debug.Assert(!double.IsNaN(state.Proportion));
                        ProportionalStackPanelSplitter.SetProportion(control, state.Proportion);
                    }

                    // If there's a target, mark for distribution
                    if (target != null)
                    {
                        ProportionalStackPanelSplitter.SetProportion(target, double.NaN);
                    }

                    // Set new expansion state
                    state.IsExpanded = isExpanded;
                }
            }

            // Current proportion
            var assignedProportion = 0.0;
            
            // All unassigned ranges
            List<Tuple<IControl, double>> unassigned = new();

            // Summarize range
            for (var i = 0; i < children.Count; i++)
            {
                var element = (Control)children[i];

                if (element is not ProportionalStackPanelSplitter)
                {
                    var proportion = ProportionalStackPanelSplitter.GetProportion(element);

                    if (double.IsNaN(proportion))
                    {
                        unassigned.Add(new Tuple<IControl, double>(element, 0));
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
                    if (element is not ProportionalStackPanelSplitter)
                    {
                        var proportion = (1.0 - toAssign) / unassigned.Count;
                        Debug.Assert(!double.IsNaN(proportion + control.Item2));
                        ProportionalStackPanelSplitter.SetProportion(element, proportion + control.Item2);
                        assignedProportion += (1.0 - toAssign) / unassigned.Count;
                    }
                }
            }

            // Expansion or contraction?
            if (assignedProportion < 1)
            {
                var numChildren = (double)children.Count(c => c is not ProportionalStackPanelSplitter);

                var toAdd = (1.0 - assignedProportion) / numChildren;

                foreach (var child in children.Where(c => c is not ProportionalStackPanelSplitter))
                {
                    var proportion = ProportionalStackPanelSplitter.GetProportion(child) + toAdd;
                    ProportionalStackPanelSplitter.SetProportion(child, proportion);
                }
            }
            else if (assignedProportion > 1)
            {
                var numChildren = (double)children.Count(c => c is not ProportionalStackPanelSplitter);

                var toRemove = (assignedProportion - 1.0) / numChildren;

                foreach (var child in children.Where(c => c is not ProportionalStackPanelSplitter))
                {
                    var proportion = ProportionalStackPanelSplitter.GetProportion(child) - toRemove;
                    ProportionalStackPanelSplitter.SetProportion(child, proportion);
                }
            }
        }

        /// <summary>
        /// Get the default splitter thickness
        /// </summary>
        private double GetTotalSplitterThickness(IList<IControl> children)
        {
            var result = children.OfType<ProportionalStackPanelSplitter>().Sum(c => c.Thickness);
            return double.IsNaN(result) ? 0 : result;
        }

        /// <summary>
        /// Check if a given control is expanded
        /// </summary>
        /// <param name="control"></param>
        /// <returns></returns>
        private bool IsExpanded(IControl control)
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
            var children = GetChildren();
            var splitterThickness = GetTotalSplitterThickness(children);

            AssignProportions(children, constraint);

            // Measure each of the Children
            foreach (var control in children)
            {
                var element = (Control)control;
                // Get the child's desired size
                var remainingSize = new Size(
                    Math.Max(0.0, constraint.Width - usedWidth - splitterThickness),
                    Math.Max(0.0, constraint.Height - usedHeight - splitterThickness));

                var proportion = ProportionalStackPanelSplitter.GetProportion(element);

                bool isExpanded = IsExpanded(element);

                if (element is not ProportionalStackPanelSplitter && isExpanded)
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

                        if (element is ProportionalStackPanelSplitter || !isExpanded)
                        {
                            usedWidth += desiredSize.Width;
                        }
                        else
                        {
                            usedWidth += Math.Max(0, (constraint.Width - splitterThickness) * proportion);
                        }

                        break;
                    case Orientation.Vertical:
                        maximumWidth = Math.Max(maximumWidth, usedWidth + desiredSize.Width);

                        if (element is ProportionalStackPanelSplitter || !isExpanded)
                        {
                            usedHeight += desiredSize.Height;
                        }
                        else
                        {
                            usedHeight += Math.Max(0, (constraint.Height - splitterThickness) * proportion);
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
            var children = GetChildren();
            var splitterThickness = GetTotalSplitterThickness(children);
            var index = 0;

            AssignProportions(children, arrangeSize);

            foreach (var element in children)
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
                if (index < children.Count())
                {
                    var desiredSize = element.DesiredSize;
                    var proportion = ProportionalStackPanelSplitter.GetProportion(element);

                    bool isExpanded = IsExpanded(element);

                    switch (Orientation)
                    {
                        case Orientation.Horizontal:
                            if (element is ProportionalStackPanelSplitter || !isExpanded)
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

                            break;
                        case Orientation.Vertical:
                            if (element is ProportionalStackPanelSplitter || !isExpanded)
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

                            break;
                    }
                }

                element.Arrange(remainingRect);
                index++;
            }

            return arrangeSize;
        }

        /// <summary>
        /// Get all children
        /// </summary>
        internal IList<IControl> GetChildren()
        {
            return Children.Select(c =>
            {
                if (c is ContentPresenter cp)
                {
                    cp.UpdateChild();
                    return cp.Child;
                }

                return c;
            }).ToList();
        }

        class ExpansionState
        {
            public bool IsExpanded;
            public double Proportion;
        }

        private Dictionary<IControl, ExpansionState> _expansionStates = new();
    }
}
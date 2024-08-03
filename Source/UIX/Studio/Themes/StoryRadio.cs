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
using System.Linq;
using System.Runtime.CompilerServices;
using System.Windows.Input;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Rendering;
using ReactiveUI;

namespace Studio.Views.Controls
{
    public partial class StoryRadio : UserControl
    {
        private class GroupManager
        {
            /// <summary>
            /// Group to item dictionary
            /// </summary>
            public Dictionary<string, List<WeakReference<StoryRadio>>> Items = new();

            /// <summary>
            /// Add a new story radio
            /// </summary>
            /// <param name="story">radio to add</param>
            /// <param name="groupName">designated group name</param>
            public void Add(StoryRadio story, string groupName)
            {
                if (!Items.TryGetValue(groupName, out var group))
                {
                    group = new List<WeakReference<StoryRadio>>();
                    Items.Add(groupName, group);
                }
                
                group.Add(new WeakReference<StoryRadio>(story));
            }
            
            /// <summary>
            /// Remove a story radio
            /// </summary>
            /// <param name="story">radio to remove</param>
            /// <param name="groupName">designated group name</param>
            public void Remove(StoryRadio story, string groupName)
            {
                if (Items.TryGetValue(groupName, out var items))
                {
                    items.RemoveAll(x => x.TryGetTarget(out var item) || item == story);
                }
            }

            /// <summary>
            /// Enumerate all radios for a group name
            /// </summary>
            /// <param name="groupName">group name to query</param>
            /// <returns>enumerator</returns>
            public IEnumerable<StoryRadio> GetEnumerator(string groupName)
            {
                if (!Items.TryGetValue(groupName, out var items))
                {
                    yield break;
                }

                foreach (WeakReference<StoryRadio> weak in items)
                {
                    if (weak.TryGetTarget(out var item))
                    {
                        yield return item;
                    }
                }
            }
        }
        
        /// <summary>
        /// Title style for the given story
        /// </summary>
        public static readonly StyledProperty<string> TitleProperty = AvaloniaProperty.Register<StoryRadio, string>(nameof(Title));
        
        /// <summary>
        /// Selection style for the given view
        /// </summary>
        public static readonly StyledProperty<bool> IsSelectedProperty = AvaloniaProperty.Register<StoryRadio, bool>(nameof(IsSelected), false);
        
        /// <summary>
        /// Group name style for the given view
        /// </summary>
        public static readonly StyledProperty<string> GroupNameProperty = AvaloniaProperty.Register<StoryRadio, string>(nameof(GroupName), string.Empty);
        
        /// <summary>
        /// Select command style for the given view
        /// </summary>
        public static readonly StyledProperty<ICommand> SelectCommandProperty = AvaloniaProperty.Register<StoryRadio, ICommand>(nameof(SelectCommand));
        
        /// <summary>
        /// Command style for the given view
        /// </summary>
        public static readonly StyledProperty<ICommand> CommandProperty = AvaloniaProperty.Register<StoryRadio, ICommand>(nameof(Command));
        
        /// <summary>
        /// Command parameter style for the given view
        /// </summary>
        public static readonly StyledProperty<object> CommandParameterProperty = AvaloniaProperty.Register<StoryRadio, object>(nameof(CommandParameterProperty));

        /// <summary>
        /// Constructor
        /// </summary>
        public StoryRadio()
        {
            SelectCommand = ReactiveCommand.Create(OnSelect);
        }

        /// <summary>
        /// Invoked on any property change
        /// </summary>
        protected override void OnPropertyChanged(AvaloniaPropertyChangedEventArgs change)
        {
            // Update selection classes
            if (change.Property.Name == nameof(IsSelected))
            {
                if (IsSelected)
                {
                    PseudoClasses.Add(":selected");
                }
                else
                {
                    PseudoClasses.Remove(":selected");
                }
            }
            
            // Pass down
            base.OnPropertyChanged(change);
        }

        /// <summary>
        /// Invoked on selection
        /// </summary>
        private void OnSelect()
        {
            IsSelected = true;
           
            // Disable selection on all other radios
            foreach (StoryRadio storyRadio in  _groupManager?.GetEnumerator(GroupName) ?? Enumerable.Empty<StoryRadio>())
            {
                if (storyRadio != this)
                {
                    storyRadio.IsSelected = false;
                }
            }
            
            Command?.Execute(CommandParameter);
        }

        /// <summary>
        /// Invoked on visual attach
        /// </summary>
        protected override void OnAttachedToVisualTree(VisualTreeAttachmentEventArgs e)
        {
            if (!string.IsNullOrEmpty(GroupName))
            {
                _groupManager?.Remove(this, GroupName);
                _groupManager = GetGroupManager();
                _groupManager.Add(this, GroupName);
            }
            
            base.OnAttachedToVisualTree(e);
        }

        /// <summary>
        /// Invoked on visual detach
        /// </summary>
        protected override void OnDetachedFromVisualTree(VisualTreeAttachmentEventArgs e)
        {
            base.OnDetachedFromVisualTree(e);

            if (!string.IsNullOrEmpty(GroupName))
            {
                _groupManager?.Remove(this, GroupName);
            }
        }

        /// <summary>
        /// Get the group manager for this control
        /// </summary>
        private GroupManager GetGroupManager()
        {
            if (VisualRoot == null)
            {
                return _defaultGroupManager;
            }

            return _visualRoots.GetValue(VisualRoot, _ => new GroupManager());
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
        /// Selection getter / setter
        /// </summary>
        public bool IsSelected
        {
            get => GetValue(IsSelectedProperty);
            set => SetValue(IsSelectedProperty, value);
        }
        
        /// <summary>
        /// Group name getter / setter
        /// </summary>
        public string GroupName
        {
            get => GetValue(GroupNameProperty);
            set => SetValue(GroupNameProperty, value);
        }

        /// <summary>
        /// Select command getter / setter
        /// </summary>
        public ICommand SelectCommand
        {
            get => GetValue(SelectCommandProperty);
            set => SetValue(SelectCommandProperty, value);
        }

        /// <summary>
        /// Command getter / setter
        /// </summary>
        public ICommand Command
        {
            get => GetValue(CommandProperty);
            set => SetValue(CommandProperty, value);
        }

        /// <summary>
        /// Command parameter getter / setter
        /// </summary>
        public object CommandParameter
        {
            get => GetValue(CommandParameterProperty);
            set => SetValue(CommandParameterProperty, value);
        }

        /// <summary>
        /// Owning group
        /// </summary>
        private GroupManager? _groupManager;
        
        /// <summary>
        /// All assigned group managers for a visual root
        /// </summary>
        private static ConditionalWeakTable<IRenderRoot, GroupManager> _visualRoots = new();

        /// <summary>
        /// Shared default group manager
        /// </summary>
        private static GroupManager _defaultGroupManager = new();
    }
}
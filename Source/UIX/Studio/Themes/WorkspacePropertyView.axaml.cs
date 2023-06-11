using System.Windows.Input;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Avalonia.Media;
using ReactiveUI;

namespace Studio.Views.Controls
{
    public partial class WorkspacePropertyView : UserControl
    {
        /// <summary>
        /// Title style for the given view
        /// </summary>
        public static readonly StyledProperty<string> TitleProperty = AvaloniaProperty.Register<WorkspacePropertyView, string>(nameof(Title));
        
        /// <summary>
        /// Expansion icon style for the given view
        /// </summary>
        public static readonly StyledProperty<StreamGeometry> ExpansionIconProperty = AvaloniaProperty.Register<WorkspacePropertyView, StreamGeometry>(nameof(ExpansionIcon), ResourceLocator.GetIcon("ChevronUp")!);
        
        /// <summary>
        /// Expansion style for the given view
        /// </summary>
        public static readonly StyledProperty<bool> IsExpandedProperty = AvaloniaProperty.Register<WorkspacePropertyView, bool>(nameof(IsExpanded), true);
        
        /// <summary>
        /// Expansion command style for the given view
        /// </summary>
        public static readonly StyledProperty<ICommand> ExpandCommandProperty = AvaloniaProperty.Register<WorkspacePropertyView, ICommand>(nameof(ExpandCommand));

        public WorkspacePropertyView()
        {
            ExpandCommand = ReactiveCommand.Create(OnExpand);
        }

        protected override void OnPropertyChanged<T>(AvaloniaPropertyChangedEventArgs<T> change)
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
    }
}
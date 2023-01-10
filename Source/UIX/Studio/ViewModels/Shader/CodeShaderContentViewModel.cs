using System;
using System.Collections.ObjectModel;
using Avalonia.Media;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Runtime.ViewModels.Shader;
using Studio.ViewModels.Workspace.Listeners;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Shader
{
    public class CodeShaderContentViewModel : ReactiveObject, IShaderContentViewModel
    {
        /// <summary>
        /// View icon
        /// </summary>
        public StreamGeometry? Icon
        {
            get => _icon;
            set => this.RaiseAndSetIfChanged(ref _icon, value);
        }
        
        /// <summary>
        /// Workspace within this overview
        /// </summary>
        public IPropertyViewModel? PropertyCollection
        {
            get => _propertyCollection;
            set => this.RaiseAndSetIfChanged(ref _propertyCollection, value);
        }

        /// <summary>
        /// Is this model active?
        /// </summary>
        public bool IsActive
        {
            get => _isActive;
            set => this.RaiseAndSetIfChanged(ref _isActive, value);
        }

        /// <summary>
        /// Underlying object
        /// </summary>
        public Workspace.Objects.ShaderViewModel? Object
        {
            get => _object;
            set
            {
                this.RaiseAndSetIfChanged(ref _object, value);

                if (_object != null)
                {
                    OnObjectChanged();
                }
            }
        }

        /// <summary>
        /// Selected file
        /// </summary>
        public ShaderFileViewModel? SelectedShaderFileViewModel
        {
            get => _selectedSelectedShaderFileViewModel;
            set => this.RaiseAndSetIfChanged(ref _selectedSelectedShaderFileViewModel, value);
        }

        /// <summary>
        /// Invoked on object change
        /// </summary>
        private void OnObjectChanged()
        {
            // Submit request if not already
            if (Object!.Contents == string.Empty)
            {
                PropertyCollection?.GetService<IShaderCodeService>()?.EnqueueShaderContents(Object);
            }

            // Set VM when available
            Object.FileViewModels.ToObservableChangeSet().OnItemAdded(x =>
            {
                if (SelectedShaderFileViewModel == null)
                {
                    SelectedShaderFileViewModel = x;
                }
            }).Subscribe();
        }

        /// <summary>
        /// Internal object
        /// </summary>
        private Workspace.Objects.ShaderViewModel? _object;

        /// <summary>
        /// Underlying view model
        /// </summary>
        private IPropertyViewModel? _propertyCollection;

        /// <summary>
        /// Internal icon state
        /// </summary>
        private StreamGeometry? _icon = ResourceLocator.GetIcon("Code");

        /// <summary>
        /// Selected file
        /// </summary>
        private ShaderFileViewModel? _selectedSelectedShaderFileViewModel = null;

        /// <summary>
        /// Internal active state
        /// </summary>
        private bool _isActive = false;
    }
}
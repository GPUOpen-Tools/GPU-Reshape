using System;
using Avalonia.Media;
using ReactiveUI;
using Studio.ViewModels.Workspace.Listeners;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Shader
{
    public class BlockGraphShaderContentViewModel : ReactiveObject, IShaderContentViewModel
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
        /// Invoked on object changes
        /// </summary>
        private void OnObjectChanged()
        {
            // Submit request if not already
            if (Object!.Contents == string.Empty)
            {
                PropertyCollection?.GetService<IShaderCodeService>()?.EnqueueShaderBlockGraph(Object);
            }
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
        private StreamGeometry? _icon = ResourceLocator.GetIcon("DotGraph");

        /// <summary>
        /// Internal active state
        /// </summary>
        private bool _isActive = false;
    }
}
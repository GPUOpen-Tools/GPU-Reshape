using System;
using System.Collections.ObjectModel;
using Avalonia.Media;
using Dock.Model.ReactiveUI.Controls;
using DynamicData;
using ReactiveUI;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Documents
{
    public class ShaderViewModel : Document, IDocumentViewModel
    {
        /// <summary>
        /// Document icon
        /// </summary>
        public StreamGeometry? Icon
        {
            get => _icon;
            set => this.RaiseAndSetIfChanged(ref _icon, value);
        }

        /// <summary>
        /// Icon color
        /// </summary>
        public Color? IconForeground
        {
            get => _iconForeground;
            set => this.RaiseAndSetIfChanged(ref _iconForeground, value);
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
        /// Shader UID
        /// </summary>
        public UInt64 GUID
        {
            get => _guid;
            set
            {
                this.RaiseAndSetIfChanged(ref _guid, value);
                OnShaderChanged();
            }
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

                // Set title
                if (_object != null)
                {
                    Title = _object.Filename;
                }
            }
        }

        /// <summary>
        /// Invoked when the shader has changed
        /// </summary>
        public void OnShaderChanged()
        {
            // Get collection
            var collection = _propertyCollection?.GetProperty<ShaderCollectionViewModel>();

            // Fetch object
            Object = collection?.GetOrAddShader(_guid);
        }

        /// <summary>
        /// Internal guid
        /// </summary>
        private UInt64 _guid;

        /// <summary>
        /// Internal object
        /// </summary>
        private Workspace.Objects.ShaderViewModel? _object;

        /// <summary>
        /// Underlying view model
        /// </summary>
        private IPropertyViewModel? _propertyCollection;
        
        /// <summary>
        /// Internal icon
        /// </summary>
        private StreamGeometry? _icon = ResourceLocator.GetIcon("DotsGrid");

        /// <summary>
        /// Internal icon color
        /// </summary>
        private Color? _iconForeground = ResourceLocator.GetResource<Color>("ThemeForegroundColor");
    }
}
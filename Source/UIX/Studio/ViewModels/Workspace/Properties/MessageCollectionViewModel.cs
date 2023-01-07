using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Windows.Input;
using Avalonia;
using Avalonia.Threading;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Studio.Models.Workspace.Listeners;
using Studio.Models.Workspace.Objects;
using Studio.Services;
using Studio.ViewModels.Workspace.Listeners;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.ViewModels.Workspace.Properties
{
    public class MessageCollectionViewModel : ReactiveObject, IMessageCollectionViewModel
    {
        /// <summary>
        /// Name of this property
        /// </summary>
        public string Name { get; } = "Messages";

        /// <summary>
        /// Visibility of this property
        /// </summary>
        public PropertyVisibility Visibility => PropertyVisibility.WorkspaceOverview;

        /// <summary>
        /// Parent property
        /// </summary>
        public IPropertyViewModel? Parent { get; set; }

        /// <summary>
        /// Child properties
        /// </summary>
        public ISourceList<IPropertyViewModel> Properties { get; set; } = new SourceList<IPropertyViewModel>();

        /// <summary>
        /// All services
        /// </summary>
        public ISourceList<IPropertyService> Services { get; set; } = new SourceList<IPropertyService>();

        /// <summary>
        /// All condensed messages
        /// </summary>
        public ObservableCollection<Objects.ValidationObject> ValidationObjects { get; } = new();

        /// <summary>
        /// Opens a given shader document from view model
        /// </summary>
        public ICommand OpenShaderDocument;

        /// <summary>
        /// View model associated with this property
        /// </summary>
        public IConnectionViewModel? ConnectionViewModel
        {
            get => _connectionViewModel;
            set
            {
                this.RaiseAndSetIfChanged(ref _connectionViewModel, value);

                OnConnectionChanged();
            }
        }

        public MessageCollectionViewModel()
        {
            OpenShaderDocument = ReactiveCommand.Create<object>(OnOpenShaderDocument);
        }

        /// <summary>
        /// On shader document open
        /// </summary>
        /// <param name="viewModel">must be ValidationObject</param>
        private void OnOpenShaderDocument(object viewModel)
        {
            var validationObject = (ValidationObject)viewModel;

            // May not be ready yet
            if (validationObject.Segment == null)
            {
                return;
            }

            // Create document
            if (App.Locator.GetService<IWindowService>()?.LayoutViewModel is { } layoutViewModel)
            {
                layoutViewModel.OpenDocument?.Execute(new Documents.ShaderViewModel()
                {
                    Id = $"Shader{validationObject.Segment.Location.SGUID}",
                    Title = $"Shader (loading)",
                    PropertyCollection = Parent,
                    GUID = validationObject.Segment.Location.SGUID
                });
            }
        }

        /// <summary>
        /// Invoked when a connection has changed
        /// </summary>
        private void OnConnectionChanged()
        {
            // Set connection
            _shaderMappingService.ConnectionViewModel = ConnectionViewModel;
            
            // Make visible
            Parent?.Services.Add(_shaderMappingService);
            
            // Register internal listeners
            _connectionViewModel?.Bridge?.Register(ShaderSourceMappingMessage.ID, _shaderMappingService);

            // Create all properties
            CreateProperties();
        }

        /// <summary>
        /// Create all properties
        /// </summary>
        private void CreateProperties()
        {
            Properties.Clear();

            if (_connectionViewModel == null)
            {
                return;
            }
        }

        /// <summary>
        /// Shader mapping bridge listener
        /// </summary>
        private ShaderMappingService _shaderMappingService = new();

        /// <summary>
        /// Internal view model
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;
    }
}
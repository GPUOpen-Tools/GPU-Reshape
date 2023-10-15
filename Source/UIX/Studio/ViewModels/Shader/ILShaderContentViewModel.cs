// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
using System.Windows.Input;
using Avalonia.Media;
using ReactiveUI;
using Runtime.ViewModels.IL;
using Runtime.ViewModels.Traits;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Documents;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Services;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Shader
{
    public class ILShaderContentViewModel : ReactiveObject, ITextualShaderContentViewModel
    {
        /// <summary>
        /// The owning navigation context
        /// </summary>
        public INavigationContext? NavigationContext { get; set; }
        
        /// <summary>
        /// Given descriptor
        /// </summary>
        public ShaderDescriptor? Descriptor
        {
            set
            {
                NavigationLocation = null;
                NavigationLocation = value?.StartupLocation;
            }
        }
        
        /// <summary>
        /// View icon
        /// </summary>
        public StreamGeometry? Icon
        {
            get => _icon;
            set => this.RaiseAndSetIfChanged(ref _icon, value);
        }

        /// <summary>
        /// The final assembled program
        /// </summary>
        public string AssembledProgram
        {
            get => _assembledProgram;
            set => this.RaiseAndSetIfChanged(ref _assembledProgram, value);
        }

        /// <summary>
        /// The current assembler
        /// </summary>
        public Assembler? Assembler
        {
            get => _assembler;
            set => this.RaiseAndSetIfChanged(ref _assembler, value);
        }

        /// <summary>
        /// Currently selected validation object
        /// </summary>
        public ValidationObject? SelectedValidationObject
        {
            get => _selectedValidationObject;
            set => this.RaiseAndSetIfChanged(ref _selectedValidationObject, value);
        }

        /// <summary>
        /// Current detail view model
        /// </summary>
        public IValidationDetailViewModel? DetailViewModel
        {
            get => _detailViewModel;
            set => this.RaiseAndSetIfChanged(ref _detailViewModel, value);
        }

        /// <summary>
        /// Current location
        /// </summary>
        public NavigationLocation? NavigationLocation
        {
            get => _navigationLocation;
            set => this.RaiseAndSetIfChanged(ref _navigationLocation, value);
        }

        /// <summary>
        /// Selection command
        /// </summary>
        public ICommand? OnSelected { get; }
        
        /// <summary>
        /// Close detail command
        /// </summary>
        public ICommand? CloseDetail { get; }

        /// <summary>
        /// Show in source command
        /// </summary>
        public ICommand? ShowInSource { get; }
        
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

        public ILShaderContentViewModel()
        {
            CloseDetail = ReactiveCommand.Create(OnCloseDetail);
            ShowInSource = ReactiveCommand.Create(OnShowInSource);
        }

        /// <summary>
        /// Invoked on detail closes
        /// </summary>
        private void OnCloseDetail()
        {
            DetailViewModel = null;
        }

        /// <summary>
        /// Invoked on navigation requests
        /// </summary>
        private void OnShowInSource()
        {
            // Navigate to the currently selected validation object
            NavigationContext?.Navigate(typeof(CodeShaderContentViewModel), SelectedValidationObject?.Segment?.Location);
        }

        /// <summary>
        /// Is the overlay visible?
        /// </summary>
        /// <returns></returns>
        public bool IsOverlayVisible()
        {
            return true;
        }

        /// <summary>
        /// Is a validation object visible?
        /// </summary>
        public bool IsObjectVisible(ValidationObject validationObject)
        {
            return true;
        }

        /// <summary>
        /// Transform a shader location line
        /// </summary>
        public int TransformLine(ShaderLocation shaderLocation)
        {
            if (Assembler == null)
            {
                throw new Exception("Transformation without an assembler");
            }
            
            // Transform instruction indices to line from assembler
            return (int)Assembler.GetMapping(shaderLocation.BasicBlockId, shaderLocation.InstructionIndex).Line;
        }

        /// <summary>
        /// Invoked on object change
        /// </summary>
        private void OnObjectChanged()
        {
            // Submit request if not already
            if (Object!.Program == null)
            {
                PropertyCollection?.GetService<IShaderCodeService>()?.EnqueueShaderIL(Object);
            }

            // Bind program, assemble when changed
            Object.WhenAnyValue(x => x.Program).WhereNotNull().Subscribe(program =>
            {
                // Create assembler
                _assembler = new Assembler(program);
                
                // Assembler used assemble!
                AssembledProgram = _assembler.Assemble();
            });
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
        private StreamGeometry? _icon = ResourceLocator.GetIcon("CommandCode");

        /// <summary>
        /// Internal active state
        /// </summary>
        private bool _isActive = false;

        /// <summary>
        /// Internal location
        /// </summary>
        private NavigationLocation? _navigationLocation;

        /// <summary>
        /// Internal assembler
        /// </summary>
        private Assembler? _assembler;

        /// <summary>
        /// Internal assembled program
        /// </summary>
        private string _assembledProgram;

        /// <summary>
        /// Internal selection state
        /// </summary>
        private ValidationObject? _selectedValidationObject;

        /// <summary>
        /// Internal detail state
        /// </summary>
        private IValidationDetailViewModel? _detailViewModel;
    }
}
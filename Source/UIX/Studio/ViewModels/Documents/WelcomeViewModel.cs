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

using System.Reactive;
using System.Reactive.Subjects;
using System.Windows.Input;
using Avalonia;
using Avalonia.Media;
using Dock.Model.ReactiveUI.Controls;
using ReactiveUI;
using Studio.Services;

namespace Studio.ViewModels.Documents
{
    public class WelcomeViewModel : Document, IDocumentViewModel
    {
        /// <summary>
        /// Descriptor setter, constructs the top document from given descriptor
        /// </summary>
        public IDescriptor? Descriptor
        {
            get => _descriptor;
            set => this.RaiseAndSetIfChanged(ref _descriptor, value);
        }
        
        /// <summary>
        /// Create connection command
        /// </summary>
        public ICommand CreateConnection { get; }
        
        /// <summary>
        /// Create connection command
        /// </summary>
        public ICommand LaunchApplication { get; }
        
        /// <summary>
        /// Close command
        /// </summary>
        public ICommand Close { get; }

        /// <summary>
        /// Closed event
        /// </summary>
        public Subject<Unit> OnClose { get; } = new();
        
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

        public WelcomeViewModel()
        {
            CreateConnection = ReactiveCommand.Create(OnCreateConnection);
            LaunchApplication = ReactiveCommand.Create(OnLaunchApplication);
            Close = ReactiveCommand.Create(OnCloseStudio);
        }

        /// <summary>
        /// Invoked on connection
        /// </summary>
        private void OnCreateConnection()
        {
            App.Locator.GetService<IWindowService>()?.OpenFor(new ConnectViewModel());
        }

        /// <summary>
        /// Invoked on launch
        /// </summary>
        private void OnLaunchApplication()
        {
            App.Locator.GetService<IWindowService>()?.OpenFor(new LaunchViewModel());
        }

        /// <summary>
        /// Invoked on close
        /// </summary>
        private void OnCloseStudio()
        {
            OnClose.OnNext(Unit.Default);
        }
        
        /// <summary>
        /// Internal icon state
        /// </summary>
        private StreamGeometry? _icon = ResourceLocator.GetIcon("App");

        /// <summary>
        /// Internal icon color
        /// </summary>
        private Color? _iconForeground = ResourceLocator.GetResource<Color>("SystemBaseHighColor");

        /// <summary>
        /// Internal descriptor
        /// </summary>
        private IDescriptor? _descriptor;
    }
}
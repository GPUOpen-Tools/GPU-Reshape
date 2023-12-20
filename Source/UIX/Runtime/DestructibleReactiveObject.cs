// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

using System.Reactive.Disposables;
using ReactiveUI;
using Runtime.ViewModels.Traits;

namespace Studio
{
    public class DestructibleReactiveObject : ReactiveObject, IActivatableViewModel, IDestructableObject
    {
        /// <summary>
        /// Underlying activator
        /// </summary>
        public ViewModelActivator Activator { get; }

        /// <summary>
        /// Constructor
        /// </summary>
        public DestructibleReactiveObject()
        {
            // Create activator
            Activator = new ViewModelActivator();

            // Bind activation
            this.WhenActivated(disposables =>
            {
                // Construct top
                Construct();

                // Destruct top
                Disposable
                    .Create(() => Destruct())
                    .DisposeWith(disposables);
            });
        }

        /// <summary>
        /// Invoked on construction
        /// </summary>
        public virtual void Construct()
        {
            // Poof
        }

        /// <summary>
        /// Invoked on destruction
        /// </summary>
        public virtual void Destruct()
        {
            // Poof
        }
    }
}
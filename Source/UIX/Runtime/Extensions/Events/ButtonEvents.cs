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
using System.Reactive.Linq;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Dock.Model.Core;
using Dock.Model.Core.Events;

namespace Studio.Extensions
{
    public class ButtonEvents
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public ButtonEvents(Button button)
        {
            this._button = button;
        }
        
        /// <summary>
        /// Observable clicked
        /// </summary>
        public IObservable<RoutedEventArgs> Click => Observable.FromEvent<EventHandler<RoutedEventArgs>, RoutedEventArgs>(handler =>
        {
            return (s, e) => handler(e);
        }, handler => _button.Click += handler, handler => _button.Click -= handler);
        
        /// <summary>
        /// Observable double tapped
        /// </summary>
        public IObservable<TappedEventArgs> DoubleTapped => Observable.FromEvent<EventHandler<TappedEventArgs>, TappedEventArgs>(handler =>
        {
            return (s, e) => handler(e);
        }, handler => _button.DoubleTapped += handler, handler => _button.DoubleTapped -= handler);
        
        /// <summary>
        /// Internal button
        /// </summary>
        private readonly Button _button;
    }
    
    public static class ButtonExtensions
    {
        /// <summary>
        /// Create events
        /// </summary>
        public static ButtonEvents Events(this Button button)
        {
            return new ButtonEvents(button);
        }
    }
}
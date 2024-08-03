﻿// 
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
    public class ListBoxEvents
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public ListBoxEvents(ListBox listBox)
        {
            this._listBox = listBox;
        }
        
        /// <summary>
        /// Observable double tapped
        /// </summary>
        public IObservable<TappedEventArgs> DoubleTapped => Observable.FromEvent<EventHandler<TappedEventArgs>, TappedEventArgs>(handler =>
        {
            return (s, e) => handler(e);
        }, handler => _listBox.DoubleTapped += handler, handler => _listBox.DoubleTapped -= handler);
        
        /// <summary>
        /// Observable pointer wheel
        /// </summary>
        public IObservable<PointerWheelEventArgs> PointerWheelChanged => Observable.FromEvent<EventHandler<PointerWheelEventArgs>, PointerWheelEventArgs>(handler =>
        {
            return (s, e) => handler(e);
        }, handler => _listBox.PointerWheelChanged += handler, handler => _listBox.PointerWheelChanged -= handler);
        
        /// <summary>
        /// Internal listbox
        /// </summary>
        private readonly ListBox _listBox;
    }
    
    public static class ListBoxExtensions
    {
        /// <summary>
        /// Create events
        /// </summary>
        public static ListBoxEvents Events(this ListBox listBox)
        {
            return new ListBoxEvents(listBox);
        }
    }
}
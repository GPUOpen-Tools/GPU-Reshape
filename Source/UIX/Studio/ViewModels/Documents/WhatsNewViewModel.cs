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
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Windows.Input;
using Avalonia;
using Avalonia.Media;
using Avalonia.Platform;
using Dock.Model.ReactiveUI.Controls;
using Newtonsoft.Json;
using ReactiveUI;
using Studio.Services;
using Studio.ViewModels.Data;

namespace Studio.ViewModels.Documents
{
    public class WhatsNewViewModel : Document, IDocumentViewModel
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
        public IBrush? IconForeground
        {
            get => _iconForeground;
            set => this.RaiseAndSetIfChanged(ref _iconForeground, value);
        }

        /// <summary>
        /// Invoked on story selection
        /// </summary>
        public ICommand StorySelectedCommand { get; }

        /// <summary>
        /// Invoked on don't show again selection
        /// </summary>
        public ICommand DontShowAgainCommand { get; }
        
        /// <summary>
        /// All stories
        /// </summary>
        public ObservableCollection<WhatsNewStoryViewModel> Stories { get; } = new();

        /// <summary>
        /// Current selected story
        /// </summary>
        public WhatsNewStoryViewModel? SelectedStory
        {
            get => _selectedStory;
            set => this.RaiseAndSetIfChanged(ref _selectedStory, value);
        }

        /// <summary>
        /// Is the footnote visible?
        /// </summary>
        public bool IsFootnoteVisible
        {
            get => _isFootnoteVisible;
            set => this.RaiseAndSetIfChanged(ref _isFootnoteVisible, value);
        }

        public WhatsNewViewModel()
        {
            // Create commands
            StorySelectedCommand = ReactiveCommand.Create<object>(OnStorySelected);
            DontShowAgainCommand = ReactiveCommand.Create(OnDontShowAgain);
            
            // Document info
            Id    = "WhatsNew";
            Title = "What's New";
            
            // Try to open static index
            Stream? stream = AssetLoader.Open(new Uri("avares://GPUReshape/Resources/WhatsNew/Index.json"));
            if (stream == null)
            {
                Studio.Logging.Error("Failed to open WhatsNew/index.json");
                return;
            }

            // Try to deserialize
            Dictionary<string, string>? value;
            try
            {
                value = JsonConvert.DeserializeObject<Dictionary<string, string>>(new StreamReader(stream).ReadToEnd());
                if (value == null)
                {
                    return;
                }
            }
            catch (Exception _)
            {
                Studio.Logging.Error("Failed to deserialize WhatsNew/index.json");
                return;
            }

            // Create story view models
            foreach (var story in value)
            {
                Stories.Add(new WhatsNewStoryViewModel()
                {
                    Title = story.Key,
                    Uri = new Uri($"avares://GPUReshape/Resources/WhatsNew/{story.Value}")
                });
            }

            // Always have a story selected
            SelectedStory = Stories[0];
            SelectedStory.Selected = true;

            // By default, footnotes are only visible on the welcoming story
            // A bit hardcoded, but it'll suffice for now
            this.WhenAnyValue(x => x.SelectedStory).Subscribe(_ =>
            {
                IsFootnoteVisible = SelectedStory == Stories[0];
            });
        }

        /// <summary>
        /// Invoked on story selection
        /// </summary>
        private void OnStorySelected(object sender)
        {
            SelectedStory = sender as WhatsNewStoryViewModel;
        }

        /// <summary>
        /// Invoked on don't show again
        /// </summary>
        private void OnDontShowAgain()
        {
            // Do not open on next run
            if (ServiceRegistry.Get<ISettingsService>()?.Get<StartupViewModel>() is { } startup)
            {
                startup.ShowWhatsNew = false;
            }
            
            // Close open tabs
            if (ServiceRegistry.Get<IWindowService>()?.LayoutViewModel is { } layoutViewModel)
            {
                layoutViewModel.DocumentLayout?.CloseOwnedDocuments(typeof(WhatsNewDescriptor));
            }
        }

        /// <summary>
        /// Internal icon state
        /// </summary>
        private StreamGeometry? _icon = ResourceLocator.GetIcon("App");

        /// <summary>
        /// Internal icon color
        /// </summary>
        private IBrush? _iconForeground = new SolidColorBrush(ResourceLocator.GetResource<Color>("SystemBaseHighColor"));

        /// <summary>
        /// Internal descriptor
        /// </summary>
        private IDescriptor? _descriptor;

        /// <summary>
        /// Internal story selection
        /// </summary>
        private WhatsNewStoryViewModel? _selectedStory;

        /// <summary>
        /// Internal footnote state
        /// </summary>
        private bool _isFootnoteVisible = false;
    }
}
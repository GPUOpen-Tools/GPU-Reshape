using System;
using ReactiveUI;

namespace Studio.ViewModels.Documents
{
    public class WhatsNewStoryViewModel : ReactiveObject
    {
        /// <summary>
        /// Title of this story
        /// </summary>
        public string Title
        {
            get => _title;
            set => this.RaiseAndSetIfChanged(ref _title, value);
        }

        /// <summary>
        /// Uri of this story
        /// </summary>
        public Uri Uri
        {
            get => _uri;
            set => this.RaiseAndSetIfChanged(ref _uri, value);
        }

        /// <summary>
        /// Is this story selected
        /// </summary>
        public bool Selected
        {
            get => _selected;
            set => this.RaiseAndSetIfChanged(ref _selected, value);
        }

        /// <summary>
        /// Internal uri
        /// </summary>
        private Uri _uri;
        
        /// <summary>
        /// Internal title
        /// </summary>
        private string _title = string.Empty;

        /// <summary>
        /// Internal selected
        /// </summary>
        private bool _selected = false;
    }
}
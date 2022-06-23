using System;
using ReactiveUI;

namespace Studio.ViewModels.Workspace.Properties
{
    public class CondensedMessage : ReactiveObject
    {
        /// <summary>
        /// Backend model
        /// </summary>
        public Models.Workspace.Properties.CondensedMessage Model { get; set; }
        
        /// <summary>
        /// Number of messages
        /// </summary>
        public uint Count
        {
            get => Model.Count;
            set => this.RaiseAndSetIfChanged(ref Model.Count, value);
        }

        /// <summary>
        /// Shader extract
        /// </summary>
        public string Extract
        {
            get => Model.Extract;
            set => this.RaiseAndSetIfChanged(ref Model.Extract, value);
        }

        /// <summary>
        /// General contents
        /// </summary>
        public string Content
        {
            get => Model.Content;
            set => this.RaiseAndSetIfChanged(ref Model.Content, value);
        }
    }
}
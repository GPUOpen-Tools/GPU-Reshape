using System;
using ReactiveUI;

namespace Studio.ViewModels.Workspace.Objects
{
    public class CondensedMessageViewModel : ReactiveObject
    {
        /// <summary>
        /// Backend model
        /// </summary>
        public Models.Workspace.Objects.CondensedMessage Model { get; set; }
        
        /// <summary>
        /// Underlying validation object
        /// </summary>
        public ValidationObject? ValidationObject { get; set; }
        
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
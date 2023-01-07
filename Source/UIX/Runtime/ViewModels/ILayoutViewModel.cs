using System.Reactive;
using System.Reactive.Subjects;
using ReactiveUI;
using System.Windows.Input;
using Dock.Model.Controls;

namespace Runtime.ViewModels
{
    public interface ILayoutViewModel
    {
        /// <summary>
        /// Open a document
        /// </summary>
        public ICommand? OpenDocument { get; }
        
        /// <summary>
        /// Reset the layout
        /// </summary>
        public ICommand ResetLayout { get; }
        
        /// <summary>
        /// Close the layout
        /// </summary>
        public ICommand CloseLayout { get; }

        /// <summary>
        /// Current layout hosted
        /// </summary>
        public IRootDock? Layout { get; set; }
    }
}

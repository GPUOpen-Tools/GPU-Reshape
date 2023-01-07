using Runtime.ViewModels;

namespace Studio.Services
{
    public interface IWindowService
    {
        /// <summary>
        /// Shared layout
        /// </summary>
        public ILayoutViewModel? LayoutViewModel { get; set; }
        
        /// <summary>
        /// Open a window for a given view model
        /// </summary>
        /// <param name="viewModel">view model</param>
        void OpenFor(object viewModel);

        /// <summary>
        /// Request exit
        /// </summary>
        void Exit();
    }
}
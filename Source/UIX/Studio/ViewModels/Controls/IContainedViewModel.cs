namespace Studio.ViewModels.Controls
{
    public interface IContainedViewModel
    {
        /// <summary>
        /// Contained view model within this container model
        /// Certain view models act as containers for view models used in contextual operations
        /// </summary>
        public object? ViewModel { get; set; }
    }
}
using System.Collections.ObjectModel;

namespace Studio.ViewModels.Workspace.Properties
{
    public interface IMessageCollectionViewModel : IPropertyViewModel
    {
        /// <summary>
        /// All condensed messages
        /// </summary>
        public ObservableCollection<Objects.ValidationObject> ValidationObjects { get; }
    }
}
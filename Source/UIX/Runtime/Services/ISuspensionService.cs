using System.ComponentModel;

namespace Studio.Services
{
    public interface ISuspensionService
    {
        /// <summary>
        /// Bind an object for type based suspension, members are bound to the cold storage for the underlying type
        /// </summary>
        /// <param name="obj"></param>
        void BindTypedSuspension(INotifyPropertyChanged obj);
    }
}
using System;
using Avalonia;
using Message.CLR;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Services;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Traits
{
    public interface IBusObject
    {
        /// <summary>
        /// Commit all state to a given stream
        /// </summary>
        /// <param name="stream"></param>
        public void Commit(OrderedMessageView<ReadWriteMessageStream> stream);
    }

    public static class BusObjectExtensions
    {
        /// <summary>
        /// Enqueue a bus object from a given property view model
        /// </summary>
        /// <param name="self"></param>
        /// <param name="propertyViewModel"></param>
        public static void EnqueueBus(this IBusObject self, IPropertyViewModel propertyViewModel)
        {
            propertyViewModel.GetRoot().GetService<IBusPropertyService>()?.Enqueue(self);
        }
        
        /// <summary>
        /// Enqueue a bus object from a property view model
        /// </summary>
        /// <param name="self"></param>
        /// <typeparam name="T"></typeparam>
        public static void EnqueueBus<T>(this T self) where T : IBusObject, IPropertyViewModel
        {
            self.EnqueueBus(self);
        }
    }
}

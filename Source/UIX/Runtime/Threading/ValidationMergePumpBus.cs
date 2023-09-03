using System.Collections.Generic;
using Studio.ViewModels.Workspace.Objects;

namespace Runtime.Threading
{
    public class ValidationMergePumpBus
    {
        /// <summary>
        /// Increment a validation object
        /// </summary>
        /// <param name="validationObject">the object</param>
        /// <param name="increment">the count to increment by</param>
        public static void Increment(ValidationObject validationObject, uint increment)
        {
            _bus.Add(() => { validationObject.IncrementCountNoRaise(increment); }, validationObject);
        }

        /// <summary>
        /// Merge the collection
        /// </summary>
        /// <param name="objects"></param>
        private static void CollectionMerge(IEnumerable<ValidationObject> objects)
        {
            foreach (ValidationObject validationObject in objects)
            {
                validationObject.NotifyCountChanged();
            }
        }

        /// <summary>
        /// Underlying bus
        /// </summary>
        private static CollectionMergePumpBus<ValidationObject> _bus = new()
        {
            CollectionMerge = CollectionMerge
        };
    }
}
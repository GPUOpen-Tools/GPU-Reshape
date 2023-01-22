namespace Studio.ViewModels.Documents
{
    public interface IDescriptorObject
    {
        /// <summary>
        /// Descriptor getter
        /// </summary>
        public IDescriptor? Descriptor { get; }
    }
}
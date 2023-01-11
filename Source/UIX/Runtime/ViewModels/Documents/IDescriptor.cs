namespace Studio.ViewModels.Documents
{
    public interface IDescriptor
    {
        /// <summary>
        /// Sortable identifier for unique comparisons
        /// </summary>
        object? Identifier { get; }
    }
}
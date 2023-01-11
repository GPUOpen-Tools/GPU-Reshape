namespace Studio.ViewModels.Documents
{
    public class WelcomeDescriptor : IDescriptor
    {
        /// <summary>
        /// Sortable identifier
        /// </summary>
        public object? Identifier { get; set; } = typeof(WelcomeDescriptor);
    }
}
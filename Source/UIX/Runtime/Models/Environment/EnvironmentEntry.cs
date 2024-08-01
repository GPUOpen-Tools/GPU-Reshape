namespace Studio.Models.Environment
{
    public class EnvironmentEntry
    {
        /// <summary>
        /// Environment key
        /// </summary>
        public string Key { get; set; } = string.Empty;

        /// <summary>
        /// Assigned value to key, may be empty
        /// </summary>
        public string Value { get; set; } = string.Empty;
    }
}
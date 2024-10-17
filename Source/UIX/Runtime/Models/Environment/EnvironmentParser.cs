using System.Collections.Generic;
using System.Text;

namespace Studio.Models.Environment
{
    public class EnvironmentParser
    {
        /// <summary>
        /// Parse an environment string
        /// </summary>
        public static EnvironmentEntry[] Parse(string str)
        {
            List<EnvironmentEntry> entries = new();

            // Entry to be pushed
            EnvironmentEntry? pendingEntry = null;
            
            for (int i = 0; i < str.Length;)
            {
                if (!SkipWhitespaces(str, ref i))
                {
                    break;
                }

                // Push previous entry if any
                if (pendingEntry != null)
                {
                    entries.Add(pendingEntry);
                }
                
                // Create new entry
                pendingEntry = new();
                pendingEntry.Key = ParseSegment(str, ref i);
                
                if (!SkipWhitespaces(str, ref i))
                {
                    break;
                }
                
                // Has value?
                if (str[i] != '=')
                {
                    continue;
                }

                i++;
                
                if (!SkipWhitespaces(str, ref i))
                {
                    break;
                }
                
                // Parse value
                pendingEntry.Value = ParseSegment(str, ref i);
            }

            // May have dangling entry
            if (pendingEntry != null)
            {
                entries.Add(pendingEntry);
            }

            return entries.ToArray();
        }

        /// <summary>
        /// Assemble a list of environment entries
        /// </summary>
        public static string Assemble(EnvironmentEntry[] entries)
        {
            StringBuilder builder = new();

            foreach (EnvironmentEntry entry in entries)
            {
                AssembleSegment(builder, entry.Key);

                // Value is optional
                if (entry.Value != string.Empty)
                {
                    builder.Append('=');
                    AssembleSegment(builder, entry.Value);
                }

                builder.Append(' ');
            }

            return builder.ToString();
        }

        /// <summary>
        /// Assemble a string, handles wrapping
        /// </summary>
        private static void AssembleSegment(StringBuilder builder, string str)
        {
            // If it has spaces, we need to wrap the value
            bool hasSpaces = str.Contains(' ');
                    
            if (hasSpaces)
            {
                builder.Append('"');
            }
                    
            builder.Append(str);
                    
            if (hasSpaces)
            {
                builder.Append('"');
            }
        }

        /// <summary>
        /// Skip all whitespaces
        /// </summary>
        /// <returns>false if out of characters</returns>
        private static bool SkipWhitespaces(string str, ref int i)
        {
            while (i < str.Length && char.IsWhiteSpace(str[i]))
            {
                i++;
            }

            return i < str.Length;
        }

        /// <summary>
        /// Parse a segment, handles wrapped values
        /// </summary>
        /// <returns>inner segment, does not include quotations</returns>
        private static string ParseSegment(string str, ref int i)
        {
            int end;
            
            // Wrapped?
            if (str[i] == '"')
            {
                end = str.IndexOf('"', i + 1);
            }
            else
            {
                end = str.IndexOfAny(new[] { ' ', '=' }, i + 1);
            }
            
            // End may be the end of the string
            if (end == -1)
            {
                end = str.Length;
            }

            string segment;
            
            // If wrapped, remove the quotes
            if (str[i] == '"')
            {
                i++;
                
                int endOffset = str[end - 1] == '"' ? 2 : 0;
                segment = str.Substring(i, end - i - endOffset);

                end++;
            }
            else
            {
                segment = str.Substring(i, end - i);
            }

            i = end;
            return segment;
        }
    }
}
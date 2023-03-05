using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;

namespace Runtime.Models.Query
{
    public class QueryParser
    {
        /// <summary>
        /// Standardized query pattern
        /// </summary>
        public static readonly string QueryPattern = "([A-Za-z]+):([^\\s@:+!\"]+|\"([^\"]*)\")";

        /// <summary>
        /// Parse all attributes from a given query
        /// </summary>
        /// <param name="query">string query</param>
        /// <returns>null if failed</returns>
        public static QueryAttribute[]? GetAttributes(string query)
        {
            // Remove endpoints
            query = query.Trim();

            // No query?
            if (query == string.Empty)
            {
                return new QueryAttribute[] { };
            }

            // Try to match
            MatchCollection collection = Regex.Matches(query, QueryPattern);

            // Must have at least one match if there's a single character
            if (collection.Count == 0)
            {
                return null;
            }

            // Get last group for length test
            Group lastGroup = collection.Last().Groups[0];

            // Total size must match
            if (lastGroup.Index + lastGroup.Length != query.Length)
            {
                return null;
            }

            // To attribute list
            return collection.Select(x => new QueryAttribute()
            {
                Offset = x.Index,
                Length = x.Length,
                Key = x.Groups[1].Value,
                Value = x.Groups[2].Value
            }).ToArray();
        }

        /// <summary>
        /// Create a connection query from a string
        /// </summary>
        /// <param name="query">query string</param>
        /// <param name="viewModel">resulting view model</param>
        /// <returns>failure status</returns>
        public static QueryResult FromString(QueryType type, string query, out QueryParser? queryObject)
        {
            // Try to parse attributes
            QueryAttribute[]? matches = GetAttributes(query);
            if (matches == null)
            {
                queryObject = null;
                return QueryResult.Invalid;
            }

            // Construct query
            return FromAttributes(type, matches, out queryObject);
        }

        /// <summary>
        /// Create a connection query from attributes
        /// </summary>
        /// <param name="attributes">attribute list</param>
        /// <param name="queryObject">resulting view model</param>
        /// <returns></returns>
        public static QueryResult FromAttributes(QueryType type, QueryAttribute[] attributes, out QueryParser? queryObject)
        {
            // Default failed state
            queryObject = null;

            // Create temporary query
            var query = new QueryParser();

            // Handle all attributes
            foreach (QueryAttribute attribute in attributes)
            {
                // Invalid?
                if (attribute.Key.Length == 0 || attribute.Value.Length == 0)
                {
                    return QueryResult.Invalid;
                }

                // Must have key
                if (!type.Variables.ContainsKey(attribute.Key))
                {
                    return QueryResult.Invalid;
                }

                // Duplicate?
                if (query._values.ContainsKey(attribute.Key))
                {
                    return QueryResult.DuplicateKey;
                }

                // Get type
                Type valueType = type.Variables[attribute.Key];

                // Handle type
                if (valueType == typeof(int))
                {
                    // Try to parse
                    if (!int.TryParse(attribute.Value, out int value))
                    {
                        return QueryResult.Invalid;
                    }

                    // Set attribute
                    query._values[attribute.Key] = value;
                }
                else if (valueType == typeof(string))
                {
                    // Set attribute
                    query._values[attribute.Key] = attribute.Value;
                }
                else if (valueType.IsEnum)
                {
                    // Try to parse
                    if (!Enum.TryParse(valueType, attribute.Value, true, out object? value))
                    {
                        return QueryResult.Invalid;
                    }
                    
                    // Set attribute
                    query._values[attribute.Key] = value!;
                }
                else
                {
                    throw new NotSupportedException("Type not supported");
                }
            }

            // Set result
            queryObject = query;

            // OK
            return QueryResult.OK;
        }

        /// <summary>
        /// Check if a key exists
        /// </summary>
        /// <param name="key"></param>
        /// <returns></returns>
        public bool HasValue(string key)
        {
            return _values.ContainsKey(key);
        }

        /// <summary>
        /// Get an integer value
        /// </summary>
        /// <param name="key"></param>
        /// <returns>null if not present</returns>
        public int? GetInt(string key)
        {
            if (_values.TryGetValue(key, out object? value))
            {
                return (int)value;
            }

            return null;
        }

        /// <summary>
        /// Get a string value
        /// </summary>
        /// <param name="key"></param>
        /// <returns>null if not present</returns>
        public string? GetString(string key)
        {
            if (_values.TryGetValue(key, out object? value))
            {
                return (string)value;
            }

            return null;
        }

        /// <summary>
        /// Get a typed value
        /// </summary>
        /// <param name="key"></param>
        /// <typeparam name="T">query type must match</typeparam>
        /// <returns>null if not present</returns>
        public T? Get<T>(string key) where T : struct
        {
            if (_values.TryGetValue(key, out object? value))
            {
                return (T)value;
            }

            return null;
        }

        /// <summary>
        /// Internal values
        /// </summary>
        private Dictionary<string, object> _values = new();
    }
}
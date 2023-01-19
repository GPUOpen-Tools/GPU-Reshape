using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;
using ReactiveUI;
using Runtime.Models.Connections;
using Studio.Models.Workspace;

namespace Studio.ViewModels.Connections
{
    public class ConnectionQueryViewModel : ReactiveObject
    {
        /// <summary>
        /// Endpoint IP
        /// </summary>
        public string? IPvX
        {
            get => _ipvX;
            set => this.RaiseAndSetIfChanged(ref _ipvX, value);
        }

        /// <summary>
        /// Endpoint port
        /// </summary>
        public int? Port
        {
            get => _port;
            set => this.RaiseAndSetIfChanged(ref _port, value);
        }

        /// <summary>
        /// Host resolver decoration filter
        /// </summary>
        public string? ApplicationFilter
        {
            get => _applicationFilter;
            set => this.RaiseAndSetIfChanged(ref _applicationFilter, value);
        }

        /// <summary>
        /// Host resolver pid filter
        /// </summary>
        public int? ApplicationPid
        {
            get => _applicationPid;
            set => this.RaiseAndSetIfChanged(ref _applicationPid, value);
        }

        /// <summary>
        /// Host resolver api filter
        /// </summary>
        public string? ApplicationAPI
        {
            get => _applicationAPI;
            set => this.RaiseAndSetIfChanged(ref _applicationAPI, value);
        }

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
        public static ConnectionStatus FromString(string query, out ConnectionQueryViewModel? viewModel)
        {
            // Try to parse attributes
            QueryAttribute[]? matches = GetAttributes(query);
            if (matches == null)
            {
                viewModel = null;
                return ConnectionStatus.QueryInvalid;
            }

            // Construct query
            return FromAttributes(matches, out viewModel);
        }

        /// <summary>
        /// Create a connection query from attributes
        /// </summary>
        /// <param name="attributes">attribute list</param>
        /// <param name="viewModel">resulting view model</param>
        /// <returns></returns>
        public static ConnectionStatus FromAttributes(QueryAttribute[] attributes, out ConnectionQueryViewModel? viewModel)
        {
            // Default failed state
            viewModel = null;

            // Create temporary query
            var query = new ConnectionQueryViewModel();

            // Handle all attributes
            foreach (QueryAttribute attribute in attributes)
            {
                // Invalid?
                if (attribute.Key.Length == 0 || attribute.Value.Length == 0)
                {
                    return ConnectionStatus.QueryInvalid;
                }

                // Handle symbol
                switch (attribute.Key)
                {
                    default:
                    {
                        return ConnectionStatus.QueryInvalid;
                    }
                    case "ip":
                    {
                        if (query.IPvX != null)
                        {
                            return ConnectionStatus.QueryDuplicateKey;
                        }

                        // Set attribute
                        query.IPvX = attribute.Value;
                        
                        // Special value
                        if (query.IPvX.Equals("localhost", StringComparison.InvariantCultureIgnoreCase))
                        {
                            query.IPvX = "127.0.0.1";
                        }
                        break;
                    }
                    case "port":
                    {
                        if (query.Port.HasValue)
                        {
                            return ConnectionStatus.QueryDuplicateKey;
                        }

                        // Try to parse
                        if (!int.TryParse(attribute.Value, out int port))
                        {
                            return ConnectionStatus.QueryInvalidPort;
                        }

                        // Set attribute
                        query.Port = port;
                        break;
                    }
                    case "app":
                    {
                        if (query.ApplicationFilter != null)
                        {
                            return ConnectionStatus.QueryDuplicateKey;
                        }

                        // Set attribute
                        query.ApplicationFilter = attribute.Value;
                        break;
                    }
                    case "pid":
                    {
                        if (query.ApplicationPid.HasValue)
                        {
                            return ConnectionStatus.QueryDuplicateKey;
                        }

                        // Try to parse
                        if (!int.TryParse(attribute.Value, out int pid))
                        {
                            return ConnectionStatus.QueryInvalidPID;
                        }

                        // Set attribute
                        query.ApplicationPid = pid;
                        break;
                    }
                    case "api":
                    {
                        if (query.ApplicationAPI != null)
                        {
                            return ConnectionStatus.QueryDuplicateKey;
                        }

                        // Set attribute
                        query.ApplicationAPI = attribute.Value;
                        break;
                    }
                }
            }

            // Set result
            viewModel = query;

            // OK
            return ConnectionStatus.None;
        }

        /// <summary>
        /// Internal ip
        /// </summary>
        private string? _ipvX;

        /// <summary>
        /// Internal port
        /// </summary>
        private int? _port;

        /// <summary>
        /// Internal PID
        /// </summary>
        private int? _applicationPid;

        /// <summary>
        /// Internal filter
        /// </summary>
        private string? _applicationFilter;
        
        /// <summary>
        /// Internal API
        /// </summary>
        private string? _applicationAPI;
    }
}
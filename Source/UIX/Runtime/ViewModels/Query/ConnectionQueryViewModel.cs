using System;
using ReactiveUI;
using Runtime.Models.Query;
using Studio.Models.Workspace;

namespace Studio.ViewModels.Query
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
        /// Create a query from attributes
        /// </summary>
        /// <param name="attributes">all attributes</param>
        /// <param name="viewModel"></param>
        /// <returns>result code</returns>
        /// <exception cref="ArgumentOutOfRangeException"></exception>
        public static ConnectionStatus FromAttributes(QueryAttribute[] attributes, out ConnectionQueryViewModel? viewModel)
        {
            QueryResult result = QueryParser.FromAttributes(new QueryType()
            {
                Variables = new()
                {
                    { "ip", typeof(string) },
                    { "port", typeof(int) },
                    { "app", typeof(string) },
                    { "pid", typeof(int) },
                    { "api", typeof(string) }
                }
            }, attributes, out QueryParser? query);

            // Handle result
            switch (result)
            {
                case QueryResult.OK:
                    break;
                case QueryResult.Invalid:
                    viewModel = null;
                    return ConnectionStatus.QueryInvalid;
                case QueryResult.DuplicateKey:
                    viewModel = null;
                    return ConnectionStatus.QueryDuplicateKey;
                default:
                    throw new ArgumentOutOfRangeException();
            }

            // Create VM
            viewModel = new ConnectionQueryViewModel()
            {
                IPvX = query.GetString("ip"),
                Port = query.GetInt("port"),
                ApplicationFilter = query.GetString("app"),
                ApplicationPid = query.GetInt("pid"),
                ApplicationAPI = query.GetString("api"),
            };

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
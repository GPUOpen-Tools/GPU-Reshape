using System;
using ReactiveUI;
using Runtime.Models.Query;
using Studio.Models.Workspace;
using Studio.Models.Workspace.Objects;

namespace Studio.ViewModels.Query
{
    public class PipelineFilterQueryViewModel : ReactiveObject
    {
        /// <summary>
        /// Filter type
        /// </summary>
        public PipelineType? Type
        {
            get => _type;
            set => this.RaiseAndSetIfChanged(ref _type, value);
        }

        /// <summary>
        /// Filter name
        /// </summary>
        public string? Name
        {
            get => _name;
            set => this.RaiseAndSetIfChanged(ref _name, value);
        }

        /// <summary>
        /// Filter decorator
        /// </summary>
        public string Decorator
        {
            get => _decorator;
            set => this.RaiseAndSetIfChanged(ref _decorator, value);
        }
        
        /// <summary>
        /// Create a query from attributes
        /// </summary>
        /// <param name="attributes">given attributes</param>
        /// <param name="viewModel"></param>
        /// <returns>result code</returns>
        public static QueryResult FromAttributes(QueryAttribute[] attributes, out PipelineFilterQueryViewModel? viewModel)
        {
            QueryResult result = QueryParser.FromAttributes(new QueryType()
            {
                Variables = new()
                {
                    { "type", typeof(PipelineType) },
                    { "name", typeof(string) },
                }
            }, attributes, out QueryParser? query);

            // Success?
            if (result != QueryResult.OK)
            {
                viewModel = null;
                return result;
            }

            // Create VM
            viewModel = new PipelineFilterQueryViewModel()
            {
                Type = query.Get<PipelineType>("type"),
                Name = query.GetString("name")
            };

            // OK
            return QueryResult.OK;
        }
        
        /// <summary>
        /// Internal type
        /// </summary>
        private PipelineType? _type;

        /// <summary>
        /// Internal name
        /// </summary>
        private string? _name;

        /// <summary>
        /// Internal decorator
        /// </summary>
        private string _decorator = string.Empty;
    }
}
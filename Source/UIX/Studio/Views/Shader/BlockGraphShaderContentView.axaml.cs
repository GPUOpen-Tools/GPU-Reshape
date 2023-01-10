using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Reactive.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Avalonia.Media;
using AvaloniaEdit.TextMate;
using AvaloniaEdit.Utils;
using AvaloniaGraphControl;
using DynamicData;
using DynamicData.Binding;
using Newtonsoft.Json;
using ReactiveUI;
using Studio.Models.Logging;
using Studio.Services;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Workspace.Objects;
using Studio.Views.Editor;
using TextMateSharp.Grammars;
using ShaderViewModel = Studio.ViewModels.Documents.ShaderViewModel;

namespace Studio.Views.Shader
{
    public partial class BlockGraphShaderContentView : UserControl, IViewFor
    {
        /// <summary>
        /// Assigned view model
        /// </summary>
        public object? ViewModel { get => DataContext; set => DataContext = value; }
        
        public BlockGraphShaderContentView()
        {
            InitializeComponent();

            // Bind contents
            this.WhenAnyValue(x => x.DataContext)
                .WhereNotNull()
                .Cast<BlockGraphShaderContentViewModel>()
                .WhereNotNull()
                .Subscribe(x =>
            {
                x.Object.WhenAnyValue(x => x.BlockGraph).WhereNotNull().Subscribe(nodeGraph =>
                {
                    CreateGraph(nodeGraph);
                });
            });
        }

        /// <summary>
        /// Get the color from a descriptor
        /// </summary>
        /// <param name="descriptor"></param>
        /// <returns></returns>
        private Color GetColor(string descriptor)
        {
            if (descriptor == string.Empty)
            {
                return Colors.White;
            }

            return Color.Parse(descriptor);
        }
        
        /// <summary>
        /// Create a graph from a node graph descriptor
        /// </summary>
        /// <param name="nodeGraph"></param>
        private void CreateGraph(string nodeGraph)
        {
            // Attempt to deserialize
            dynamic? blockStructure;
            try
            {
                // Attempt to deserialize
                blockStructure = JsonConvert.DeserializeObject(nodeGraph);
                if (blockStructure == null)
                {
                    return;
                }

                // Block lookup
                Dictionary<uint, Graphing.Block> objects = new();
                
                // Create graph
                var graph = new Graph();

                // Create all blocks
                foreach (var block in blockStructure.blocks)
                {
                    objects.Add(uint.Parse(block.Name), new Graphing.Block()
                    {
                        Name = block.Value.name,
                        Color = GetColor((string)block.Value.color),
                    });
                }

                // Create all edges
                foreach (var edge in blockStructure.edges)
                {
                    graph.Edges.Add(new Graphing.Edge(objects[(uint)edge.from], objects[(uint)edge.to])
                    {
                        Color = GetColor((string)edge.color)
                    });
                }
            
                // OK
                GraphControl.Graph = graph;
            }
            catch (Exception)
            {
                // Inform of failure
                App.Locator.GetService<ILoggingService>()?.ViewModel.Events.Add(new LogEvent()
                {
                    Severity = LogSeverity.Error,
                    Message = $"Failed to deserialize block graph"
                });

                if (Debugger.IsAttached)
                {
                    throw;
                }
            }
        }
    }
}
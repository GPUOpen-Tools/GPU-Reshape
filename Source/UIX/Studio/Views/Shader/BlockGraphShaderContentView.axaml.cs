// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Media;
using AvaloniaGraphControl;
using DynamicData;
using Newtonsoft.Json;
using ReactiveUI;
using Studio.Extensions;
using Studio.Models.Logging;
using Studio.Services;
using Studio.ViewModels.Shader;

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
                .CastNullable<BlockGraphShaderContentViewModel>()
                .Subscribe(viewModel =>
            {
                viewModel.WhenAnyValue(x => x.Object).WhereNotNull().Subscribe(x =>
                {
                    x.WhenAnyValue(y => y.BlockGraph).Subscribe(contents =>
                    {
                        CreateGraph(contents);
                    });
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

                // Visit all functions
                foreach (var function in blockStructure.functions)
                {
                    // Create all blocks
                    foreach (var block in function.blocks)
                    {
                        objects.Add(uint.Parse(block.Name), new Graphing.Block()
                        {
                            Name = block.Value.name,
                            Color = new SolidColorBrush(GetColor((string)block.Value.color)),
                        });
                    }

                    // Create all edges
                    foreach (var edge in function.edges)
                    {
                        graph.Edges.Add(new Graphing.Edge(objects[(uint)edge.from], objects[(uint)edge.to])
                        {
                            Color = new SolidColorBrush(GetColor((string)edge.color))
                        });
                    }

                    // Edge case
                    if (function.edges.Count == 0)
                    {
                        graph.Edges.Add(new Graphing.Edge(objects.First().Value, objects.First().Value)
                        {
                            Color = new SolidColorBrush(Colors.White)
                        });
                    }
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
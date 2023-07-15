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
using Avalonia.Threading;
using Message.CLR;
using Runtime.ViewModels.Shader;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.ViewModels.Workspace.Services
{
    public class ShaderCodeService : IPropertyService, IShaderCodeService, Bridge.CLR.IBridgeListener
    {
        /// <summary>
        /// Connection for this listener
        /// </summary>
        public IConnectionViewModel? ConnectionViewModel { get; set; }

        /// <summary>
        /// Bridge handler
        /// </summary>
        /// <param name="streams"></param>
        /// <param name="count"></param>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            if (!streams.GetSchema().IsOrdered())
            {
                return;
            }

            // Create view
            var view = new OrderedMessageView(streams);

            // Consume all messages
            lock (this)
            {
                foreach (OrderedMessage message in view)
                {
                    switch (message.ID)
                    {
                        case ShaderCodeMessage.ID:
                        {
                            var shaderCode = message.Get<ShaderCodeMessage>();

                            // Try to get the view model
                            if (!_pendingShaderViewModels.TryGetValue(shaderCode.shaderUID, out PendingEntry? entry) || entry.ShaderViewModel == null)
                            {
                                continue;
                            }

                            // Failed to find?
                            if (shaderCode.found == 0)
                            {
                                Dispatcher.UIThread.InvokeAsync(() => { entry.ShaderViewModel.AsyncStatus |= AsyncShaderStatus.NotFound; });
                            }

                            // Only native?
                            if (shaderCode.native == 1)
                            {
                                Dispatcher.UIThread.InvokeAsync(() => { entry.ShaderViewModel.AsyncStatus |= AsyncShaderStatus.NoDebugSymbols; });
                            }
                            break;
                        }
                        case ShaderCodeFileMessage.ID:
                        {
                            var shaderCode = message.Get<ShaderCodeFileMessage>();

                            // Try to get the view model
                            if (!_pendingShaderViewModels.TryGetValue(shaderCode.shaderUID, out PendingEntry? entry) || entry.ShaderViewModel == null)
                            {
                                continue;
                            }

                            // Flatten dynamics
                            uint fileUID = shaderCode.fileUID;
                            string code = shaderCode.code.String;
                            string filename = shaderCode.filename.String;

                            // Copy contents
                            Dispatcher.UIThread.InvokeAsync(() =>
                            {
                                // First file?
                                if (fileUID == 0)
                                {
                                    entry.ShaderViewModel.Contents = code;
                                    entry.ShaderViewModel.Filename = filename;
                                }

                                // Create general model
                                entry.ShaderViewModel.FileViewModels.Add(new ShaderFileViewModel()
                                {
                                    Contents = code,
                                    UID = fileUID,
                                    Filename = filename
                                });
                            });
                            break;
                        }
                        case ShaderILMessage.ID:
                        {
                            var shaderCode = message.Get<ShaderILMessage>();

                            // Try to get the view model
                            if (!_pendingShaderViewModels.TryGetValue(shaderCode.shaderUID, out PendingEntry? entry) || entry.ShaderViewModel == null)
                            {
                                continue;
                            }

                            // Found?
                            if (shaderCode.found == 0)
                            {
                                UInt64 uid = shaderCode.shaderUID;

                                // Set contents
                                Dispatcher.UIThread.InvokeAsync(() => { entry.ShaderViewModel.IL = $"Shader {{{uid}}} not found"; });
                                continue;
                            }

                            // Flatten dynamics
                            string code = shaderCode.code.String;

                            // Copy contents
                            Dispatcher.UIThread.InvokeAsync(() => { entry.ShaderViewModel.IL = code; });
                            break;
                        }
                        case ShaderBlockGraphMessage.ID:
                        {
                            var shaderCode = message.Get<ShaderBlockGraphMessage>();

                            // Try to get the view model
                            if (!_pendingShaderViewModels.TryGetValue(shaderCode.shaderUID, out PendingEntry? entry) || entry.ShaderViewModel == null)
                            {
                                continue;
                            }

                            // Flatten dynamics
                            string nodes = shaderCode.nodes.String;

                            // Copy contents
                            Dispatcher.UIThread.InvokeAsync(() => { entry.ShaderViewModel.BlockGraph = nodes; });
                            break;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Enqueue a request for shader contents
        /// </summary>
        /// <param name="shaderViewModel"></param>
        public void EnqueueShaderContents(ShaderViewModel shaderViewModel)
        {
            lock (this)
            {
                // Get entry
                PendingEntry entry = GetEntry(shaderViewModel.GUID);

                // Valid or already pooled?
                if (ConnectionViewModel == null || entry.State.HasFlag(ShaderCodePoolingState.Contents))
                {
                    return;
                }

                // Update entry
                entry.State |= ShaderCodePoolingState.Contents;
                entry.ShaderViewModel = shaderViewModel;

                // Add request
                var request = ConnectionViewModel.GetSharedBus().Add<GetShaderCodeMessage>();
                request.poolCode = 1;
                request.shaderUID = shaderViewModel.GUID;
            }
        }

        /// <summary>
        /// Enqueue a request for shader IL
        /// </summary>
        /// <param name="shaderViewModel"></param>
        public void EnqueueShaderIL(ShaderViewModel shaderViewModel)
        {
            lock (this)
            {
                // Get entry
                PendingEntry entry = GetEntry(shaderViewModel.GUID);

                // Valid or already pooled?
                if (ConnectionViewModel == null || entry.State.HasFlag(ShaderCodePoolingState.IL))
                {
                    return;
                }

                // Update entry
                entry.State |= ShaderCodePoolingState.IL;
                entry.ShaderViewModel = shaderViewModel;

                // Add request
                var request = ConnectionViewModel.GetSharedBus().Add<GetShaderILMessage>();
                request.shaderUID = shaderViewModel.GUID;
            }
        }

        /// <summary>
        /// Enqueue a request for shader block graph
        /// </summary>
        /// <param name="shaderViewModel"></param>
        public void EnqueueShaderBlockGraph(ShaderViewModel shaderViewModel)
        {
            lock (this)
            {
                // Get entry
                PendingEntry entry = GetEntry(shaderViewModel.GUID);

                // Valid or already pooled?
                if (ConnectionViewModel == null || entry.State.HasFlag(ShaderCodePoolingState.BlockGraph))
                {
                    return;
                }

                // Update entry
                entry.State |= ShaderCodePoolingState.BlockGraph;
                entry.ShaderViewModel = shaderViewModel;

                // Add request
                var request = ConnectionViewModel.GetSharedBus().Add<GetShaderBlockGraphMessage>();
                request.shaderUID = shaderViewModel.GUID;
            }
        }

        /// <summary>
        /// Get an existing entry
        /// </summary>
        /// <param name="guid"></param>
        /// <returns></returns>
        private PendingEntry GetEntry(UInt64 guid)
        {
            if (!_pendingShaderViewModels.TryGetValue(guid, out PendingEntry? entry))
            {
                entry = new PendingEntry()
                {
                    State = ShaderCodePoolingState.None
                };

                // Populate missing entry
                _pendingShaderViewModels.Add(guid, entry);
            }

            return entry;
        }

        /// <summary>
        /// Internal pending entry
        /// </summary>
        private class PendingEntry
        {
            /// <summary>
            /// Current pooling state
            /// </summary>
            public ShaderCodePoolingState State;

            /// <summary>
            /// Parent view model
            /// </summary>
            public Objects.ShaderViewModel? ShaderViewModel;
        }

        /// <summary>
        /// All pending view models, i.e. in-flight
        /// </summary>
        private Dictionary<UInt64, PendingEntry> _pendingShaderViewModels = new();
    }
}
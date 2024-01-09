// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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
using Avalonia.Threading;
using Message.CLR;
using ReactiveUI;
using Runtime.ViewModels.Shader;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.ViewModels.Workspace.Services
{
    public class VersioningService : IPropertyService, IVersioningService, Bridge.CLR.IBridgeListener
    {
        /// <summary>
        /// Connection for this listener
        /// </summary>
        public IConnectionViewModel? ConnectionViewModel
        {
            get => _connectionViewModel;
            set
            {
                _connectionViewModel = value;

                if (_connectionViewModel != null)
                {
                    OnConnectionChanged();
                }
            }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public VersioningService()
        {
        }
        
        /// <summary>
        /// Try to find a resource
        /// </summary>
        /// <param name="puid">given puid</param>
        /// <param name="version">exact version to fetch</param>
        /// <returns></returns>
        public Resource? GetResource(uint puid, uint version)
        {
            lock (this)
            {
                foreach (Branch branch in _branches)
                {
                    // If the current branch is ahead, the resource was not found, this could be caused by de-synchronization
                    if (branch.Head > version)
                    {
                        // TODO: We need queue wise synchronization, the branches may be collapsed prematurely
                        //       Ignore this for now.
                        // return null;
                    }

                    // Try to find the puid
                    if (branch.Resources.TryGetValue(puid, out Resource resource))
                    {
                        return resource;
                    }
                }

                // No branch nor resource was found
                return null;
            }
        }

        /// <summary>
        /// Invoked on connection changes
        /// </summary>
        private void OnConnectionChanged()
        {
            // Register
            ConnectionViewModel!.Bridge?.Register(this);
            ConnectionViewModel.Bridge?.Register(ResourceVersionMessage.ID, this);
            
            // Request version summarization
            ConnectionViewModel.GetSharedBus().Add<GetVersionSummarizationMessage>();
        }

        /// <summary>
        /// Invoked on destruction
        /// </summary>
        public void Destruct()
        {
            // Remove listeners
            ConnectionViewModel!.Bridge?.Deregister(this);
            ConnectionViewModel?.Bridge?.Deregister(ResourceVersionMessage.ID, this);
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        /// <param name="streams"></param>
        /// <param name="count"></param>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            if (streams.GetSchema().IsOrdered())
            {
                HandleVersioning(streams);
            }
            else
            {
                HandleResourceVersions(streams);
            }
        }

        /// <summary>
        /// Handle all ordered versioning
        /// </summary>
        /// <param name="streams"></param>
        private void HandleVersioning(ReadOnlyMessageStream streams)
        {
            lock (this)
            {
                foreach (OrderedMessage message in new OrderedMessageView(streams))
                {
                    switch (message.ID)
                    {
                        case VersionSummarizationMessage.ID:
                        {
                            var versionSummarization = message.Get<VersionSummarizationMessage>();

                            // Create summarization branch if needed
                            if (_branches.Count == 0 || _branches.Last().Head != versionSummarization.head)
                            {
                                // Create branch
                                var branch = new Branch()
                                {
                                    Head = versionSummarization.head
                                };
                            
                                // Add to trackers
                                _branches.Add(branch);
                                _branchLookup.Add(branch.Head, branch);
                            }
                            break;
                        }
                        case VersionBranchMessage.ID:
                        {
                            var versionBranch = message.Get<VersionBranchMessage>();
                            
                            // Validate we are beyond local head
                            Debug.Assert(_branches.Count == 0 || _branches.Last().Head < versionBranch.head, "Branching not on local head");

                            // Create branch
                            var branch = new Branch()
                            {
                                Head = versionBranch.head
                            };
                            
                            // Add to trackers
                            _branches.Add(branch);
                            _branchLookup.Add(branch.Head, branch);
                            break;
                        }
                        case VersionCollapseMessage.ID:
                        {
                            var versionCollapse = message.Get<VersionCollapseMessage>();

                            // Get target branch
                            if (!_branchLookup.TryGetValue(versionCollapse.head, out Branch? target))
                            {
                                break;
                            }

                            // Collapse all branches up until target
                            int rangeIndex = 0;
                            for (; rangeIndex < _branches.Count; rangeIndex++)
                            {
                                Branch branch = _branches[rangeIndex];
                                
                                // Beyond segmentation point?
                                if (branch.Head >= versionCollapse.head)
                                {
                                    break;
                                }

                                // Collapse all resources into target
                                foreach (var kv in branch.Resources)
                                {
                                    // Merge upward if not present
                                    target.Resources.TryAdd(kv.Key, kv.Value);
                                }
                                
                                // Remove lookup
                                _branchLookup.Remove(branch.Head);
                            }
                            
                            // Remove collapsed branches
                            _branches.RemoveRange(0, rangeIndex);
                            break;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Handle all linear resource versions
        /// </summary>
        /// <param name="streams"></param>
        private void HandleResourceVersions(ReadOnlyMessageStream streams)
        {
            lock (this)
            {
                // If no branch is available this implies we're snooping on message streams
                // submitted before the summarization request.
                if (_branches.Count == 0)
                {
                    return;
                }
            
                // Always append on last
                Branch branch = _branches.Last();
                
                // Add all resources
                foreach (ResourceVersionMessage message in new DynamicMessageView<ResourceVersionMessage>(streams))
                {
                    // Deletion?
                    if (message.version == UInt32.MaxValue)
                    {
                        branch.Resources.Remove(message.puid);
                        continue;
                    }
                    
                    // Validate streaming synchronization
                    Debug.Assert(message.version == branch.Head, "Versioning de-synchronization, committed resource to a former branch");

                    // Add resource (duplicate keys are fine, it may be a recommit)
                    branch.Resources[message.puid] = new Resource()
                    {
                        PUID = message.puid,
                        Version = branch.Head,
                        Name = message.name.String,
                        Width = message.width,
                        Height = message.height,
                        Depth = message.depth,
                        Format = message.format.String
                    };
                }
            }
        }
        
        private class Branch
        {
            /// <summary>
            /// Head index of this branch
            /// </summary>
            public uint Head = 0;

            /// <summary>
            /// All resource mappings in this branch
            /// </summary>
            public Dictionary<uint, Resource> Resources = new();
        }

        /// <summary>
        /// Branch lookup
        /// </summary>
        private Dictionary<uint, Branch> _branchLookup = new();

        /// <summary>
        /// All branches
        /// </summary>
        private List<Branch> _branches = new();
        
        /// <summary>
        /// Internal connection
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;
    }
}
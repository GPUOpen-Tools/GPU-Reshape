using System;
using System.Collections.Generic;
using Avalonia.Threading;
using Message.CLR;

namespace Studio.ViewModels.Workspace.Listeners
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
                            if (!_pendingShaderViewModels.TryGetValue(shaderCode.shaderUID, out Objects.ShaderViewModel? shaderViewModel))
                            {
                                continue;
                            }

                            // Failed to find?
                            if (shaderCode.found == 0)
                            {
                                ShaderCodeMessage.FlatInfo flat = shaderCode.Flat;

                                Dispatcher.UIThread.InvokeAsync(() =>
                                {
                                    shaderViewModel.Contents = $"Shader {{{flat.shaderUID}}} not found";
                                });
                            }
                            break;
                        }
                        case ShaderCodeFileMessage.ID:
                        {
                            var shaderCode = message.Get<ShaderCodeFileMessage>();

                            // Try to get the view model
                            if (!_pendingShaderViewModels.TryGetValue(shaderCode.shaderUID, out Objects.ShaderViewModel? shaderViewModel))
                            {
                                continue;
                            }

                            // Flatten dynamics
                            string code = shaderCode.code.String;
                            string filename = shaderCode.filename.String;

                            // Copy contents
                            Dispatcher.UIThread.InvokeAsync(() =>
                            {
                                shaderViewModel.Contents = code;
                                shaderViewModel.Filename = filename;
                            });
                            
                            // Remove from future lookups
                            _pendingShaderViewModels.Remove(shaderCode.shaderUID);
                            break;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Enqueue a shader for source code pooling
        /// </summary>
        /// <param name="shaderViewModel"></param>
        public void EnqueueShader(Objects.ShaderViewModel shaderViewModel)
        {
            lock (this)
            {
                // Avoid double submissions
                if (_pendingShaderViewModels.ContainsKey(shaderViewModel.GUID))
                {
                    return;
                }
                
                // Add pending
                _pendingShaderViewModels.Add(shaderViewModel.GUID, shaderViewModel);

                // Not bound?
                if (ConnectionViewModel == null)
                {
                    return;
                }
                
                // Add request
                var request = ConnectionViewModel.GetSharedBus().Add<GetShaderCodeMessage>();
                request.poolCode = 1;
                request.shaderUID = shaderViewModel.GUID;
            }
        }

        /// <summary>
        /// All pending view models, i.e. in-flight
        /// </summary>
        private Dictionary<UInt64, Objects.ShaderViewModel> _pendingShaderViewModels = new();
    }
}
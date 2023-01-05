using System;
using System.Collections.Generic;
using Avalonia.Threading;
using Message.CLR;
using ReactiveUI;
using Studio.Models.Workspace.Listeners;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.ViewModels.Workspace.Listeners
{
    public class ShaderMappingService : IPropertyService, IShaderMappingService, Bridge.CLR.IBridgeListener
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
            if (streams.GetSchema().IsDynamic(ShaderSourceMappingMessage.ID))
            {
                // Create view
                var view = new DynamicMessageView<ShaderSourceMappingMessage>(streams);

                // Consume all messages
                lock (this)
                {
                    foreach (ShaderSourceMappingMessage message in view)
                    {
                        OnShaderSourceMapping(message);
                    }
                }
            }
        }

        private void OnShaderSourceMapping(ShaderSourceMappingMessage message)
        {
            var segment = new ShaderSourceSegment
            {
                Extract = message.contents.String,
                Location = new ShaderLocation
                {
                    SGUID = message.shaderGUID,
                    Line = (int)message.line,
                    Column = (int)message.column
                }
            };

            // Any listeners?
            if (_enqueuedObjects.TryGetValue(message.sguid, out List<ValidationObject>? objects))
            {
                Dispatcher.UIThread.InvokeAsync(() =>
                {
                    foreach (ValidationObject validationObject in objects)
                    {
                        validationObject.Segment = segment;
                    }
                });

                // Remove from listeners
                _enqueuedObjects.Remove(message.sguid);
            }

            if (!_segments.ContainsKey(message.sguid))
            {
                _segments.Add(message.sguid, segment);
            }
        }

        public void EnqueueMessage(Objects.ValidationObject validationObject, uint sguid)
        {
            lock (this)
            {
                if (_segments.TryGetValue(sguid, out ShaderSourceSegment? segment))
                {
                    Dispatcher.UIThread.InvokeAsync(() => { validationObject.Segment = segment; });
                    return;
                }

                if (_enqueuedObjects.TryGetValue(sguid, out List<ValidationObject>? enqueued))
                {
                    enqueued.Add(validationObject);
                }
                else
                {
                    Dispatcher.UIThread.InvokeAsync(() =>
                    {
                        // Add request
                        var request = ConnectionViewModel!.GetSharedBus().Add<GetShaderSourceMappingMessage>();
                        request.sguid = sguid;
                    });
                
                    // Add to listeners
                    _enqueuedObjects.Add(sguid, new List<ValidationObject> { validationObject });
                }
            }

        }

        /// <summary>
        /// Get a segment from sguid
        /// </summary>
        /// <param name="sguid">shader guid</param>
        /// <returns>null if not found</returns>
        public ShaderSourceSegment? GetSegment(uint sguid)
        {
            lock (this)
            {
                _segments.TryGetValue(sguid, out ShaderSourceSegment? segment);
                return segment;
            }
        }

        /// <summary>
        /// All enqueued objects
        /// </summary>
        private Dictionary<uint, List<Objects.ValidationObject>> _enqueuedObjects = new();

        /// <summary>
        /// Internal segments
        /// </summary>
        private Dictionary<uint, ShaderSourceSegment> _segments = new();
    }
}
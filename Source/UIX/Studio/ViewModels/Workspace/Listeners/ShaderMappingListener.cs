using System;
using System.Collections.Generic;
using Message.CLR;
using ReactiveUI;
using Studio.Models.Workspace.Listeners;
using Studio.Models.Workspace.Objects;

namespace Studio.ViewModels.Workspace.Listeners
{
    public class ShaderMappingListener : Bridge.CLR.IBridgeListener
    {
        /// <summary>
        /// Bridge handler
        /// </summary>
        /// <param name="streams"></param>
        /// <param name="count"></param>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            if (!streams.GetSchema().IsDynamic(ShaderSourceMappingMessage.ID))
            {
                return;
            }
            
            // Create view
            var view = new DynamicMessageView<ShaderSourceMappingMessage>(streams);

            // Consume all messages
            lock (this)
            {
                foreach (ShaderSourceMappingMessage message in view)
                {
                    _segments.Add(message.sguid, new ShaderSourceSegment
                    {
                        Extract = message.contents.String,
                        Location = new ShaderLocation
                        {
                            SGUID = message.shaderGUID,
                            Line = (int)message.line,
                            Column = (int)message.column
                        }
                    });
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
                _segments.TryGetValue(sguid, out var segment);
                return segment;
            }
        }

        /// <summary>
        /// Internal segments
        /// </summary>
        private Dictionary<uint, ShaderSourceSegment> _segments = new();
    }
}
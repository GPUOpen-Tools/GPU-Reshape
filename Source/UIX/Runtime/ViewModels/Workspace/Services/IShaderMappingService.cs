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
    public interface IShaderMappingService : IPropertyService
    {
        /// <summary>
        /// Enqueue a validation object with given sguid
        /// </summary>
        /// <param name="validationObject"></param>
        /// <param name="sguid"></param>
        public void EnqueueMessage(Objects.ValidationObject validationObject, uint sguid);

        /// <summary>
        /// Get a segment from sguid
        /// </summary>
        /// <param name="sguid">shader guid</param>
        /// <returns>null if not found</returns>
        public ShaderSourceSegment? GetSegment(uint sguid);
    }
}
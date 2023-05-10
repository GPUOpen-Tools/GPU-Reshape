using System;
using System.Collections.Generic;
using Message.CLR;
using Studio.Models.Workspace.Objects;

namespace Studio.ViewModels.Workspace.Listeners
{
    public interface IVersioningService : IPropertyService
    {
        /// <summary>
        /// Try to find a resource
        /// </summary>
        /// <param name="puid">given puid</param>
        /// <param name="version">exact version to fetch</param>
        /// <returns></returns>
        public Resource? GetResource(uint puid, uint version);
    }
}

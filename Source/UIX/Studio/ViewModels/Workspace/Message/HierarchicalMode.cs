using System;
using Studio.Utils;

namespace Studio.ViewModels.Workspace.Message
{
    [Flags]
    public enum HierarchicalMode
    {
        /// <summary>
        /// No filtering
        /// </summary>
        None = 0,
        
        /// <summary>
        /// Filter first by the validation type
        /// </summary>
        FilterByType = 1,
        
        /// <summary>
        /// Filter then by the originating shader (GUID)
        /// </summary>
        FilterByShader = 2,
        
        /// <summary>
        /// Negation used for binding
        /// </summary>
        Negate = EnumUtils.NegateConstant,
        
        /// <summary>
        /// Negation helpers
        /// </summary>
        FilterByTypeNegated = FilterByType | Negate,
        FilterByShaderNegated = FilterByShader | Negate,
    }
}
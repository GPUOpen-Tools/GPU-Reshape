using System.Collections.ObjectModel;
using Runtime.ViewModels.Shader;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.ViewModels.Code;

public class CodeFileShaderMappingViewModel
{
    /// <summary>
    /// The shader that was mapped
    /// </summary>
    public ShaderViewModel ShaderViewModel { get; set; }
    
    /// <summary>
    /// The specific file of the shader that was mapped
    /// </summary>
    public ShaderFileViewModel ShaderFileViewModel { get; set; }
}

public class CodeFileShaderMappingSetViewModel
{
    /// <summary>
    /// All mappings of this file
    /// </summary>
    public ObservableCollection<CodeFileShaderMappingViewModel> Mappings { get; } = new();
}

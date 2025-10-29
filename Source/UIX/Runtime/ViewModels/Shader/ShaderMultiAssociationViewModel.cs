using System.Collections.ObjectModel;
using Studio.ViewModels.Workspace.Objects;

namespace Runtime.ViewModels.Shader;

public struct ShaderMultiAssociationPair<T> where T : class
{
    /// <summary>
    /// Shader the association was found in
    /// </summary>
    public required ShaderViewModel ShaderViewModel;

    /// <summary>
    /// The associated object for the shader
    /// </summary>
    public required T Association;
}

public class ShaderMultiAssociationViewModel<T>  where T : class
{
    /// <summary>
    /// All found associations
    /// </summary>
    public ObservableCollection<ShaderMultiAssociationPair<T>> Associations { get; } = new();
}

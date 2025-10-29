using System.Collections.ObjectModel;
using Studio.ViewModels.Code;

namespace Studio.ViewModels.Workspace.Services;

public interface IFileCodeService : IPropertyService
{
    /// <summary>
    /// Has indexing finished?
    /// </summary>
    public bool Indexed { get; }
    
    /// <summary>
    /// All indexed files
    /// </summary>
    public ObservableCollection<CodeFileViewModel> Files { get; }

    /// <summary>
    /// Get the mapping set for a file
    /// </summary>
    public CodeFileShaderMappingSetViewModel GetOrAddMappingSet(CodeFileViewModel codeFileViewModel);
    
    /// <summary>
    /// Instantiate a file and watch for changes
    /// </summary>
    public void InstantiateWithWatch(CodeFileViewModel codeFileViewModel);
}

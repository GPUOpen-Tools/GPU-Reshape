using AvaloniaEdit;
using Studio.ViewModels.Shader;

namespace Studio.ViewModels.Traits;

public interface IEditorExtension
{
    /// <summary>
    /// Install this extension
    /// </summary>
    public void InstallViewModel(IContentViewModel viewModel);
    
    /// <summary>
    /// Install this extension
    /// </summary>
    public void InstallView(IContentViewModel viewModel, TextEditor textEditor);
}

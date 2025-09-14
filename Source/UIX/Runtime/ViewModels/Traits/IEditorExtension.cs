using AvaloniaEdit;
using Studio.ViewModels.Shader;

namespace Studio.ViewModels.Traits;

public interface IEditorExtension
{
    /// <summary>
    /// Install this extension
    /// </summary>
    public void InstallViewModel(IShaderContentViewModel viewModel);
    
    /// <summary>
    /// Install this extension
    /// </summary>
    public void InstallView(IShaderContentViewModel viewModel, TextEditor textEditor);
}

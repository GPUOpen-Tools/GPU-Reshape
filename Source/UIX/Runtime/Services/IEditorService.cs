using AvaloniaEdit;
using DynamicData;
using ReactiveUI;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Traits;

namespace Studio.Services;

public interface IEditorService : IReactiveObject
{
    /// <summary>
    /// All extensions
    /// </summary>
    public ISourceList<IEditorExtension> Extensions { get; }

    /// <summary>
    /// Install extensions against an editor
    /// </summary>
    public void InstallViewModel(IContentViewModel viewModel);

    /// <summary>
    /// Install extensions against an editor
    /// </summary>
    public void InstallView(IContentViewModel viewModel, TextEditor textEditor);
}

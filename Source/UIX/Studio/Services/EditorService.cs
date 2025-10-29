using AvaloniaEdit;
using DynamicData;
using ReactiveUI;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Traits;

namespace Studio.Services;

public class EditorService : ReactiveObject, IEditorService
{
    /// <summary>
    /// All registered extensions
    /// </summary>
    public ISourceList<IEditorExtension> Extensions { get; } = new SourceList<IEditorExtension>();

    /// <summary>
    /// Install all extensions against an editor
    /// </summary>
    public void InstallViewModel(IContentViewModel viewModel)
    {
        foreach (IEditorExtension extension in Extensions.Items)
        {
            extension.InstallViewModel(viewModel);
        }
    }

    /// <summary>
    /// Install all extensions against an editor
    /// </summary>
    public void InstallView(IContentViewModel viewModel, TextEditor textEditor)
    {
        foreach (IEditorExtension extension in Extensions.Items)
        {
            extension.InstallView(viewModel, textEditor);
        }
    }
}

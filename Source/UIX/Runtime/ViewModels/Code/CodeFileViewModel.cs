using System.IO;
using ReactiveUI;
using Runtime.ViewModels.Traits;

namespace Studio.ViewModels.Code;

public class CodeFileViewModel : ReactiveObject, ISerializable
{
    /// <summary>
    /// Contents of this file
    /// </summary>
    public string Filename
    {
        get => _filename;
        set => this.RaiseAndSetIfChanged(ref _filename, value);
    }

    /// <summary>
    /// Contents of this file
    /// </summary>
    public string Contents
    {
        get => _contents;
        set => this.RaiseAndSetIfChanged(ref _contents, value);
    }

    /// <summary>
    /// Serialize this object
    /// </summary>
    public object Serialize()
    {
        return new SerializationMap()
        {
            { "Filename", Filename },
            { "Contents", Contents }
        };
    }

    /// <summary>
    /// Internal contents
    /// </summary>
    private string _contents = string.Empty;

    /// <summary>
    /// Internal filename
    /// </summary>
    private string _filename = string.Empty;
}

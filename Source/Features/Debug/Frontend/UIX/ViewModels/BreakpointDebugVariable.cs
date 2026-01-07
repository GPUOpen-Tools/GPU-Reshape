using ReactiveUI;
using Runtime.ViewModels.IL;
using Studio.Models.IL;

namespace GRS.Features.Debug.UIX.ViewModels;

public class BreakpointDebugVariable : ReactiveObject
{
    /// <summary>
    /// Type of this variable
    /// </summary>
    public Type Type
    {
        get => _type;
        set => this.RaiseAndSetIfChanged(ref _type, value);
    }
    
    /// <summary>
    /// Name of this variable
    /// </summary>
    public string Name
    {
        get => _name;
        set => this.RaiseAndSetIfChanged(ref _name, value);
    }
    
    /// <summary>
    /// Internal handle
    /// </summary>
    public uint Handle
    {
        get => _handle;
        set => this.RaiseAndSetIfChanged(ref _handle, value);
    }
    
    /// <summary>
    /// Decorated string
    /// </summary>
    public string Decoration
    {
        get
        {
            return $"{Name} - {Assembler.AssembleInlineType(_type, true)}";
        }
    }
    
    /// <summary>
    /// Internal type
    /// </summary>
    private Type _type;
    
    /// <summary>
    /// Internal name
    /// </summary>
    private string _name;
    
    /// <summary>
    /// Internal handle
    /// </summary>
    private uint _handle;
}
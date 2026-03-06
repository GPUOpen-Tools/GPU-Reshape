using ReactiveUI;
using Runtime.ViewModels.IL;
using Studio.Models.IL;

namespace GRS.Features.Debug.UIX.ViewModels;

public class BreakpointDebugValue : ReactiveObject
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
    public uint ValueId
    {
        get => _valueId;
        set => this.RaiseAndSetIfChanged(ref _valueId, value);
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
    /// All values
    /// </summary>
    public BreakpointDebugValue[] Values
    {
        get => _values;
        set => this.RaiseAndSetIfChanged(ref _values, value);
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
    private uint _valueId;

    /// <summary>
    /// Internal values
    /// </summary>
    private BreakpointDebugValue[] _values;
}

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
    /// Internal id
    /// </summary>
    public uint VariableId
    {
        get => _variableId;
        set => this.RaiseAndSetIfChanged(ref _variableId, value);
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
    /// Structural value
    /// </summary>
    public BreakpointDebugValue Value { get; set; } = new();
    
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
    private uint _variableId;
}

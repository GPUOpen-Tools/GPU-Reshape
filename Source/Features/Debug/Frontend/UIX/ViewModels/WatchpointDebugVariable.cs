using System;
using ReactiveUI;
using Runtime.ViewModels.IL;
using Type = Studio.Models.IL.Type;

namespace GRS.Features.Debug.UIX.ViewModels;

public class WatchpointDebugValue : ReactiveObject
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
    /// Variable handle
    /// </summary>
    public UInt64 VariableHash
    {
        get => _variableHash;
        set => this.RaiseAndSetIfChanged(ref _variableHash, value);
    }
    
    /// <summary>
    /// Value handle
    /// </summary>
    public uint ValueId
    {
        get => _valueId;
        set => this.RaiseAndSetIfChanged(ref _valueId, value);
    }
    
    /// <summary>
    /// Does this value have a valid reconstruction?
    /// </summary>
    public bool HasReconstruction
    {
        get => _hasReconstruction;
        set => this.RaiseAndSetIfChanged(ref _hasReconstruction, value);
    }
    
    /// <summary>
    /// Decorated string
    /// </summary>
    public string Decoration
    {
        get
        {
            string type = Assembler.AssembleInlineType(_type, true);
        
            if (string.IsNullOrEmpty(_name))
            {
                return type;
            }
            
            return $"{type} {Name}";
        }
    }

    /// <summary>
    /// All values
    /// </summary>
    public WatchpointDebugValue[] Values
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
    /// Internal handles
    /// </summary>
    private UInt64 _variableHash;
    private uint _valueId;

    /// <summary>
    /// Internal values
    /// </summary>
    private WatchpointDebugValue[] _values = Array.Empty<WatchpointDebugValue>();

    /// <summary>
    /// Internal reconstruction state
    /// </summary>
    private bool _hasReconstruction;
}

public class WatchpointDebugVariable : ReactiveObject
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
    public UInt64 VariableHash
    {
        get => _variableHash;
        set => this.RaiseAndSetIfChanged(ref _variableHash, value);
    }
    
    /// <summary>
    /// Decorated string
    /// </summary>
    public string Decoration
    {
        get
        {
            string type = Assembler.AssembleInlineType(_type, true);
        
            if (string.IsNullOrEmpty(_name))
            {
                return type;
            }
            
            return $"{type} {Name}";
        }
    }

    /// <summary>
    /// Structural value
    /// </summary>
    public WatchpointDebugValue Value { get; set; } = new();
    
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
    private UInt64 _variableHash;
}

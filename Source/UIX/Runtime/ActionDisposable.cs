using System;

namespace Studio;

public class ActionDisposable : IDisposable
{
    public ActionDisposable(Action action)
    {
        _action = action;
    }

    /// <summary>
    /// Invoked on disposes
    /// </summary>
    public void Dispose()
    {
        _action.Invoke();
    }
    
    /// <summary>
    /// Action to be invoked on disposes
    /// </summary>
    private readonly Action _action;
}
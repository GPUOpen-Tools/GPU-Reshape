using System;
using Avalonia.Controls;
using ReactiveUI;
using Splat;
using Studio.Views.Controls;

namespace Studio.Views
{
    public class UniformWindowLocator
    {
        public Window? ResolveWindow<T>(T viewModel)
        {
            if (viewModel == null)
            {
                return null;
            }
            
            // Get name
            string? typeName = viewModel.GetType().FullName;
            if (typeName == null)
            {
                return null;
            }
            
            // Naive type translation
            typeName = typeName.Replace(".ViewModels.", ".Views.");
            typeName = typeName.Replace("ViewModel", "Window");

            // Resolve window
            Type? viewType = Type.GetType(typeName);
            if (viewType == null)
            {
                return null;
            }
            
            // Create instance
            return Activator.CreateInstance(viewType) as Window ?? throw new TypeLoadException("Window must be derivable");
        }
    }
}
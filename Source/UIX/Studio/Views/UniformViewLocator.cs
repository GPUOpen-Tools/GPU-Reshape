using System;
using ReactiveUI;
using Splat;
using Studio.Views.Controls;

namespace Studio.Views
{
    public class UniformViewLocator : IViewLocator
    {
        public IViewFor? ResolveView<T>(T viewModel, string? contract = null)
        {
            if (viewModel == null)
            {
                return null;
            }
            
            // Get name
            string typeName = viewModel.GetType().FullName;
            
            // Replace all instances of view model with view (incredibly complicated view model to view association, almost frightening)
            string viewName = typeName.Replace("ViewModel", "View");

            // Resolve view
            var viewType = Type.GetType(viewName) ?? throw new TypeLoadException("Missing view for associated view model");
            
            // Create instance
            return Activator.CreateInstance(viewType) as IViewFor ?? throw new TypeLoadException("View must implement IViewFor");
        }
    }
}
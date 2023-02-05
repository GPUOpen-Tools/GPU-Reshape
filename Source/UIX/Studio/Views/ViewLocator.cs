using System;
using Avalonia;
using ReactiveUI;
using Splat;
using Studio.Services;
using Studio.Views.Controls;

namespace Studio.Views
{
    public class ViewLocator : IViewLocator
    {
        public IViewFor? DefaultView;
        
        public IViewFor? ResolveView<T>(T viewModel, string? contract = null)
        {
            if (viewModel == null)
            {
                return null;
            }
            
            // Attempt to instantiate
            IViewFor? view = App.Locator.GetService<ILocatorService>()?.InstantiateDerived<IViewFor>(viewModel);
            if (view == null)
            {
                return null;
            }

            // OK
            view.ViewModel = viewModel;
            return view;
        }
    }
}
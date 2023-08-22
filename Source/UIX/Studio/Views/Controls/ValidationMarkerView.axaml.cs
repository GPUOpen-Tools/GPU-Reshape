using System;
using Avalonia.Controls;
using ReactiveUI;
using Studio.Extensions;
using Studio.ViewModels.Controls;

namespace Studio.Views.Controls
{
    public partial class ValidationMarkerView : UserControl
    {
        /// <summary>
        /// View model helper
        /// </summary>
        public ValidationMarkerViewModel ViewModel => (ValidationMarkerViewModel)DataContext!;
        
        public ValidationMarkerView()
        {
            InitializeComponent();

            this.WhenAnyValue(x => x.DataContext)
                .CastNullable<ValidationMarkerViewModel>()
                .Subscribe(x =>
                {
                    // Bind detail click
                    ValidationButton.Events().Click.Subscribe(_ =>
                    {
                        x.DetailCommand?.Execute(x.SelectedObject);
                    });
                    
                    // Hide flyout on selection
                    x.WhenAnyValue(y => y.SelectedObject).Subscribe(_ =>
                    {
                        DropdownButton.Flyout.Hide();
                    });
                });
        }
    }
}
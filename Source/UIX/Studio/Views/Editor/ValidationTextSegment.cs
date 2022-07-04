using AvaloniaEdit.Document;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.Views.Editor
{
    public class ValidationTextSegment : TextSegment
    {
        public ValidationObject? Object { get; set; }
    }
}
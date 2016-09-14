using System.ComponentModel.Composition;
using System.Windows.Media;
using Microsoft.VisualStudio.Text.Classification;
using Microsoft.VisualStudio.Utilities;

namespace EpochVS
{
    [Export(typeof(EditorFormatDefinition))]
    [ClassificationType(ClassificationTypeNames = "EpochComment")]
    [Name("EpochComment")]
    [UserVisible(true)] // This should be visible to the end user
    [Order(Before = Priority.Default)] // Set the priority to be after the default classifiers
    internal sealed class EpochCommentFormat : ClassificationFormatDefinition
    {
        public EpochCommentFormat()
        {
            this.DisplayName = "Epoch Comment";
            this.ForegroundColor = Colors.Green;
        }
    }
}

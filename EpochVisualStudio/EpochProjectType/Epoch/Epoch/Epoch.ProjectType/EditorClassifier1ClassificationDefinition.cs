//------------------------------------------------------------------------------
// <copyright file="EditorClassifier1ClassificationDefinition.cs" company="Company">
//     Copyright (c) Company.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System.ComponentModel.Composition;
using Microsoft.VisualStudio.Text.Classification;
using Microsoft.VisualStudio.Utilities;

namespace EpochVS
{
    /// <summary>
    /// Classification type definition export for EditorClassifier1
    /// </summary>
    internal static class EditorClassifier1ClassificationDefinition
    {
        // This disables "The field is never used" compiler's warning. Justification: the field is used by MEF.
#pragma warning disable 169

        [Export(typeof(ClassificationTypeDefinition))]
        [Name("EpochComment")]
        private static ClassificationTypeDefinition typeDefinition;

#pragma warning restore 169
    }
}

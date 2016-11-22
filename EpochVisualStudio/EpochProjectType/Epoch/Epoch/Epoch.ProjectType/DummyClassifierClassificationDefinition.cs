//------------------------------------------------------------------------------
// <copyright file="DummyClassifierClassificationDefinition.cs" company="Company">
//     Copyright (c) Company.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System.ComponentModel.Composition;
using Microsoft.VisualStudio.Text.Classification;
using Microsoft.VisualStudio.Utilities;

namespace EpochVS
{
    /// <summary>
    /// Classification type definition export for DummyClassifier
    /// </summary>
    internal static class DummyClassifierClassificationDefinition
    {
        // This disables "The field is never used" compiler's warning. Justification: the field is used by MEF.
#pragma warning disable 169

        /// <summary>
        /// Defines the "DummyClassifier" classification type.
        /// </summary>
        [Export(typeof(ClassificationTypeDefinition))]
        [Name("DummyClassifier")]
        private static ClassificationTypeDefinition typeDefinition;

#pragma warning restore 169
    }
}

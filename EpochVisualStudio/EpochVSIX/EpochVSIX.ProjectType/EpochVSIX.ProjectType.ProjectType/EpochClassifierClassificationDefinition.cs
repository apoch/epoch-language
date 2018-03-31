//------------------------------------------------------------------------------
// <copyright file="EpochClassifierClassificationDefinition.cs" company="Company">
//     Copyright (c) Company.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System.ComponentModel.Composition;
using Microsoft.VisualStudio.Text.Classification;
using Microsoft.VisualStudio.Utilities;

namespace EpochVSIX
{
    /// <summary>
    /// Classification type definition export for EpochClassifier
    /// </summary>
    internal static class EpochClassifierClassificationDefinition
    {
        // This disables "The field is never used" compiler's warning. Justification: the field is used by MEF.
#pragma warning disable 169

        /// <summary>
        /// Defines the "EpochClassifier" classification type.
        /// </summary>
        [Export(typeof(ClassificationTypeDefinition))]
        [Name("EpochComment")]
        private static ClassificationTypeDefinition typeDefinition;

#pragma warning restore 169
    }
}

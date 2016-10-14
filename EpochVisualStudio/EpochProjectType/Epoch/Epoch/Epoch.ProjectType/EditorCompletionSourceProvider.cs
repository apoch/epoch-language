using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.ComponentModel.Composition;
using Microsoft.VisualStudio.Language.Intellisense;
using Microsoft.VisualStudio.Text;
using Microsoft.VisualStudio.Text.Operations;
using Microsoft.VisualStudio.Utilities;

namespace EpochVS
{
    [Export(typeof(ICompletionSourceProvider))]
    [ContentType("EpochFile")]
    [Name("Epoch name completion")]
    class EditorCompletionSourceProvider : ICompletionSourceProvider
    {
        [Import]
        internal ITextStructureNavigatorSelectorService NavigatorService { get; set; }

        [Import]
        internal IGlyphService GlyphService = null;


        public ICompletionSource TryCreateCompletionSource(ITextBuffer textBuffer)
        {
            return new EditorCompletionSource(this, textBuffer, GlyphService);
        }
    }


    [Export(typeof(ISignatureHelpSourceProvider))]
    [Name("Epoch function signature help source")]
    [Order(Before = "default")]
    [ContentType("EpochFile")]
    internal class EditorSignatureHelpSourceProvider : ISignatureHelpSourceProvider
    {
        [Import]
        internal ITextStructureNavigatorSelectorService NavigatorService { get; set; }

        public ISignatureHelpSource TryCreateSignatureHelpSource(ITextBuffer textBuffer)
        {
            return new EditorSignatureHelpSource(textBuffer, NavigatorService);
        }
    }
}

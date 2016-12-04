using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.ComponentModel.Composition;
using Microsoft.VisualStudio.Language.Intellisense;
using Microsoft.VisualStudio.Text;
using Microsoft.VisualStudio.Text.Operations;
using Microsoft.VisualStudio.Utilities;

namespace EpochVSIX
{
    internal class EpochCompletionSource : ICompletionSource
    {
        private EpochCompletionSourceProvider m_sourceProvider;
        private ITextBuffer m_textBuffer;
        private List<Completion> m_completionList;
        private IGlyphService m_glyphService;
        private ProjectParser Parser;

        public EpochCompletionSource(EpochCompletionSourceProvider sourceProvider, ITextBuffer textBuffer, IGlyphService glyphService)
        {
            m_sourceProvider = sourceProvider;
            m_textBuffer = textBuffer;
            m_glyphService = glyphService;

            Parser = ProjectParser.GetInstance();
        }

        void ICompletionSource.AugmentCompletionSession(ICompletionSession session, IList<CompletionSet> completionSets)
        {
            m_completionList = new List<Completion>();

            List<string> strList = new List<string>();
            strList.Add("print");
            strList.Add("assert");

            //ProjectParser.ParseTextBuffer(m_textBuffer);
            Parser.GetAvailableFunctionNames(strList);

            var funcglyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupMethod, StandardGlyphItem.GlyphItemPublic);
            foreach (string str in strList)
                m_completionList.Add(new Completion(str, str, str, funcglyph, null));


            List<string> structureNames = new List<string>();
            Parser.GetAvailableStructureNames(structureNames);

            var structglyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupType, StandardGlyphItem.GlyphItemPublic);
            foreach (string str in structureNames)
                m_completionList.Add(new Completion(str, str, str, structglyph, null));

            m_completionList.Sort((a, b) => { return a.DisplayText.CompareTo(b.DisplayText); });


            // TODO - cleanup
            completionSets.Add(new CompletionSet(
                "Epoch",    //the non-localized title of the tab
                "Epoch",    //the display title of the tab
                FindTokenSpanAtPosition(session.GetTriggerPoint(m_textBuffer),
                    session),
                m_completionList,
                null));
        }

        private ITrackingSpan FindTokenSpanAtPosition(ITrackingPoint point, ICompletionSession session)
        {
            SnapshotPoint currentPoint = (session.TextView.Caret.Position.BufferPosition) - 1;
            ITextStructureNavigator navigator = m_sourceProvider.NavigatorService.GetTextStructureNavigator(m_textBuffer);
            TextExtent extent = navigator.GetExtentOfWord(currentPoint);
            return currentPoint.Snapshot.CreateTrackingSpan(extent.Span, SpanTrackingMode.EdgeInclusive);
        }

        private bool m_isDisposed;
        public void Dispose()
        {
            if (!m_isDisposed)
            {
                GC.SuppressFinalize(this);
                m_isDisposed = true;
            }
        }
    }
}

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

            Parser.GetAvailableFunctionNames(strList);

            var funcglyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupMethod, StandardGlyphItem.GlyphItemPublic);
            foreach (string str in strList)
                m_completionList.Add(new Completion(str, str, str, funcglyph, null));


            List<string> structureNames = new List<string>();
            Parser.GetAvailableStructureNames(structureNames);

            var structglyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupType, StandardGlyphItem.GlyphItemPublic);
            foreach (string str in structureNames)
                m_completionList.Add(new Completion(str, str, str, structglyph, null));


            List<string> udtList = new List<string>();
            Parser.GetAvailableTypeNames(udtList);

            var udtglyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupTypedef, StandardGlyphItem.GlyphItemPublic);
            foreach (string str in udtList)
                m_completionList.Add(new Completion(str, str, str, udtglyph, null));


            List<string> typeList = new List<string>();
            typeList.Add("boolean");
            typeList.Add("buffer");
            typeList.Add("integer");
            typeList.Add("integer16");
            typeList.Add("integer64");
            typeList.Add("real");
            typeList.Add("string");

            var typeglyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupIntrinsic, StandardGlyphItem.GlyphItemPublic);
            foreach (string str in typeList)
                m_completionList.Add(new Completion(str, str, str, typeglyph, null));


            var keywordList = new List<string>();
            keywordList.Add("ref");
            keywordList.Add("else");
            keywordList.Add("elseif");
            keywordList.Add("if");
            keywordList.Add("structure");
            keywordList.Add("type");
            keywordList.Add("while");

            var keywordglyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupUnknown, StandardGlyphItem.GlyphItemPublic);
            foreach (string str in keywordList)
                m_completionList.Add(new Completion(str, str, str, keywordglyph, null));


            var valueList = new List<string>();
            valueList.Add("true");
            valueList.Add("false");
            valueList.Add("nothing");

            var valueglyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupValueType, StandardGlyphItem.GlyphItemPublic);
            foreach (string str in valueList)
                m_completionList.Add(new Completion(str, str, str, valueglyph, null));


            var varlist = new List<string>();
            Parser.GetAvailableVariables(varlist, null, 0, 0);      // TODO - actual document and location

            var varglyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupVariable, StandardGlyphItem.GlyphItemPublic);
            foreach (string str in varlist)
                m_completionList.Add(new Completion(str, str, str, varglyph, null));


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

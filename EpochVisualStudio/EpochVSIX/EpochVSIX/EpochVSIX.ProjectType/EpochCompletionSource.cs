using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.ComponentModel.Composition;
using Microsoft.VisualStudio.Language.Intellisense;
using Microsoft.VisualStudio.Text;
using Microsoft.VisualStudio.Text.Operations;
using Microsoft.VisualStudio.Utilities;
using Microsoft.VisualStudio.TextManager.Interop;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio.Shell;

namespace EpochVSIX
{
    internal class EpochCompletionSource : ICompletionSource
    {
        private EpochCompletionSourceProvider m_sourceProvider;
        private ITextBuffer m_textBuffer;
        private List<Completion> m_completionList;
        private IGlyphService m_glyphService;
        private Parser.Project m_parsedProject;

        public EpochCompletionSource(EpochCompletionSourceProvider sourceProvider, ITextBuffer textBuffer, IGlyphService glyphService)
        {
            m_sourceProvider = sourceProvider;
            m_textBuffer = textBuffer;
            m_glyphService = glyphService;

            m_parsedProject = textBuffer.Properties.GetProperty<Parser.Project>(typeof(Parser.Project));
        }

        void ICompletionSource.AugmentCompletionSession(ICompletionSession session, IList<CompletionSet> completionSets)
        {
            ITrackingSpan applicableTo = null;
            m_completionList = new List<Completion>();

            var bufferpos = session.TextView.Caret.Position.BufferPosition.Subtract(1);
            if (bufferpos.GetChar().Equals('.'))
            {
                applicableTo = FindTokenSpanAfterPosition(session);

                var memberglyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupStruct, StandardGlyphItem.GlyphItemPublic);
                var memfunglyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupMethod, StandardGlyphItem.GlyphItemPublic);

                List<string> varstack = GetMemberAccessStack(bufferpos);

                var variables = GetApplicableVariables(session);

                if (varstack != null)
                {
                    foreach (var v in variables)
                    {
                        if (v.Name.Text.Equals(varstack[0]))
                        {
                            var defn = m_parsedProject.GetStructureDefinition(v.Type.Name);
                            if (defn == null)
                                continue;

                            for (int i = 1; i < varstack.Count; ++i)
                            {
                                if (defn == null)
                                    break;

                                foreach (var m in defn.Members)
                                {
                                    if (m.Name.Equals(varstack[i]))
                                    {
                                        defn = m_parsedProject.GetStructureDefinition(m.Type.Name);
                                        break;
                                    }
                                }

                                defn = null;
                            }

                            if (defn != null)
                            {
                                foreach (var m in defn.Members)
                                    m_completionList.Add(new Completion(m.Name.Text, m.Name.Text, $"{m.Type.Name} {m.Name.Text}", memberglyph, null));      //  TODO - support member functions
                            }
                        }
                    }
                }
            }
            else
            {
                applicableTo = FindTokenSpanAtPosition(session.GetTriggerPoint(m_textBuffer), session);

                var funcList = m_parsedProject.GetAvailableFunctionSignatures();

                var funcglyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupMethod, StandardGlyphItem.GlyphItemPublic);
                foreach (var sig in funcList)
                {
                    m_completionList.Add(new Completion(sig.Name.Text, sig.Name.Text, string.Join("\r\n\r\n", sig.Overloads.Select(o => o.Format(sig.Name.Text)).ToArray()), funcglyph, null));
                }


                var structuredefs = m_parsedProject.GetAvailableStructureDefinitions();

                var structglyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupType, StandardGlyphItem.GlyphItemPublic);
                foreach (var strdef in structuredefs)
                    m_completionList.Add(new Completion(strdef.Key, strdef.Key, strdef.Value.ToString(), structglyph, null));


                var udtList = m_parsedProject.GetAvailableTypeNames();

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

                var keywordglyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphKeyword, StandardGlyphItem.GlyphItemPublic);
                foreach (string str in keywordList)
                    m_completionList.Add(new Completion(str, str, str, keywordglyph, null));


                var valueList = new List<string>();
                valueList.Add("true");
                valueList.Add("false");
                valueList.Add("nothing");

                var valueglyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupValueType, StandardGlyphItem.GlyphItemPublic);
                foreach (string str in valueList)
                    m_completionList.Add(new Completion(str, str, str, valueglyph, null));


                var varlist = GetApplicableVariables(session);

                var varglyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupVariable, StandardGlyphItem.GlyphItemPublic);
                foreach (var v in varlist)
                    m_completionList.Add(new Completion(v.Name.Text, v.Name.Text, v.Type.Name + " " + v.Name.Text, varglyph, null));

                m_completionList.Sort((a, b) => { return a.DisplayText.CompareTo(b.DisplayText); });
            }

            if (m_completionList.Count > 0)
                completionSets.Add(new CompletionSet("Epoch", "Epoch", applicableTo, m_completionList, null));
        }

        private ITrackingSpan FindTokenSpanAtPosition(ITrackingPoint point, ICompletionSession session)
        {
            SnapshotPoint currentPoint = (session.TextView.Caret.Position.BufferPosition) - 1;
            ITextStructureNavigator navigator = m_sourceProvider.NavigatorService.GetTextStructureNavigator(m_textBuffer);
            TextExtent extent = navigator.GetExtentOfWord(currentPoint);
            return currentPoint.Snapshot.CreateTrackingSpan(extent.Span, SpanTrackingMode.EdgeInclusive);
        }

        private ITrackingSpan FindTokenSpanAfterPosition(ICompletionSession session)
        {
            SnapshotPoint currentPoint = (session.TextView.Caret.Position.BufferPosition);
            return currentPoint.Snapshot.CreateTrackingSpan(currentPoint.Position, 0, SpanTrackingMode.EdgeInclusive);
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


        private List<Parser.Variable> GetApplicableVariables(ICompletionSession session)
        {
            var pt = session.TextView.Caret.Position.Point.GetPoint(session.TextView.TextSnapshot, PositionAffinity.Predecessor);
            if (!pt.HasValue)
                return new List<Parser.Variable>();

            return m_parsedProject.GetAvailableVariables(m_textBuffer, pt.Value);
        }

        private List<string> GetMemberAccessStack(SnapshotPoint bufferpos)
        {
            ITextStructureNavigator navigator = m_sourceProvider.NavigatorService.GetTextStructureNavigator(m_textBuffer);

            SnapshotPoint currentPoint = bufferpos - 1;
            TextExtent extent = navigator.GetExtentOfWord(currentPoint);

            var ret = new List<string>();

            SnapshotPoint dotpos;
            do
            {
                ret.Insert(0, extent.Span.GetText());
                dotpos = extent.Span.Start - 1;

                var prevtokenpos = dotpos - 1;
                extent = navigator.GetExtentOfWord(prevtokenpos);
            } while (dotpos.GetChar().Equals('.'));

            return ret;
        }
    }
}

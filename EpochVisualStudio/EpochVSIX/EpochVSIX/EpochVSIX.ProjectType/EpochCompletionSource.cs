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
            ITrackingSpan applicableTo = null;
            m_completionList = new List<Completion>();

            var bufferpos = session.TextView.Caret.Position.BufferPosition.Subtract(1);
            if (bufferpos.GetChar().Equals('.'))
            {
                applicableTo = FindTokenSpanAfterPosition(session);

                var memberglyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupStruct, StandardGlyphItem.GlyphItemPublic);
                var memfunglyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupMethod, StandardGlyphItem.GlyphItemPublic);

                SnapshotPoint currentPoint = bufferpos - 1;
                ITextStructureNavigator navigator = m_sourceProvider.NavigatorService.GetTextStructureNavigator(m_textBuffer);
                TextExtent extent = navigator.GetExtentOfWord(currentPoint);

                string varname = extent.Span.GetText();

                var parser = ProjectParser.GetInstance();
                var variables = new List<ProjectParser.Variable>();

                var pt = session.TextView.Caret.Position.Point.GetPoint(session.TextView.TextSnapshot, PositionAffinity.Predecessor);
                if (pt.HasValue)
                {
                    IVsTextBuffer bufferAdapter;
                    m_textBuffer.Properties.TryGetProperty(typeof(Microsoft.VisualStudio.TextManager.Interop.IVsTextBuffer), out bufferAdapter);

                    if (bufferAdapter != null)
                    {
                        ThreadHelper.ThrowIfNotOnUIThread();
                        var persistFileFormat = bufferAdapter as IPersistFileFormat;
                        string ppzsFilename = null;
                        uint iii;
                        if (persistFileFormat != null)
                        {
                            persistFileFormat.GetCurFile(out ppzsFilename, out iii);

                            if (!string.IsNullOrEmpty(ppzsFilename))
                            {
                                var line = pt.Value.GetContainingLine().LineNumber;
                                var column = pt.Value.Position - pt.Value.GetContainingLine().Start.Position;
                                parser.GetAvailableVariables(variables, ppzsFilename, line, column);

                                foreach (var v in variables)
                                {
                                    if (v.Name.Equals(varname))
                                    {
                                        var defn = parser.GetStructureDefinition(v.Type);
                                        if (defn != null)
                                        {
                                            foreach (var m in defn.Members)
                                                m_completionList.Add(new Completion(m.Name, m.Name, $"{m.Type} {m.Name}", memberglyph, null));
                                        }
                                    }
                                }
                            }

                            // TODO - do something sane if the file is unsaved (and other error cases)
                        }
                    }
                }
            }
            else
            {
                applicableTo = FindTokenSpanAtPosition(session.GetTriggerPoint(m_textBuffer), session);

                var funcList = new List<ProjectParser.FunctionDefinition>();
                Parser.GetAvailableFunctionSignatures(funcList);

                var funcglyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupMethod, StandardGlyphItem.GlyphItemPublic);
                foreach (var sig in funcList)
                    m_completionList.Add(new Completion(sig.FunctionName, sig.FunctionName, sig.ToString(), funcglyph, null));


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


                var varlist = new List<ProjectParser.Variable>();
                Parser.GetAvailableVariables(varlist, null, 0, 0);      // TODO - actual document and location

                var varglyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupVariable, StandardGlyphItem.GlyphItemPublic);
                foreach (var v in varlist)
                    m_completionList.Add(new Completion(v.Name, v.Name, v.Type + " " + v.Name, varglyph, null));

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
    }
}

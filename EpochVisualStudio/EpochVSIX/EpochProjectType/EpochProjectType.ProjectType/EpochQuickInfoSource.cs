using Microsoft.VisualStudio.Language.Intellisense;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.VisualStudio.Text;
using Microsoft.VisualStudio.Text.Editor;
using Microsoft.VisualStudio.Text.Operations;
using Microsoft.VisualStudio.Text.Tagging;
using Microsoft.VisualStudio.Utilities;
using System.ComponentModel.Composition;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio.TextManager.Interop;
using Microsoft.VisualStudio.Editor;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Text.Classification;

namespace EpochVSIX
{
    class EpochQuickInfoSource : IQuickInfoSource
    {
        private enum TypeOfContent
        {
            Variable,
            Function,
            Structure,
        }

        private class Content
        {
            public string Text;
            public TypeOfContent Type;
        }

        private bool m_isDisposed;
        private EpochQuickInfoSourceProvider m_provider;
        private ITextBuffer m_subjectBuffer;
        private IVsDebugger m_debugger;
        private IVsEditorAdaptersFactoryService m_adapter;
        private Parser.Project m_parsedProject;
        private ITextBufferFactoryService m_bufferFactory;
        private IContentTypeRegistryService m_contentTypes;
        private IClassifierProvider m_classifierProvider;
        private IGlyphService m_glyphService;

        public EpochQuickInfoSource(EpochQuickInfoSourceProvider provider, ITextBuffer subjectBuffer, IVsDebugger debugger, IVsEditorAdaptersFactoryService adapter, ITextBufferFactoryService bufferfactory, IContentTypeRegistryService contentTypes, IClassifierProvider classifierprovider, IGlyphService glyphService)
        {
            m_provider = provider;
            m_subjectBuffer = subjectBuffer;
            m_debugger = debugger;
            m_adapter = adapter;
            m_parsedProject = subjectBuffer.Properties.GetProperty<Parser.Project>(typeof(Parser.Project));
            m_bufferFactory = bufferfactory;
            m_contentTypes = contentTypes;
            m_classifierProvider = classifierprovider;
            m_glyphService = glyphService;
        }

        public void AugmentQuickInfoSession(IQuickInfoSession session, IList<object> qiContent, out ITrackingSpan applicableToSpan)
        {
            // Map the trigger point down to our buffer.
            SnapshotPoint? subjectTriggerPoint = session.GetTriggerPoint(m_subjectBuffer.CurrentSnapshot);
            if (!subjectTriggerPoint.HasValue)
            {
                applicableToSpan = null;
                return;
            }

            var dict = new Dictionary<string, Content>();

            var funclist = m_parsedProject.GetAvailableFunctionSignatures();
            foreach (var func in funclist)
            {
                dict.Add(func.Name.Text, new Content { Text = func.ToString(), Type = TypeOfContent.Function });
            }

            var structlist = m_parsedProject.GetAvailableStructureDefinitions();
            foreach (var st in structlist)
            {
                dict.Add(st.Key, new Content { Text = st.Value.ToString(), Type = TypeOfContent.Structure });
            }

            var variables = GetApplicableVariables(session);
            foreach (var v in variables)
                dict.Add(v.Name.Text, new Content { Text = v.ToString(), Type = TypeOfContent.Variable });

            ITextSnapshot currentSnapshot = subjectTriggerPoint.Value.Snapshot;
            SnapshotSpan querySpan = new SnapshotSpan(subjectTriggerPoint.Value, 0);

            //look for occurrences of our QuickInfo words in the span
            ITextStructureNavigator navigator = m_provider.NavigatorService.GetTextStructureNavigator(m_subjectBuffer);
            TextExtent extent = navigator.GetExtentOfWord(subjectTriggerPoint.Value);
            string searchText = extent.Span.GetText();

            foreach (string key in dict.Keys)
            {
                if (key.CompareTo(searchText) == 0)
                {
                    applicableToSpan = currentSnapshot.CreateTrackingSpan(extent.Span.Start, key.Length, SpanTrackingMode.EdgeInclusive);

                    Content value;
                    dict.TryGetValue(key, out value);
                    if (value != null)
                        qiContent.Add(value.Text);// ConstructHighlightableContent(value.Text, value.Type));

                    return;
                }
            }

            applicableToSpan = null;
        }

        public void Dispose()
        {
            if (!m_isDisposed)
            {
                GC.SuppressFinalize(this);
                m_isDisposed = true;
            }
        }


        private List<Parser.Variable> GetApplicableVariables(IQuickInfoSession session)
        {
            var pt = session.GetTriggerPoint(session.TextView.TextBuffer).GetPoint(session.TextView.TextSnapshot);
            return m_parsedProject.GetAvailableVariables(m_subjectBuffer, pt);
        }

        private EpochQuickInfoContent ConstructHighlightableContent(string text, TypeOfContent typeOfContent)
        {
            var content = new EpochQuickInfoContent();
            content.Lines = new List<List<ClassificationSpan>>();

            if (typeOfContent == TypeOfContent.Variable)
                content.Glyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupVariable, StandardGlyphItem.GlyphItemPublic);
            else if (typeOfContent == TypeOfContent.Function)
                content.Glyph = content.SubGlyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupMethod, StandardGlyphItem.GlyphItemPublic);
            else if (typeOfContent == TypeOfContent.Structure)
            {
                content.Glyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupType, StandardGlyphItem.GlyphItemPublic);
                content.SubGlyph = m_glyphService.GetGlyph(StandardGlyphGroup.GlyphGroupVariable, StandardGlyphItem.GlyphItemPublic);
            }

            var contentType = m_contentTypes.GetContentType("EpochFile");
            var buffer = m_bufferFactory.CreateTextBuffer(text, contentType);
            var snapshot = buffer.CurrentSnapshot;

            var classifier = m_classifierProvider.GetClassifier(buffer) as EpochClassifier;
            classifier.ParsedProject = m_subjectBuffer.Properties.GetProperty<Parser.Project>(typeof(Parser.Project));
            var spans = classifier.GetAllSpans(new SnapshotSpan(snapshot, 0, text.Length), true);

            var lines = text.Count(x => x == '\n') + 1;
            for(int i = 0; i < lines; ++i)
            {
                var line = new List<ClassificationSpan>();
                foreach (var span in spans)
                {
                    if (span.Span.Start.GetContainingLine().LineNumber == i)
                        line.Add(span);
                }

                content.Lines.Add(line);
            }

            return content;
        }
    }


    [Export(typeof(IQuickInfoSourceProvider))]
    [Name("Epoch ToolTip QuickInfo Source")]
    [Order(Before = "Default Quick Info Presenter")]
    [ContentType("EpochFile")]
    internal class EpochQuickInfoSourceProvider : IQuickInfoSourceProvider
    {
        [Import]
        internal ITextStructureNavigatorSelectorService NavigatorService { get; set; }

        [Import]
        internal ITextBufferFactoryService TextBufferFactoryService { get; set; }

        [Import]
        internal SVsServiceProvider ServiceProvider { get; set; }

        [Import]
        internal IVsEditorAdaptersFactoryService AdapterService = null;

        [Import]
        internal IContentTypeRegistryService ContentTypeRegistry { get; set; }

        [Import]
        internal IGlyphService GlyphService = null;

        public IQuickInfoSource TryCreateQuickInfoSource(ITextBuffer textBuffer)
        {
            ThreadHelper.ThrowIfNotOnUIThread();
            var ClassifierProvider = EpochClassifierProvider.Instance;
            var debugger = ServiceProvider.GetService(typeof(IVsDebugger)) as IVsDebugger;
            return new EpochQuickInfoSource(this, textBuffer, debugger, AdapterService, TextBufferFactoryService, ContentTypeRegistry, ClassifierProvider, GlyphService);
        }
    }


    internal class EpochQuickInfoController : IIntellisenseController
    {
        private ITextView m_textView;
        private IList<ITextBuffer> m_subjectBuffers;
        private EpochQuickInfoControllerProvider m_provider;
        private IQuickInfoSession m_session;

        internal EpochQuickInfoController(ITextView textView, IList<ITextBuffer> subjectBuffers, EpochQuickInfoControllerProvider provider)
        {
            m_textView = textView;
            m_subjectBuffers = subjectBuffers;
            m_provider = provider;

            m_textView.MouseHover += this.OnTextViewMouseHover;
        }

        private void OnTextViewMouseHover(object sender, MouseHoverEventArgs e)
        {
            //find the mouse position by mapping down to the subject buffer
            SnapshotPoint? point = m_textView.BufferGraph.MapDownToFirstMatch
                 (new SnapshotPoint(m_textView.TextSnapshot, e.Position),
                PointTrackingMode.Positive,
                snapshot => m_subjectBuffers.Contains(snapshot.TextBuffer),
                PositionAffinity.Predecessor);

            if (point != null)
            {
                ITrackingPoint triggerPoint = point.Value.Snapshot.CreateTrackingPoint(point.Value.Position,
                PointTrackingMode.Positive);

                if (!m_provider.QuickInfoBroker.IsQuickInfoActive(m_textView))
                {
                    m_session = m_provider.QuickInfoBroker.TriggerQuickInfo(m_textView, triggerPoint, true);
                }
            }
        }

        public void Detach(ITextView textView)
        {
            if (m_textView == textView)
            {
                m_textView.MouseHover -= this.OnTextViewMouseHover;
                m_textView = null;
            }
        }

        public void ConnectSubjectBuffer(ITextBuffer subjectBuffer)
        {
        }

        public void DisconnectSubjectBuffer(ITextBuffer subjectBuffer)
        {
        }
    }


    [Export(typeof(IIntellisenseControllerProvider))]
    [Name("Epoch ToolTip QuickInfo Controller")]
    [ContentType("EpochFile")]
    internal class EpochQuickInfoControllerProvider : IIntellisenseControllerProvider
    {
        [Import]
        internal IQuickInfoBroker QuickInfoBroker { get; set; }

        public IIntellisenseController TryCreateIntellisenseController(ITextView textView, IList<ITextBuffer> subjectBuffers)
        {
            return new EpochQuickInfoController(textView, subjectBuffers, this);
        }
    }
}

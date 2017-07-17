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

namespace EpochVSIX
{
    class EpochQuickInfoSource : IQuickInfoSource
    {
        private bool m_isDisposed;
        private EpochQuickInfoSourceProvider m_provider;
        private ITextBuffer m_subjectBuffer;
        private Dictionary<string, string> m_dictionary;
        private IVsDebugger m_debugger;
        private IVsEditorAdaptersFactoryService m_adapter;

        public EpochQuickInfoSource(EpochQuickInfoSourceProvider provider, ITextBuffer subjectBuffer, IVsDebugger debugger, IVsEditorAdaptersFactoryService adapter)
        {
            m_provider = provider;
            m_subjectBuffer = subjectBuffer;
            m_debugger = debugger;
            m_adapter = adapter;
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

            var funclist = new List<ProjectParser.FunctionDefinition>();
            ProjectParser.GetInstance().GetAvailableFunctionSignatures(funclist);

            m_dictionary = new Dictionary<string, string>();
            foreach (var func in funclist)
                m_dictionary.Add(func.FunctionName, func.ToString());


            ITextSnapshot currentSnapshot = subjectTriggerPoint.Value.Snapshot;
            SnapshotSpan querySpan = new SnapshotSpan(subjectTriggerPoint.Value, 0);

            //look for occurrences of our QuickInfo words in the span
            ITextStructureNavigator navigator = m_provider.NavigatorService.GetTextStructureNavigator(m_subjectBuffer);
            TextExtent extent = navigator.GetExtentOfWord(subjectTriggerPoint.Value);
            string searchText = extent.Span.GetText();

            foreach (string key in m_dictionary.Keys)
            {
                if (key.CompareTo(searchText) == 0)
                {
                    applicableToSpan = currentSnapshot.CreateTrackingSpan(extent.Span.Start, key.Length, SpanTrackingMode.EdgeInclusive);

                    string value;
                    m_dictionary.TryGetValue(key, out value);
                    if (value != null)
                        qiContent.Add(value);
                    else
                        qiContent.Add("");

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


        public IQuickInfoSource TryCreateQuickInfoSource(ITextBuffer textBuffer)
        {
            var debugger = ServiceProvider.GetService(typeof(IVsDebugger)) as IVsDebugger;
            return new EpochQuickInfoSource(this, textBuffer, debugger, AdapterService);
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

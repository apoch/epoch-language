using System;
using System.ComponentModel.Composition;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Editor;
using Microsoft.VisualStudio.Language.Intellisense;
using Microsoft.VisualStudio.OLE.Interop;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Text;
using Microsoft.VisualStudio.Text.Editor;
using Microsoft.VisualStudio.TextManager.Interop;
using Microsoft.VisualStudio.Utilities;
using Microsoft.VisualStudio.Text.Operations;

namespace EpochVSIX
{
    [Export(typeof(IVsTextViewCreationListener))]
    [Name("Epoch syntax completion handler")]
    [ContentType("EpochFile")]
    [TextViewRole(PredefinedTextViewRoles.Editable)]
    internal class EpochCompletionHandlerProvider : IVsTextViewCreationListener
    {
        [Import]
        internal IVsEditorAdaptersFactoryService AdapterService = null;
        [Import]
        internal ICompletionBroker CompletionBroker { get; set; }
        [Import]
        internal SVsServiceProvider ServiceProvider { get; set; }
        [Import]
        internal ITextStructureNavigatorSelectorService NavigatorService { get; set; }
        [Import]
        internal ISignatureHelpBroker SignatureHelpBroker { get; set; }

        public void VsTextViewCreated(IVsTextView textViewAdapter)
        {
            ITextView textView = AdapterService.GetWpfTextView(textViewAdapter);
            if (textView == null)
                return;

            Func<EpochCompletionCommandHandler> createCommandHandler = delegate ()
            {
                return new EpochCompletionCommandHandler(textViewAdapter, textView, this, SignatureHelpBroker, NavigatorService.GetTextStructureNavigator(textView.TextBuffer));
            };

            textView.Properties.GetOrCreateSingletonProperty(createCommandHandler);

            ProjectParser.GetInstance().ParseProject(ServiceProvider.GetService(typeof(EnvDTE.DTE)) as EnvDTE.DTE);
        }
    }

    internal class EpochCompletionCommandHandler : IOleCommandTarget
    {
        private IOleCommandTarget m_nextCommandHandler;
        private ITextView m_textView;
        private EpochCompletionHandlerProvider m_provider;
        private ICompletionSession m_session;
        private ITextStructureNavigator m_navigator;
        private ISignatureHelpBroker m_broker;
        private ISignatureHelpSession m_signatureHelpSession;

        internal EpochCompletionCommandHandler(IVsTextView textViewAdapter, ITextView textView, EpochCompletionHandlerProvider provider, ISignatureHelpBroker broker, ITextStructureNavigator nav)
        {
            this.m_textView = textView;
            this.m_provider = provider;
            this.m_navigator = nav;
            this.m_broker = broker;

            //add the command to the command chain
            textViewAdapter.AddCommandFilter(this, out m_nextCommandHandler);
        }

        public int QueryStatus(ref Guid pguidCmdGroup, uint cCmds, OLECMD[] prgCmds, IntPtr pCmdText)
        {
            return m_nextCommandHandler.QueryStatus(ref pguidCmdGroup, cCmds, prgCmds, pCmdText);
        }

        public int Exec(ref Guid pguidCmdGroup, uint nCmdID, uint nCmdexecopt, IntPtr pvaIn, IntPtr pvaOut)
        {
            if (VsShellUtilities.IsInAutomationFunction(m_provider.ServiceProvider))
            {
                return m_nextCommandHandler.Exec(ref pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
            }
            //make a copy of this so we can look at it after forwarding some commands
            uint commandID = nCmdID;
            char typedChar = char.MinValue;
            //make sure the input is a char before getting it
            if (pguidCmdGroup == VSConstants.VSStd2K && nCmdID == (uint)VSConstants.VSStd2KCmdID.TYPECHAR)
            {
                typedChar = (char)(ushort)Marshal.GetObjectForNativeVariant(pvaIn);
            }

            if (typedChar.Equals('('))
            {
                m_signatureHelpSession = m_broker.TriggerSignatureHelp(m_textView);
            }
            else if (typedChar.Equals(')') && m_signatureHelpSession != null)
            {
                m_signatureHelpSession.Dismiss();
                m_signatureHelpSession = null;
            }

            //check for a commit character
            if (nCmdID == (uint)VSConstants.VSStd2KCmdID.RETURN
                || nCmdID == (uint)VSConstants.VSStd2KCmdID.TAB)
            {
                //check for a a selection
                if (m_session != null && !m_session.IsDismissed)
                {
                    //if the selection is fully selected, commit the current session
                    if (m_session.SelectedCompletionSet.SelectionStatus.IsSelected)
                    {
                        m_session.Commit();
                        //also, don't add the character to the buffer
                        return VSConstants.S_OK;
                    }
                    else
                    {
                        //if there is no selection, dismiss the session
                        m_session.Dismiss();
                    }
                }
            }

            //pass along the command so the char is added to the buffer
            int retVal = m_nextCommandHandler.Exec(ref pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
            bool handled = false;
            if (!typedChar.Equals(char.MinValue) && char.IsLetterOrDigit(typedChar))
            {
                if (m_session == null || m_session.IsDismissed) // If there is no active session, bring up completion
                {
                    if (this.TriggerCompletion())
                        m_session.Filter();
                }
                else    //the completion session is already active, so just filter
                {
                    m_session.Filter();
                }
                handled = true;
            }
            else if (commandID == (uint)VSConstants.VSStd2KCmdID.BACKSPACE   //redo the filter if there is a deletion
                || commandID == (uint)VSConstants.VSStd2KCmdID.DELETE)
            {
                if (m_session != null && !m_session.IsDismissed)
                    m_session.Filter();
                handled = true;
            }
            if (handled) return VSConstants.S_OK;
            return retVal;
        }

        private bool TriggerCompletion()
        {
            //the caret must be in a non-projection location 
            SnapshotPoint? caretPoint =
            m_textView.Caret.Position.Point.GetPoint(
            textBuffer => (FilterTextBufferPos(textBuffer)), PositionAffinity.Predecessor);
            if (!caretPoint.HasValue)
            {
                return false;
            }

            EpochClassifier classifier = m_textView.TextBuffer.Properties.GetOrCreateSingletonProperty<EpochClassifier>(creator: () => null);
            if (classifier != null)
            {
                var spans = classifier.GetClassificationSpans(m_textView.GetTextElementSpan(caretPoint.Value));
                if (spans != null)
                {
                    foreach (var span in spans)
                    {
                        if (span.ClassificationType.IsOfType("comment"))
                            return false;

                        if (span.ClassificationType.IsOfType("string"))
                            return false;
                    }
                }
            }

            m_session = m_provider.CompletionBroker.CreateCompletionSession
         (m_textView,
                caretPoint.Value.Snapshot.CreateTrackingPoint(caretPoint.Value.Position, PointTrackingMode.Positive),
                true);

            //subscribe to the Dismissed event on the session 
            m_session.Dismissed += this.OnSessionDismissed;
            m_session.Start();

            return true;
        }

        private bool FilterTextBufferPos(ITextBuffer textBuffer)
        {
            if (textBuffer.ContentType.IsOfType("projection"))
                return false;

            return true;
        }

        private void OnSessionDismissed(object sender, EventArgs e)
        {
            m_session.Dismissed -= this.OnSessionDismissed;
            m_session = null;
        }
    }
}
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
using Epoch.ProjectParser;
using Microsoft.VisualStudio.Text.Operations;

namespace EpochVS
{
    [Export(typeof(IVsTextViewCreationListener))]
    [Name("Epoch name completion handler")]
    [ContentType("EpochFile")]
    [TextViewRole(PredefinedTextViewRoles.Editable)]
    class EditorCompletionCommandHandlerProvider : IVsTextViewCreationListener
    {
        [Import]
        internal IVsEditorAdaptersFactoryService AdapterService = null;
        [Import]
        internal ICompletionBroker CompletionBroker { get; set; }
        [Import]
        internal ISignatureHelpBroker SignatureBroker { get; set; }
        [Import]
        internal SVsServiceProvider ServiceProvider { get; set; }
        [Import]
        internal ITextStructureNavigatorSelectorService NavigatorService { get; set; }

        public void VsTextViewCreated(IVsTextView textViewAdapter)
        {
            ITextView textView = AdapterService.GetWpfTextView(textViewAdapter);
            if (textView == null)
                return;

            Func<EditorCompletionCommandHandler> createCommandHandler = delegate () {
                return new EditorCompletionCommandHandler(textViewAdapter, textView, this, SignatureBroker, NavigatorService.GetTextStructureNavigator(textView.TextBuffer));
            };

            textView.Properties.GetOrCreateSingletonProperty(createCommandHandler);

            ProjectParser.ParseProject(ServiceProvider.GetService(typeof(EnvDTE.DTE)) as EnvDTE.DTE);
        }

    }

    class EditorCompletionCommandHandler : IOleCommandTarget
    {
        private IOleCommandTarget m_nextCommandHandler;
        private ITextView m_textView;
        private EditorCompletionCommandHandlerProvider m_provider;
        private ICompletionSession m_session;
        private ISignatureHelpSession m_signatureSession;
        private ISignatureHelpBroker m_broker;
        private ITextStructureNavigator m_navigator;

        internal EditorCompletionCommandHandler(
            IVsTextView textViewAdapter,
            ITextView textView,
            EditorCompletionCommandHandlerProvider provider,
            ISignatureHelpBroker broker,
            ITextStructureNavigator navigator
        )
        {
            this.m_textView = textView;
            this.m_provider = provider;
            this.m_broker = broker;
            this.m_navigator = navigator;

            ProjectParser.ParseTextBuffer(textView.TextBuffer);

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

            //check for a commit character
            if (nCmdID == (uint)VSConstants.VSStd2KCmdID.RETURN
                || nCmdID == (uint)VSConstants.VSStd2KCmdID.TAB
                || (char.IsWhiteSpace(typedChar) || char.IsPunctuation(typedChar)))
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
                    if (TriggerCompletion() && m_session != null)
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
            else if (typedChar.Equals('('))
            {
                TriggerFunctionSignatureHint();

                handled = true;
            }
            else if (typedChar.Equals(')') && m_signatureSession != null)
            {
                m_signatureSession.Dismiss();
                m_signatureSession = null;
            }

            if (handled) return VSConstants.S_OK;
            return retVal;
        }

        private bool TriggerCompletion()
        {
            //the caret must be in a non-projection location 
            SnapshotPoint? caretPoint =
            m_textView.Caret.Position.Point.GetPoint(
            textBuffer => (!textBuffer.ContentType.IsOfType("projection")), PositionAffinity.Predecessor);
            if (!caretPoint.HasValue)
            {
                return false;
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

        private void TriggerFunctionSignatureHint()
        {
            m_signatureSession = m_broker.TriggerSignatureHelp(m_textView);
        }

        private void OnSessionDismissed(object sender, EventArgs e)
        {
            m_session.Dismissed -= this.OnSessionDismissed;
            m_session = null;
        }
    }
}

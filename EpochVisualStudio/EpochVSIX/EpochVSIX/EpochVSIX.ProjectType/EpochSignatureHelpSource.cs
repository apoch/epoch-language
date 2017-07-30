using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel.Composition;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio.Language.Intellisense;
using Microsoft.VisualStudio.Text;
using Microsoft.VisualStudio.Text.Editor;
using Microsoft.VisualStudio.Utilities;
using Microsoft.VisualStudio.Editor;
using Microsoft.VisualStudio.Text.Operations;
using Microsoft.VisualStudio;
using Microsoft.VisualStudio.TextManager.Interop;
using Microsoft.VisualStudio.OLE.Interop;

namespace EpochVSIX
{
    internal class EpochSignatureHelpSource : ISignatureHelpSource
    {
        private ITextBuffer m_textBuffer;
        private ITextStructureNavigatorSelectorService m_navigator;
        private Parser.Project m_parsedProject;

        public EpochSignatureHelpSource(ITextBuffer textBuffer, ITextStructureNavigatorSelectorService navigator)
        {
            m_textBuffer = textBuffer;
            m_navigator = navigator;
            m_parsedProject = textBuffer.Properties.GetProperty<Parser.Project>(typeof(Parser.Project));
        }

        public void AugmentSignatureHelpSession(ISignatureHelpSession session, IList<ISignature> signatures)
        {
            ITextSnapshot snapshot = m_textBuffer.CurrentSnapshot;
            int position = session.GetTriggerPoint(m_textBuffer).GetPosition(snapshot);

            ITrackingSpan applicableToSpan = m_textBuffer.CurrentSnapshot.CreateTrackingSpan(
             new Span(position, 0), SpanTrackingMode.EdgeInclusive, 0);


            SnapshotPoint point = session.TextView.Caret.Position.BufferPosition - 2;
            TextExtent extent = m_navigator.GetTextStructureNavigator(m_textBuffer).GetExtentOfWord(point);
            string hintfunction = extent.Span.GetText();

            var knownFunctions = m_parsedProject.GetAvailableFunctionSignatures();

            foreach (var func in knownFunctions)
            {
                if (!func.Name.Text.Equals(hintfunction))
                    continue;

                foreach (var overload in func.Overloads)
                    signatures.Add(CreateSignature(m_textBuffer, func.Name.Text, overload, "Documentation goes here.", applicableToSpan));
            }
        }

        private EpochFunctionSignature CreateSignature(ITextBuffer textBuffer, string basename, Parser.FunctionOverload methodSig, string methodDoc, ITrackingSpan span)
        {
            EpochFunctionSignature sig = new EpochFunctionSignature(textBuffer, basename, methodSig, methodDoc, null);
            textBuffer.Changed += new EventHandler<TextContentChangedEventArgs>(sig.OnSubjectBufferChanged);

            List<IParameter> paramList = new List<IParameter>();

            int locusSearchStart = 0;
            for (int i = 0; i < methodSig.Parameters.Count; ++i)
            {
                string param = methodSig.Parameters[i].ToString();

                int locusStart = methodSig.Format(basename).IndexOf(param, locusSearchStart);
                if (locusStart >= 0)
                {
                    Span locus = new Span(locusStart, param.Length);
                    locusSearchStart = locusStart + param.Length;
                    paramList.Add(new EpochParameter("Parameter documentation goes here.", locus, param, sig));
                }
            }

            sig.Parameters = new ReadOnlyCollection<IParameter>(paramList);
            sig.ApplicableToSpan = span;
            sig.ComputeCurrentParameter();
            return sig;
        }

        public ISignature GetBestMatch(ISignatureHelpSession session)
        {
            if (session.Signatures.Count > 0)
            {
                ITrackingSpan applicableToSpan = session.Signatures[0].ApplicableToSpan;
                string text = applicableToSpan.GetText(applicableToSpan.TextBuffer.CurrentSnapshot).Trim();

                foreach (var sig in session.Signatures)
                {
                    if (text.Equals(sig.Content))
                        return sig;
                }
            }
            return null;
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

    internal class EpochParameter : IParameter
    {
        public string Documentation { get; private set; }
        public Span Locus { get; private set; }
        public string Name { get; private set; }
        public ISignature Signature { get; private set; }
        public Span PrettyPrintedLocus { get; private set; }

        public EpochParameter(string documentation, Span locus, string name, ISignature signature)
        {
            Documentation = documentation;
            Locus = locus;
            Name = name;
            Signature = signature;
        }
    }

    internal class EpochFunctionSignature : ISignature
    {
        private ITextBuffer m_subjectBuffer;
        private IParameter m_currentParameter;
        private string m_content;
        private string m_documentation;
        private ITrackingSpan m_applicableToSpan;
        private ReadOnlyCollection<IParameter> m_parameters;
        private string m_printContent;

        internal EpochFunctionSignature(ITextBuffer subjectBuffer, string basename, Parser.FunctionOverload content, string doc, ReadOnlyCollection<IParameter> parameters)
        {
            m_subjectBuffer = subjectBuffer;
            m_content = content.Format(basename);
            m_documentation = doc;
            m_parameters = parameters;
            m_subjectBuffer.Changed += new EventHandler<TextContentChangedEventArgs>(OnSubjectBufferChanged);
        }

        public event EventHandler<CurrentParameterChangedEventArgs> CurrentParameterChanged;


        public IParameter CurrentParameter
        {
            get { return m_currentParameter; }
            internal set
            {
                if (m_currentParameter != value)
                {
                    IParameter prevCurrentParameter = m_currentParameter;
                    m_currentParameter = value;
                    this.RaiseCurrentParameterChanged(prevCurrentParameter, m_currentParameter);
                }
            }
        }

        private void RaiseCurrentParameterChanged(IParameter prevCurrentParameter, IParameter newCurrentParameter)
        {
            EventHandler<CurrentParameterChangedEventArgs> tempHandler = this.CurrentParameterChanged;
            if (tempHandler != null)
            {
                tempHandler(this, new CurrentParameterChangedEventArgs(prevCurrentParameter, newCurrentParameter));
            }
        }

        internal void ComputeCurrentParameter()
        {
            if (Parameters.Count == 0)
            {
                this.CurrentParameter = null;
                return;
            }

            //the number of commas in the string is the index of the current parameter
            string sigText = ApplicableToSpan.GetText(m_subjectBuffer.CurrentSnapshot);

            int currentIndex = 0;
            int commaCount = 0;
            while (currentIndex < sigText.Length)
            {
                int commaIndex = sigText.IndexOf(',', currentIndex);
                if (commaIndex == -1)
                {
                    break;
                }
                commaCount++;
                currentIndex = commaIndex + 1;
            }

            if (commaCount < Parameters.Count)
            {
                this.CurrentParameter = Parameters[commaCount];
            }
            else
            {
                //too many commas, so use the last parameter as the current one.
                this.CurrentParameter = Parameters[Parameters.Count - 1];
            }
        }

        internal void OnSubjectBufferChanged(object sender, TextContentChangedEventArgs e)
        {
            this.ComputeCurrentParameter();
        }

        public ITrackingSpan ApplicableToSpan
        {
            get { return (m_applicableToSpan); }
            internal set { m_applicableToSpan = value; }
        }

        public string Content
        {
            get { return (m_content); }
            internal set { m_content = value; }
        }

        public string Documentation
        {
            get { return (m_documentation); }
            internal set { m_documentation = value; }
        }

        public ReadOnlyCollection<IParameter> Parameters
        {
            get { return (m_parameters); }
            internal set { m_parameters = value; }
        }

        public string PrettyPrintedContent
        {
            get { return (m_printContent); }
            internal set { m_printContent = value; }
        }
    }

    [Export(typeof(ISignatureHelpSourceProvider))]
    [Name("Epoch signature hints")]
    [Order(Before = "default")]
    [ContentType("EpochFile")]
    internal class EpochSignatureHelpSourceProvider : ISignatureHelpSourceProvider
    {
        [Import]
        internal ITextStructureNavigatorSelectorService NavigatorService { get; set; }

        public ISignatureHelpSource TryCreateSignatureHelpSource(ITextBuffer textBuffer)
        {
            return new EpochSignatureHelpSource(textBuffer, NavigatorService);
        }
    }
}

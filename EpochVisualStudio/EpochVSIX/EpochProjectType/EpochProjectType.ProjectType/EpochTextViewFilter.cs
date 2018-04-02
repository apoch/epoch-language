using Microsoft.VisualStudio;
using Microsoft.VisualStudio.OLE.Interop;
using Microsoft.VisualStudio.Package;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio.Text;
using Microsoft.VisualStudio.Text.Editor;
using Microsoft.VisualStudio.TextManager.Interop;

namespace EpochVSIX
{
    class EpochTextViewFilter : ViewFilter, IOleCommandTarget
    {
        private readonly IVsDebugger Debugger;
        private readonly IWpfTextView WpfTextView;
        private readonly IVsTextLines TextLines;

        public EpochTextViewFilter(IVsDebugger debugger, CodeWindowManager mgr, IVsTextView textView, IWpfTextView wpfTextView)
            : base(mgr, textView)
        {
            ThreadHelper.ThrowIfNotOnUIThread();

            Debugger = debugger;
            WpfTextView = wpfTextView;

            textView.GetBuffer(out TextLines);
        }



        public override int GetDataTipText(TextSpan[] pSpan, out string pbstrText)
        {
            //ThreadHelper.ThrowIfNotOnUIThread();

            //if (!WpfTextView.TextBuffer.ContentType.IsOfType("Epoch"))
            //{
                pbstrText = null;
                return VSConstants.E_NOTIMPL;
            //}

            /*
            if (pSpan.Length != 1)
            {
                throw new ArgumentException("Array parameter should contain exactly one TextSpan", "pSpan");
            }

            // Adjust the span to expression boundaries.

            var snapshot = WpfTextView.TextSnapshot;
            var start = LineAndColumnNumberToSnapshotPoint(snapshot, pSpan[0].iStartLine, pSpan[0].iStartIndex);
            var end = LineAndColumnNumberToSnapshotPoint(snapshot, pSpan[0].iEndLine, pSpan[0].iEndIndex);

            // If this is a zero-length span (which it usually is, unless there's selection), adjust it
            // to cover one char to the right, since an empty span at the beginning of the expression does
            // not count as belonging to that expression;
            if (start == end && start.Position != snapshot.Length)
            {
                end += 1;
            }

            var snapshotSpan = new SnapshotSpan(start, end);
            var trackingSpan = snapshot.CreateTrackingSpan(snapshotSpan.Span, SpanTrackingMode.EdgeExclusive);

            /*
            var rep = new ReverseExpressionParser(snapshot, WpfTextView.TextBuffer, trackingSpan);
            var exprSpan = rep.GetExpressionRange(forCompletion: false);
            string expr = null;
            if (exprSpan != null)
            {
                SnapshotPointToLineAndColumnNumber(exprSpan.Value.Start, out pSpan[0].iStartLine, out pSpan[0].iStartIndex);
                SnapshotPointToLineAndColumnNumber(exprSpan.Value.End, out pSpan[0].iEndLine, out pSpan[0].iEndIndex);
                expr = VsProjectAnalyzer.ExpressionForDataTipAsync(
                    _serviceProvider,
                    _wpfTextView,
                    exprSpan.Value,
                    TimeSpan.FromSeconds(1.0)
                ).WaitAndUnwrapExceptions(Dispatcher.CurrentDispatcher);
            }
            else
            {
                // If it's not an expression, suppress the tip.
                pbstrText = null;
                return VSConstants.E_FAIL;
            }* /

            return Debugger.GetDataTipValue(TextLines, pSpan, "count", out pbstrText);
            */
        }

        public override int GetPairExtents(int iLine, int iIndex, TextSpan[] pSpan)
        {
            return VSConstants.E_NOTIMPL;
        }

        public override int GetWordExtent(int iLine, int iIndex, uint dwFlags, TextSpan[] pSpan)
        {
            return VSConstants.E_NOTIMPL;
        }


        private static SnapshotPoint LineAndColumnNumberToSnapshotPoint(ITextSnapshot snapshot, int lineNumber, int columnNumber)
        {
            var line = snapshot.GetLineFromLineNumber(lineNumber);
            var snapshotPoint = new SnapshotPoint(snapshot, line.Start + columnNumber);
            return snapshotPoint;
        }

        private static void SnapshotPointToLineAndColumnNumber(SnapshotPoint snapshotPoint, out int lineNumber, out int columnNumber)
        {
            var line = snapshotPoint.GetContainingLine();
            lineNumber = line.LineNumber;
            columnNumber = snapshotPoint.Position - line.Start.Position;
        }
    }
}

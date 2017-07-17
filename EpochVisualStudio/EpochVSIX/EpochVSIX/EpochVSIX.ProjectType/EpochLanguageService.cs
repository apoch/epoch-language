using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Package;
using Microsoft.VisualStudio.TextManager.Interop;
using Microsoft.VisualStudio.OLE.Interop;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio.Editor;
using System.ComponentModel.Composition;

namespace EpochVSIX
{
    [Export(typeof(EpochLanguageService))]
    [Guid("e0ddc0eb-23e7-45c7-8726-6fd20628992a")]
    class EpochLanguageService : LanguageService, IVsLanguageDebugInfo
    {
        private LanguagePreferences m_preferences;
        private EpochScanner m_scanner;

        internal IVsEditorAdaptersFactoryService AdapterService = null;

        public EpochLanguageService(IVsEditorAdaptersFactoryService service)
        {
            AdapterService = service;
        }

        public override string Name
        {
            get
            {
                return "Epoch Language";
            }
        }

        public override ViewFilter CreateViewFilter(CodeWindowManager mgr, IVsTextView newView)
        {
            return new EpochTextViewFilter(base.GetIVsDebugger(), mgr, newView, AdapterService.GetWpfTextView(newView));
        }

        public override string GetFormatFilterList()
        {
            return ".epoch";
        }

        public override LanguagePreferences GetLanguagePreferences()
        {
            if (m_preferences == null)
            {
                m_preferences = new LanguagePreferences(this.Site,
                                                        typeof(EpochLanguageService).GUID,
                                                        this.Name);
                m_preferences.Init();
            }
            return m_preferences;
        }

        public override IScanner GetScanner(IVsTextLines buffer)
        {
            if (m_scanner == null)
                m_scanner = new EpochScanner(buffer);

            return m_scanner;
        }

        public override AuthoringScope ParseSource(ParseRequest req)
        {
            return new EpochAuthoringScope();
        }

        public override int QueryService(ref Guid guidService, ref Guid iid, out IntPtr obj)
        {
            return base.QueryService(ref guidService, ref iid, out obj);
        }
    }

    internal class EpochScanner : IScanner
    {
        private IVsTextBuffer m_buffer;
        string m_source;

        public EpochScanner(IVsTextBuffer buffer)
        {
            m_buffer = buffer;
        }

        bool IScanner.ScanTokenAndProvideInfoAboutIt(TokenInfo tokenInfo, ref int state)
        {
            tokenInfo.Type = TokenType.Unknown;
            tokenInfo.Color = TokenColor.Text;
            return false;
        }

        void IScanner.SetSource(string source, int offset)
        {
            m_source = source.Substring(offset);
        }
    }

    internal class EpochAuthoringScope : AuthoringScope
    {
        public override string GetDataTipText(int line, int col, out TextSpan span)
        {
            span = new TextSpan();
            return "test";
        }

        public override Declarations GetDeclarations(IVsTextView view,
                                                     int line,
                                                     int col,
                                                     TokenInfo info,
                                                     ParseReason reason)
        {
            var ret = new EpochDeclarations();
            return ret;
        }

        public override string Goto(VSConstants.VSStd97CmdID cmd, IVsTextView textView, int line, int col, out TextSpan span)
        {
            span = new TextSpan();
            return null;
        }

        public override Methods GetMethods(int line, int col, string name)
        {
            return null;
        }
    }

    internal class EpochDeclarations : Declarations
    {
        public override int GetCount()
        {
            return 0;
        }

        public override string GetDescription(int index)
        {
            return "";
        }

        public override string GetDisplayText(int index)
        {
            return "";
        }

        public override int GetGlyph(int index)
        {
            return 0;
        }

        public override string GetName(int index)
        {
            return "";
        }
    }

}

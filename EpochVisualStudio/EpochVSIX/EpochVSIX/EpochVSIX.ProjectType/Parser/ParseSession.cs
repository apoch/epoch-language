using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio.TextManager.Interop;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class ParseSession
    {

        private LexSession Lexer;
        private ErrorListProvider ErrorProvider;



        internal class ErrorListHelper : IServiceProvider
        {
            public object GetService(Type serviceType)
            {
                return Package.GetGlobalService(serviceType);
            }
        }


        public ParseSession(LexSession lexer)
        {
            Lexer = lexer;

            var helper = new ErrorListHelper();
            ErrorProvider = new ErrorListProvider(helper);
            ErrorProvider.ProviderName = "Epoch Language";
            ErrorProvider.ProviderGuid = new Guid(VsPackage.PackageGuid);
        }

        public bool AugmentProject(Project project)
        {
            try
            {
                while (!Lexer.Empty)
                {
                    var sumtype = SumType.Parse(this);
                    if (sumtype != null)
                    {
                        project.RegisterSumType(sumtype.Name, sumtype.Type);
                        continue;
                    }

                    var strongalias = StrongAlias.Parse(this);
                    if (strongalias != null)
                    {
                        project.RegisterStrongAlias(strongalias.Name, strongalias.Type);
                        continue;
                    }

                    var weakalias = WeakAlias.Parse(this);
                    if (weakalias != null)
                    {
                        project.RegisterWeakAlias(weakalias.Name, weakalias.Type);
                        continue;
                    }

                    var structure = Structure.Parse(this);
                    if (structure != null)
                    {
                        project.RegisterStructureType(structure.Name, structure.Object);
                        continue;
                    }

                    var globals = GlobalBlock.Parse(this);
                    if (globals != null)
                    {
                        globals.AugmentProject(project);
                        continue;
                    }

                    var function = FunctionSignature.Parse(this);
                    if (function != null)
                    {
                        project.RegisterFunction(function);
                        continue;
                    }

                    if (!Lexer.Empty)
                        throw new Exception("Syntax error");        // TODO - improve error messages
                }
            }
            catch (Exception ex)
            {
                var errorTask = new ErrorTask();
                errorTask.Text = ex.Message;
                errorTask.Category = TaskCategory.CodeSense;
                errorTask.ErrorCategory = TaskErrorCategory.Error;
                errorTask.Document = Lexer.File;
                errorTask.Line = (!Lexer.Empty && Lexer.PeekToken(0) != null) ? (Lexer.PeekToken(0).Line) : 0;
                errorTask.Column = (!Lexer.Empty && Lexer.PeekToken(0) != null) ? (Lexer.PeekToken(0).Column) : 0;
                errorTask.Navigate += NavigationHandler;

                ErrorProvider.Tasks.Add(errorTask);
                return false;
            }

            return true;
        }

        internal bool CheckToken(int offset, string expected)
        {
            if (Lexer.Empty)
                return false;

            var token = Lexer.PeekToken(offset);
            if (token == null)
                return false;

            return token.Text.Equals(expected);
        }

        internal void ConsumeTokens(int count)
        {
            Lexer.ConsumeTokens(count);
        }

        internal Token PeekToken(int offset)
        {
            if (Lexer.Empty)
                return null;

            return Lexer.PeekToken(offset);
        }


        internal bool ParsePreopStatement(int startoffset, out int consumedtokens)
        {
            consumedtokens = startoffset;
            bool recognized = false;
            var token = PeekToken(startoffset);
            if (token == null)
                return false;

            string potential = token.Text;

            if (potential == "++")
                recognized = true;
            else if (potential == "--")
                recognized = true;

            if (recognized)
            {
                do
                {
                    startoffset += 2;
                } while (CheckToken(startoffset, "."));

                consumedtokens = startoffset;
                return true;
            }

            return false;
        }

        internal bool ParsePostopStatement(int startoffset, out int consumedtokens)
        {
            consumedtokens = startoffset;
            int totaltokens = startoffset;

            var token = PeekToken(totaltokens);
            if (token == null)
                return false;

            while (CheckToken(totaltokens + 1, "."))
                ++totaltokens;

            totaltokens += 2;

            var potential = PeekToken(totaltokens);
            if (potential == null)
                return false;

            if (potential.Text == "++" || potential.Text == "--")
            {
                consumedtokens = totaltokens;
                return true;
            }

            return false;
        }

        internal bool ParseStatement(int startoffset, out int consumedtokens)
        {
            consumedtokens = startoffset;
            int totaltokens = consumedtokens;

            if (CheckToken(totaltokens + 1, "<"))
            {
                var token = PeekToken(totaltokens);
                totaltokens += 2;
                if (!ParseTemplateArguments(totaltokens, token, out totaltokens))
                    return false;
            }
            else
                ++totaltokens;

            if (!CheckToken(totaltokens, "("))
                return false;

            ++totaltokens;
            while (!CheckToken(totaltokens, ")"))
            {
                var expr = Expression.Parse(this, totaltokens, out totaltokens);
                if (expr == null)
                    return false;

                if (CheckToken(totaltokens, ","))
                    ++totaltokens;
            }

            ++totaltokens;
            consumedtokens = totaltokens;
            return true;
        }


        internal bool ParseTemplateParameters(int startoffset, Token basetypename, out int totaltokens)
        {
            totaltokens = startoffset;
            bool hasparams = true;

            while (hasparams)
            {
                totaltokens += 2;

                if (CheckToken(totaltokens, ">"))
                {
                    ++totaltokens;
                    hasparams = false;
                }
                else if (CheckToken(totaltokens, ","))
                    ++totaltokens;
                else
                    return false;
            }

            return true;
        }

        internal bool ParseTemplateArguments(int startoffset, Token basetypename, out int consumedtokens)
        {
            consumedtokens = startoffset;
            int totaltokens = startoffset;
            bool hasargs = true;

            while (hasargs)
            {
                ++totaltokens;

                if (CheckToken(totaltokens, ">"))
                    hasargs = false;
                else if (CheckToken(totaltokens, ","))
                    ++totaltokens;
                else
                    return false;

            }

            ++totaltokens;
            consumedtokens = totaltokens;
            return true;
        }


        internal async void NavigationHandler(object sender, EventArgs args)
        {
            var task = sender as Microsoft.VisualStudio.Shell.Task;
            if (string.IsNullOrEmpty(task.Document))
                return;

            await ThreadHelper.JoinableTaskFactory.SwitchToMainThreadAsync();

            var serviceProvider = new ErrorListHelper();
            var openDoc = serviceProvider.GetService(typeof(IVsUIShellOpenDocument)) as IVsUIShellOpenDocument;
            if (openDoc == null)
                return;

            IVsWindowFrame frame;
            Microsoft.VisualStudio.OLE.Interop.IServiceProvider sp;
            IVsUIHierarchy hier;
            uint itemid;
            Guid logicalView = VSConstants.LOGVIEWID_Code;

            if (ErrorHandler.Failed(openDoc.OpenDocumentViaProject(task.Document, ref logicalView, out sp, out hier, out itemid, out frame)) || frame == null)
                return;

            object docData;
            frame.GetProperty((int)__VSFPROPID.VSFPROPID_DocData, out docData);

            VsTextBuffer buffer = docData as VsTextBuffer;
            if (buffer == null)
            {
                IVsTextBufferProvider bufferProvider = docData as IVsTextBufferProvider;
                if (bufferProvider != null)
                {
                    IVsTextLines lines;
                    ErrorHandler.ThrowOnFailure(bufferProvider.GetTextBuffer(out lines));
                    buffer = lines as VsTextBuffer;

                    if (buffer == null)
                        return;
                }
            }

            IVsTextManager mgr = serviceProvider.GetService(typeof(VsTextManagerClass)) as IVsTextManager;
            if (mgr == null)
                return;

            // TODO - this for some reason loads our code as JSON!
            mgr.NavigateToLineAndColumn(buffer, ref logicalView, task.Line, task.Column, task.Line, task.Column);
        }
    }
}

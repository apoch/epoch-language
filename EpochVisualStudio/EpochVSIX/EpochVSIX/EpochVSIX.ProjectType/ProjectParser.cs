using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.VisualStudio.Text;
using Microsoft.VisualStudio.TextManager.Interop;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio.Threading;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio;

namespace EpochVSIX
{
    internal class ErrorListHelper : IServiceProvider
    {
        public object GetService(Type serviceType)
        {
            return Package.GetGlobalService(serviceType);
        }
    }

    public class ProjectParser
    {
        private static ProjectParser Instance = null;

        internal static ProjectParser GetInstance()
        {
            if (Instance == null)
                Instance = new ProjectParser();

            return Instance;
        }


        private class Token
        {
            public string Text;
            public int Line;
            public int Column;
        }

        internal class Variable
        {
            public string Name;
            public string Type;
        }

        internal class Scope
        {
            public int StartLine;
            public int StartColumn;
            public int EndLine;
            public int EndColumn;

            public List<Variable> Variables = new List<Variable>();
        }


        private ErrorListProvider ErrorProvider = null;

        private Dictionary<string, List<string>> ParsedFunctionNames = null;
        private Dictionary<string, List<string>> ParsedStructures = null;
        private Dictionary<string, List<string>> ParsedTypes = null;

        private Dictionary<string, Dictionary<string, FunctionDefinition>> FunctionDefinitions = null;
        private Dictionary<string, StructureDefinition> StructureDefinitions = null;

        private Dictionary<string, List<Scope>> ParsedScopes = null;
        private Stack<List<Variable>> ParsedScopeStack = new Stack<List<Variable>>();


        internal class StructureDefinition
        {
            public List<StructureMember> Members = null;
        };

        public class FunctionDefinition
        {
            public string FunctionName;

            public List<FunctionParameter> Parameters = null;
            public string ReturnType;
            public Dictionary<string, FunctionTag> Tags;

            public override string ToString()
            {
                string ret = string.Format("{0} : {1}", FunctionName, string.Join(", ", Parameters));
                if (ReturnType != null && ReturnType.Length > 0)
                    ret += " -> " + ReturnType;

                return ret;
            }
        };

        public class FunctionParameter
        {
            public string Name;
            public string Type;

            public override string ToString()
            {
                return string.Format("{0} {1}", Type, Name);
            }
        };

        public class FunctionTag
        {
            public string TagName;
            public List<string> TagParams;
        };

        internal class StructureMember
        {
            public string Name;
            public string Type;
        };

        private enum CharacterClass
        {
            White,
            Comment,
            Identifier,
            Punctuation,
            PunctuationCompound,
            Literal,
            StringLiteral,
        };


        private Scope GlobalScope = null;

        internal ProjectParser()
        {
            var helper = new ErrorListHelper();

            ErrorProvider = new ErrorListProvider(helper);
            ErrorProvider.ProviderName = "Epoch Language";
            ErrorProvider.ProviderGuid = new Guid(VsPackage.PackageGuid);
        }


        private CharacterClass LexerClassify(char c, CharacterClass currentclass)
        {
            if ("abcdefx".Contains(c))
            {
                if (currentclass == CharacterClass.Literal)
                    return CharacterClass.Literal;

                return CharacterClass.Identifier;
            }

            if ("0123456789".Contains(c))
            {
                if (currentclass == CharacterClass.Identifier)
                    return CharacterClass.Identifier;

                return CharacterClass.Literal;
            }

            if ("{}:(),;[]".Contains(c))
                return CharacterClass.Punctuation;

            if ("=&+-<>!".Contains(c))
                return CharacterClass.PunctuationCompound;

            if (c == '\"')
                return CharacterClass.StringLiteral;

            if (c == '.')
            {
                if (currentclass == CharacterClass.Literal)
                    return CharacterClass.Literal;

                if (currentclass == CharacterClass.PunctuationCompound)
                    return CharacterClass.PunctuationCompound;              // minor hack

                return CharacterClass.Punctuation;
            }

            return CharacterClass.Identifier;
        }

        private bool IsValidPunctuation(string token)
        {
            if (token == "==")
                return true;

            if (token == "!=")
                return true;

            if (token == "++")
                return true;

            if (token == "--")
                return true;

            if (token == "->")
                return true;

            if (token == "&&")
                return true;

            if (token == "+=")
                return true;

            if (token == "-=")
                return true;

            if (token == "=>")
                return true;

            return false;
        }


        private List<Token> Lex(string code)
        {
            var tokens = new List<Token>();

            int index = 0;
            int lasttokenstart = 0;

            int lastLineIndex = 0;
            int currentLineIndex = 0;
            int currentColumnIndex = 0;

            CharacterClass state = CharacterClass.White;
            CharacterClass prevstate = CharacterClass.White;

            while (index < code.Length)
            {
                char c = code[index];

                if (state == CharacterClass.White)
                {
                    if ((c == '/') && (code[index + 1] == '/'))
                    {
                        state = CharacterClass.Comment;
                    }
                    else if (!char.IsWhiteSpace(c))
                    {
                        state = LexerClassify(c, state);
                        lasttokenstart = index;
                    }
                    else if (c == '\n')
                    {
                        ++currentLineIndex;
                        currentColumnIndex = 0;
                    }
                }
                else if (state == CharacterClass.Identifier)
                {
                    bool notidentifier = false;
                    if (char.IsWhiteSpace(c))
                    {
                        notidentifier = true;
                        state = CharacterClass.White;
                    }
                    else if (LexerClassify(c, state) != CharacterClass.Identifier)
                    {
                        notidentifier = true;
                        state = LexerClassify(c, state);
                    }

                    if (notidentifier)
                        tokens.Add(new Token { Text = code.Substring(lasttokenstart, index - lasttokenstart), Line = currentLineIndex, Column = currentColumnIndex });
                }
                else if (state == CharacterClass.Punctuation)
                {
                    if (char.IsWhiteSpace(c))
                        state = CharacterClass.White;
                    else if (LexerClassify(c, state) != CharacterClass.Punctuation)
                        state = LexerClassify(c, state);

                    tokens.Add(new Token { Text = code.Substring(lasttokenstart, index - lasttokenstart), Line = currentLineIndex, Column = currentColumnIndex });
                    lasttokenstart = index;
                }
                else if (state == CharacterClass.PunctuationCompound)
                {
                    bool notcompound = false;
                    if (char.IsWhiteSpace(c))
                    {
                        notcompound = true;
                        state = CharacterClass.White;
                    }
                    else if (LexerClassify(c, state) != CharacterClass.PunctuationCompound)
                    {
                        notcompound = true;
                        state = LexerClassify(c, state);
                    }
                    else
                    {
                        if ((index - lasttokenstart) > 1)
                        {
                            string potentialtoken = code.Substring(lasttokenstart, index - lasttokenstart);
                            if (!IsValidPunctuation(potentialtoken))
                            {
                                tokens.Add(new Token { Text = potentialtoken.Substring(0, potentialtoken.Length - 1), Line = currentLineIndex, Column = currentColumnIndex });
                                lasttokenstart = index - 1;
                            }
                        }
                    }

                    if (notcompound)
                    {
                        if ((index - lasttokenstart) > 1)
                        {
                            string potentialtoken = code.Substring(lasttokenstart, index - lasttokenstart);
                            if (!IsValidPunctuation(potentialtoken))
                            {
                                tokens.Add(new Token { Text = potentialtoken.Substring(0, potentialtoken.Length - 1), Line = currentLineIndex, Column = currentColumnIndex });
                                lasttokenstart = index - 1;
                            }
                        }

                        tokens.Add(new Token { Text = code.Substring(lasttokenstart, index - lasttokenstart), Line = currentLineIndex, Column = currentColumnIndex });
                    }
                }
                else if (state == CharacterClass.Comment)
                {
                    if (c == '\r')
                        state = CharacterClass.White;
                    else if (c == '\n')
                    {
                        state = CharacterClass.White;
                        ++currentLineIndex;
                        currentColumnIndex = 0;
                    }
                }
                else if (state == CharacterClass.StringLiteral)
                {
                    if (c == '\"')
                    {
                        state = CharacterClass.White;
                        tokens.Add(new Token { Text = code.Substring(lasttokenstart, index - lasttokenstart + 1), Line = currentLineIndex, Column = currentColumnIndex });
                    }
                }
                else if (state == CharacterClass.Literal)
                {
                    bool notliteral = false;
                    if (char.IsWhiteSpace(c))
                    {
                        notliteral = true;
                        state = CharacterClass.White;
                    }
                    else if (LexerClassify(c, state) != CharacterClass.Literal)
                    {
                        notliteral = true;
                        state = LexerClassify(c, state);
                    }

                    if (notliteral)
                        tokens.Add(new Token { Text = code.Substring(lasttokenstart, index - lasttokenstart), Line = currentLineIndex, Column = currentColumnIndex });
                }

                // Hack for negated literals
                if (state == CharacterClass.PunctuationCompound)
                {
                    if (LexerClassify(code[index + 1], state) == CharacterClass.Literal)
                        state = CharacterClass.Literal;
                }

                if (state != prevstate)
                    lasttokenstart = index;

                prevstate = state;
                ++index;

                if (currentLineIndex == lastLineIndex)
                    ++currentColumnIndex;

                lastLineIndex = currentLineIndex;
            }

            if ((lasttokenstart < code.Length) && (state != CharacterClass.White))
                tokens.Add(new Token { Text = code.Substring(lasttokenstart, code.Length - lasttokenstart), Line = currentLineIndex, Column = currentColumnIndex });

            return tokens;
        }

        private bool ParseTemplateParameters(List<Token> tokens)
        {
            bool hasparams = true;

            while (hasparams)
            {
                tokens.RemoveRange(0, 2);

                if (tokens[0].Text == ">")
                    hasparams = false;
                else if (tokens[0].Text == ",")
                    tokens.RemoveAt(0);
                else
                    return false;
            }

            return true;
        }

        private int ScanTemplateArgs(List<Token> tokens, int initialconsume)
        {
            int consumed = initialconsume;
            bool hasargs = true;
            while (hasargs)
            {
                ++consumed;

                if (tokens[consumed].Text == ">")
                    hasargs = false;
                else if (tokens[consumed].Text == ",")
                    ++consumed;
                else
                    return 0;
            }

            return consumed;
        }

        private int ParseTemplateArgs(List<Token> tokens, int initialconsume)
        {
            int consumed = initialconsume;

            bool hasargs = true;
            while (hasargs)
            {
                ++consumed;

                if (tokens[consumed].Text == ">")
                    hasargs = false;
                else if (tokens[consumed].Text == ",")
                    ++consumed;
                else
                    return 0;

            }

            return consumed;
        }

        private void ParseSumTypeBases(List<Token> tokens)
        {
            bool hasbases = true;
            do
            {
                if (tokens[1].Text == "<")
                {
                    int baselookahead = ParseTemplateArgs(tokens, 2);
                    tokens.RemoveRange(0, baselookahead + 2);
                }
                else
                {
                    tokens.RemoveRange(0, 2);
                }

                hasbases = (tokens[1].Text == "|");
            } while (hasbases);
        }

        private bool ParseSumType(string filename, List<Token> tokens)
        {
            if (tokens[0].Text != "type")
                return false;

            string sumtypename = tokens[1].Text;
            if (tokens[2].Text == "<")
            {
                tokens.RemoveRange(0, 3);
                if (!ParseTemplateParameters(tokens))
                    return false;

                if (tokens[1].Text != ":")
                    return false;
            }
            else if (tokens[2].Text != ":")
                return false;
            else if (tokens[4].Text != "|")
                return false;
            else
                tokens.RemoveAt(0);

            tokens.RemoveRange(0, 2);

            ParsedTypes[filename].Add(sumtypename);

            ParseSumTypeBases(tokens);

            tokens.RemoveAt(0);

            return true;
        }

        private bool ParseStrongAlias(string filename, List<Token> tokens)
        {
            if (tokens[0].Text != "type")
                return false;

            if (tokens[2].Text != ":")
                return false;

            tokens.RemoveAt(0);

            ParsedTypes[filename].Add(tokens[0].Text);

            tokens.RemoveRange(0, 3);

            return true;
        }

        private bool ParseWeakAlias(string filename, List<Token> tokens)
        {
            if (tokens[0].Text != "alias")
                return false;

            if (tokens[2].Text != "=")
                return false;

            tokens.RemoveAt(0);

            ParsedTypes[filename].Add(tokens[0].Text);

            tokens.RemoveRange(0, 3);
            return true;
        }

        private bool ParseStructure(string filename, List<Token> tokens)
        {
            bool templated = false;

            if (tokens[0].Text != "structure")
                return false;

            string structurename = tokens[1].Text;

            if (tokens[2].Text == "<")
            {
                tokens.RemoveRange(0, 3);
                if (!ParseTemplateParameters(tokens))
                    return false;

                templated = true;

                if (tokens[1].Text != ":")
                    return false;
            }
            else if (tokens[2].Text != ":")
            {
                return false;
            }

            tokens.RemoveRange(0, 2);

            if (!templated)
                tokens.RemoveAt(0);

            ParsedStructures[filename].Add(structurename);

            bool moremembers = true;
            while (moremembers)
            {
                if (tokens[0].Text == "(")
                {
                    tokens.RemoveAt(0);

                    // TODO - members that are function pointers?
                    tokens.RemoveAt(0);

                    if (tokens[0].Text != ":")
                        return false;

                    tokens.RemoveAt(0);

                    bool moreparams = true;
                    while (moreparams)
                    {
                        tokens.RemoveAt(0);
                        if (tokens[0].Text != ",")
                        {
                            moreparams = false;
                        }
                        else
                            tokens.RemoveAt(0);
                    }

                    if (tokens[0].Text == "->")
                    {
                        tokens.RemoveAt(0);
                        tokens.RemoveAt(0);
                    }

                    if (tokens[0].Text != ")")
                    {
                        return false;
                    }

                    tokens.RemoveAt(0);
                }
                else
                {
                    bool isref = false;

                    string membertype = tokens[0].Text;

                    tokens.RemoveAt(0);

                    string membername = tokens[0].Text;

                    tokens.RemoveAt(0);

                    if (membername == "<")
                    {
                        int memberlookahead = ParseTemplateArgs(tokens, 0);
                        if (memberlookahead > 0)
                        {
                            tokens.RemoveRange(0, memberlookahead + 1);
                            membername = tokens[0].Text;
                            tokens.RemoveAt(0);
                        }
                        else
                        {
                            return false;
                        }
                    }

                    if (membername == "ref")
                    {
                        isref = true;
                        membername = tokens[0].Text;
                        tokens.RemoveAt(0);
                    }

                    if (!StructureDefinitions.ContainsKey(structurename))
                    {
                        StructureDefinitions[structurename] = new StructureDefinition();
                        StructureDefinitions[structurename].Members = new List<StructureMember>();
                    }

                    var member = new StructureMember();
                    member.Name = membername;
                    member.Type = membertype;

                    if (isref)
                        member.Type += " ref";

                    StructureDefinitions[structurename].Members.Add(member);
                }

                if (tokens.Count < 1 || tokens[0].Text != ",")
                    moremembers = false;
                else
                    tokens.RemoveAt(0);
            }

            return true;
        }

        private bool ParseGlobalBlock(string filename, List<Token> tokens)
        {
            if (tokens[0].Text != "global")
                return false;

            if (tokens[1].Text != "{")
                return false;

            tokens.RemoveRange(0, 2);

            while ((tokens.Count > 0) && (tokens[0].Text != "}"))
            {
                if (!ParseInitialization(tokens, filename, false, null))
                    return false;
            }

            if (tokens.Count > 0)
                tokens.RemoveAt(0);

            return true;
        }

        private bool ParseTask(string filename, List<Token> tokens)
        {
            // TODO
            return false;
        }

        private bool ParseFunction(string filename, List<Token> tokens)
        {
            if (tokens.Count < 2)
                return false;

            string functionname = tokens[0].Text;

            if (tokens[1].Text == "<")
            {
                tokens.RemoveRange(0, 2);
                if (!ParseTemplateParameters(tokens))
                    return false;
            }

            if (tokens[1].Text != ":")
                return false;

            tokens.RemoveRange(0, 2);

            var sig = CreateFunctionSignatureWithOverloading(filename, functionname);
            functionname = sig.FunctionName;

            if (tokens[0].Text != "[")
            {
                ParseFunctionParams(tokens, filename, functionname);
                ParseFunctionReturn(tokens, filename, functionname);
            }
            ParseFunctionTags(tokens, sig);

            if (sig.Tags == null || !sig.Tags.ContainsKey("constructor"))
                ParsedFunctionNames[filename].Add(functionname);

            if (tokens.Count <= 0 || tokens[0].Text != "{")
                return true;

            tokens.RemoveAt(0);

            ParseCodeBlock(tokens, filename, functionname);

            return true;
        }

        private void ParseFunctionParams(List<Token> tokens, string filename, string functionname)
        {
            if (tokens.Count == 0)
                return;

            var sig = GetFunctionSignature(filename, functionname);

            while ((tokens[0].Text != "{") && (tokens[0].Text != "->"))
            {
                if (tokens[0].Text == "nothing")
                {
                    var nothingparam = new FunctionParameter();
                    nothingparam.Name = "";
                    nothingparam.Type = "nothing";
                    sig.Parameters.Add(nothingparam);

                    tokens.RemoveAt(0);
                }
                else if (tokens[0].Text == "(")
                {
                    tokens.RemoveAt(0);

                    // TODO - better representation of higher order functions
                    var funcparam = new FunctionParameter();
                    funcparam.Name = "";
                    funcparam.Type = "(" + tokens[0] + " : -> )";
                    sig.Parameters.Add(funcparam);

                    if (tokens[1].Text == ":")
                    {
                        tokens.RemoveRange(0, 2);

                        bool moreparams = true;
                        if (tokens[0].Text == ")")
                            moreparams = false;

                        while (moreparams)
                        {
                            tokens.RemoveAt(0);
                            if (tokens[0].Text == "ref")
                                tokens.RemoveAt(0);

                            if (tokens[0].Text != ",")
                                moreparams = false;
                            else
                                tokens.RemoveAt(0);
                        }

                        if (tokens[0].Text == "->")
                            tokens.RemoveRange(0, 2);

                        if (tokens[0].Text != ")")
                            return;
                    }

                    tokens.RemoveAt(0);
                }
                else if (HandleLiteralFunctionParam(tokens[0].Text))
                {
                    var literalparam = new FunctionParameter();
                    literalparam.Name = "";
                    literalparam.Type = tokens[0].Text;
                    sig.Parameters.Add(literalparam);

                    tokens.RemoveAt(0);
                }
                else
                {
                    int lookahead = 0;
                    if (tokens[1].Text == "<")
                        lookahead = ParseTemplateArgs(tokens, 2);

                    string typetoken = string.Join(" ", tokens.GetRange(0, 1 + lookahead).ConvertAll(a => a.Text));
                    string nametoken = tokens[1 + lookahead].Text;

                    if (nametoken == "ref")
                    {
                        tokens.RemoveAt(0);
                        nametoken = tokens[1].Text;

                        typetoken += " ref";
                    }

                    tokens.RemoveRange(0, 2 + lookahead);

                    var param = new FunctionParameter();
                    param.Type = typetoken;
                    param.Name = nametoken;
                    sig.Parameters.Add(param);
                }

                if (tokens.Count < 1 || tokens[0].Text != ",")
                    return;

                tokens.RemoveAt(0);
            }
        }

        private void ParseFunctionReturn(List<Token> tokens, string filename, string functionname)
        {
            if (tokens.Count < 1 || tokens[0].Text != "->")
                return;

            tokens.RemoveAt(0);

            if (!ParseInitialization(tokens, filename, true, functionname))
            {
                ParseExpression(tokens);
                GetFunctionSignature(filename, functionname).ReturnType = "<expr>";
            }
        }

        private FunctionDefinition GetFunctionSignature(string filename, string functionname)
        {
            return FunctionDefinitions[filename][functionname];
        }

        private FunctionDefinition CreateFunctionSignatureWithOverloading(string filename, string functionname)
        {
            if (FunctionDefinitions == null)
                FunctionDefinitions = new Dictionary<string, Dictionary<string, FunctionDefinition>>();

            if (!FunctionDefinitions.ContainsKey(filename))
                FunctionDefinitions.Add(filename, new Dictionary<string, FunctionDefinition>());

            int i = 0;
            var overloadname = functionname;
            var defs = FunctionDefinitions[filename];
            while (defs.ContainsKey(overloadname))
            {
                ++i;
                overloadname = $"{functionname}{i}";
            }

            var newfunc = new FunctionDefinition();
            newfunc.FunctionName = overloadname;
            newfunc.Parameters = new List<FunctionParameter>();
            newfunc.ReturnType = null;
            newfunc.Tags = null;

            defs.Add(overloadname, newfunc);
            return newfunc;
        }

        private void ParseFunctionTags(List<Token> tokens, FunctionDefinition func)
        {
            if (tokens.Count <= 0)
                return;

            if (tokens[0].Text != "[")
                return;

            tokens.RemoveAt(0);

            while (tokens[0].Text != "]")
                ParseSingleFunctionTag(tokens, func);

            tokens.RemoveAt(0);
        }

        private void ParseSingleFunctionTag(List<Token> tokens, FunctionDefinition func)
        {
            var tag = new FunctionTag();
            tag.TagName = tokens[0].Text;

            if (func.Tags == null)
                func.Tags = new Dictionary<string, FunctionTag>();

            func.Tags.Add(tag.TagName, tag);

            if (tokens[1].Text == "(")
            {
                // TODO - parse tag params
                tokens.RemoveRange(0, 2);

                while (tokens[0].Text != ")")
                {
                    tokens.RemoveAt(0);

                    if (tokens[0].Text == ",")
                        tokens.RemoveAt(0);
                }

                tokens.RemoveAt(0);
            }
            else
                tokens.RemoveAt(0);

            if (tokens[0].Text == ",")
                tokens.RemoveAt(0);
        }


        private bool HandleLiteralFunctionParam(string token)
        {
            if (token == "0")
                return true;

            if (token == "0.0")
                return true;

            if (token.Contains('.'))
            {
                float ignored;
                if (float.TryParse(token, out ignored))
                    return true;
            }
            else
            {
                int ignored;
                if (int.TryParse(token, out ignored))
                    return true;
            }

            return false;
        }

        private bool ParseInitialization(List<Token> tokens, string filename, bool isfuncreturn, string functionname)
        {
            if (tokens.Count < 2)
                return false;

            if (tokens[1].Text == ".")
                return false;

            bool templated = false;
            int skipahead = 0;
            if (tokens[1].Text == "<")
            {
                skipahead = ScanTemplateArgs(tokens, 2);
                templated = true;
            }

            var v = new Variable { Name = tokens[1 + skipahead].Text, Type = tokens[0].Text };      // TODO - include template parameters
            if ((filename != null) && (functionname == null))
            {
                GetOrCreateGlobalScope().Variables.Add(v);
            }
            else
            {
                if ((filename != null) && (functionname != null))
                {
                    if (ParsedScopeStack.Count > 0)
                        ParsedScopeStack.Peek().Add(v);
                }
            }

            if (tokens[2 + skipahead].Text != "=")
                return false;

            if (templated)
                ParseTemplateArgs(tokens, 2);

            if (isfuncreturn)
                GetFunctionSignature(filename, functionname).ReturnType = tokens[0].Text;

            tokens.RemoveRange(0, 3 + skipahead);

            ParseExpression(tokens);
            while (tokens.Count > 1 && tokens[0].Text == ",")
            {
                tokens.RemoveAt(0);
                ParseExpression(tokens);
            }

            return true;
        }

        private Scope GetOrCreateGlobalScope()
        {
            if (GlobalScope == null)
                GlobalScope = new Scope();

            return GlobalScope;
        }

        private bool ParseString(string filename, string code)
        {
            var tokens = Lex(code);
            try
            {
                while (tokens.Count > 0)
                {
                    if (ParseSumType(filename, tokens))
                    {
                    }
                    else if (ParseStrongAlias(filename, tokens))
                    {
                    }
                    else if (ParseWeakAlias(filename, tokens))
                    {
                    }
                    else if (ParseStructure(filename, tokens))
                    {
                    }
                    else if (ParseGlobalBlock(filename, tokens))
                    {
                    }
                    else if (ParseTask(filename, tokens))
                    {
                    }
                    else if (ParseFunction(filename, tokens))
                    {
                    }
                    else
                    {
                        throw new Exception("Syntax error");
                    }
                }
            }
            catch(Exception ex)
            {
                var errorTask = new ErrorTask();
                errorTask.Text = ex.Message;
                errorTask.Category = TaskCategory.CodeSense;
                errorTask.ErrorCategory = TaskErrorCategory.Error;
                errorTask.Document = filename;
                errorTask.Line = (tokens.Count > 0) ? (tokens[0].Line) : 0;
                errorTask.Column = (tokens.Count > 0) ? (tokens[0].Column) : 0;
                errorTask.Navigate += async (sender, args) =>
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

                    mgr.NavigateToLineAndColumn(buffer, ref logicalView, task.Line, task.Column, task.Line, task.Column);
                };

                ErrorProvider.Tasks.Add(errorTask);
                return false;
            }

            return true;
        }


        private bool ParseCodeBlock(List<Token> tokens, string filename, string functionname)
        {
            ParsedScopeStack.Push(new List<Variable>());
            Token startBrace = tokens[0];

            string token = tokens[0].Text;
            while (token != "}")
            {
                if (ParseEntity(tokens, filename, functionname))
                {
                }
                else if (ParsePreopStatement(tokens, false))
                {
                }
                else if (ParsePostopStatement(tokens))
                {
                }
                else if (ParseStatement(tokens, false))
                {
                }
                else if (ParseInitialization(tokens, filename, false, functionname))
                {
                }
                else if (ParseAssignment(tokens))
                {
                }
                else
                    return false;

                token = tokens[0].Text;

            }

            Token endBrace = tokens[0];
            tokens.RemoveAt(0);

            AddScope(filename, startBrace.Line, endBrace.Line, startBrace.Column, endBrace.Column);

            return true;
        }


        private bool ParseExpression(List<Token> tokens)
        {
            bool matchedstatement = false;

            if (!ParseExpressionTerm(tokens, true, out matchedstatement))
                return false;

            if (matchedstatement && tokens[0].Text == ")")
            {
            }
            else
            {
                while (ParseExpressionOperator(tokens))
                {
                    if (!ParseExpressionTerm(tokens, false, out matchedstatement))
                        return false;
                }
            }

            return true;
        }

        private bool ParseExpressionTerm(List<Token> tokens, bool startsexpr, out bool matchedstatement)
        {
            string term = tokens[0].Text;
            matchedstatement = false;

            if (term == ")")
                return false;

            if (term == ",")
                return false;

            if (term == "")
                return false;

            if (term == "(")
            {
                tokens.RemoveAt(0);
                if (ParseExpression(tokens))
                {
                    tokens.RemoveAt(0);
                    return true;
                }

                return false;
            }

            if (term == "!")
            {
                tokens.RemoveAt(0);
                return ParseExpressionTerm(tokens, startsexpr, out matchedstatement);
            }

            if (term == "false" || term == "true" || term == "0" || term == "0.0")
                tokens.RemoveAt(0);
            else if (ParsePreopStatement(tokens, true))
                return true;
            else if (ParseStatement(tokens, true))
            {
                matchedstatement = true;
                return true;
            }
            else
            {
                tokens.RemoveAt(0);
            }

            return true;
        }

        private bool ParseStatement(List<Token> tokens, bool substatement)
        {
            int lookahead = 0;

            if (tokens.Count < 2)
                return false;

            if (tokens[1].Text == "<")
            {
                lookahead = ParseTemplateArgs(tokens, 2);
            }

            if (tokens[1 + lookahead].Text != "(")
                return false;

            tokens.RemoveRange(0, 2 + lookahead);
            while (tokens[0].Text != ")")
            {
                if (!ParseExpression(tokens))
                    return false;

                if (tokens[0].Text == ",")
                    tokens.RemoveAt(0);
            }

            tokens.RemoveAt(0);
            return true;
        }

        private bool ParseExpressionOperator(List<Token> tokens)
        {
            if (tokens.Count <= 0)
                return false;

            string op = tokens[0].Text;
            if (op == ")")
                return false;

            if (op == ",")
                return false;

            if (op == "")
                return false;

            if (op.Length > 2)
                return false;

            bool knownoperator = false;
            if (op == ".")
                knownoperator = true;
            else if (op == "+")
                knownoperator = true;
            else if (op == "-")
                knownoperator = true;
            else if (op == "*")
                knownoperator = true;
            else if (op == "/")
                knownoperator = true;
            else if (op == "==")
                knownoperator = true;
            else if (op == "!=")
                knownoperator = true;
            else if (op == ";")
                knownoperator = true;
            else if (op == ">")
                knownoperator = true;
            else if (op == "<")
                knownoperator = true;
            else if (op == "&")
                knownoperator = true;
            else if (op == "&&")
                knownoperator = true;

            if (knownoperator)
            {
                tokens.RemoveAt(0);
                return true;
            }

            return false;
        }

        private bool ParsePreopStatement(List<Token> tokens, bool substatement)
        {
            bool recognized = false;
            string potential = tokens[0].Text;

            if (potential == "++")
                recognized = true;
            else if (potential == "--")
                recognized = true;

            if (recognized)
            {
                tokens.RemoveAt(0);
                tokens.RemoveAt(0);

                while (tokens[0].Text == ".")
                {
                    tokens.RemoveAt(0);
                    tokens.RemoveAt(0);
                }

                return true;
            }

            return false;
        }


        private bool ParseEntity(List<Token> tokens, string filename, string functionname)
        {
            string entityname = tokens[0].Text;
            if (entityname == "if")
            {
                if (tokens[1].Text == "(")
                {
                    tokens.RemoveRange(0, 2);

                    ParseExpression(tokens);
                    tokens.RemoveAt(0);

                    ParseEntityCode(tokens, filename, functionname);

                    while (tokens[0].Text == "elseif")
                    {
                        tokens.RemoveRange(0, 2);
                        ParseExpression(tokens);
                        tokens.RemoveAt(0);
                        ParseEntityCode(tokens, filename, functionname);
                    }

                    if (tokens[0].Text == "else")
                    {
                        tokens.RemoveAt(0);
                        ParseEntityCode(tokens, filename, functionname);
                    }

                    return true;
                }
            }
            else if (entityname == "while")
            {
                if (tokens[1].Text == "(")
                {
                    tokens.RemoveRange(0, 2);

                    ParseExpression(tokens);
                    tokens.RemoveAt(0);

                    return ParseEntityCode(tokens, filename, functionname);
                }
            }

            return false;
        }

        private bool ParsePostopStatement(List<Token> tokens)
        {
            bool recognized = false;

            string operand = tokens[0].Text;
            int operandlength = 1;
            while (tokens[operandlength].Text == ".")
                operandlength += 2;

            string potential = tokens[operandlength].Text;
            if (potential == "++")
                recognized = true;
            else if (potential == "--")
                recognized = true;

            if (recognized)
            {
                tokens.RemoveRange(0, operandlength + 1);

                return true;
            }

            return false;
        }


        private bool ParseAssignment(List<Token> tokens)
        {
            int lhslength = 1;
            while (tokens[lhslength].Text == ".")
                lhslength += 2;

            string assignmenttoken = tokens[lhslength].Text;
            bool recognized = false;

            if (assignmenttoken == "=")
                recognized = true;
            else if (assignmenttoken == "+=")
                recognized = true;
            else if (assignmenttoken == "-=")
                recognized = true;

            if (!recognized)
                return false;

            tokens.RemoveRange(0, lhslength + 1);

            bool haschain = true;
            while (haschain)
            {
                lhslength = 1;
                while (tokens[lhslength].Text == ".")
                    lhslength += 2;

                if (tokens[lhslength].Text == "=")
                {
                    if (assignmenttoken != "=")
                        return false;

                    tokens.RemoveRange(0, lhslength + 1);
                }
                else
                    haschain = false;
            }

            return ParseExpression(tokens);
        }

        private bool ParseEntityCode(List<Token> tokens, string filename, string functionname)
        {
            if (tokens[0].Text != "{")
                return false;

            tokens.RemoveAt(0);
            return ParseCodeBlock(tokens, filename, functionname);
        }


        private async System.Threading.Tasks.Task ParseFilesFromCurrentSolution(EnvDTE.DTE dte)
        {
            await ThreadHelper.JoinableTaskFactory.SwitchToMainThreadAsync();

            if (dte == null || dte.Solution == null)
                return;

            foreach (EnvDTE.Project project in dte.Solution.Projects)
            {
                ParseProjectItems(project.ProjectItems);
            }
        }

        private void ParseProjectItems(EnvDTE.ProjectItems items)
        {
            if (items == null)
                return;

            foreach (EnvDTE.ProjectItem item in items)
            {
                if (item.FileCount == 1 && item.FileNames[0].EndsWith(".epoch", StringComparison.OrdinalIgnoreCase))
                {
                    ParseFileOnDisk(item.FileNames[0]);
                }
                else
                {
                    ParseProjectItems(item.ProjectItems);
                }
            }
        }


        private void ParseFileOnDisk(string filename)
        {
            try
            {
                string text = System.IO.File.ReadAllText(filename);

                List<string> oldFunctionNames = null;
                List<string> oldStructures = null;
                List<string> oldTypes = null;

                Dictionary<string, FunctionDefinition> oldFunctionDefs = null;

                Dictionary<string, StructureDefinition> oldStructureDefinitions = null;

                if (ParsedFunctionNames == null)
                    ParsedFunctionNames = new Dictionary<string, List<string>>();

                if (ParsedStructures == null)
                    ParsedStructures = new Dictionary<string, List<string>>();

                if (ParsedTypes == null)
                    ParsedTypes = new Dictionary<string, List<string>>();

                if (FunctionDefinitions == null)
                    FunctionDefinitions = new Dictionary<string, Dictionary<string, FunctionDefinition>>();


                if (StructureDefinitions == null)
                    StructureDefinitions = new Dictionary<string, StructureDefinition>();

                ParsedScopeStack.Clear();

                if (!ParsedFunctionNames.ContainsKey(filename))
                    ParsedFunctionNames.Add(filename, new List<string>());
                else
                {
                    oldFunctionNames = ParsedFunctionNames[filename];
                    ParsedFunctionNames[filename].Clear();
                }

                if (!ParsedStructures.ContainsKey(filename))
                    ParsedStructures.Add(filename, new List<string>());
                else
                {
                    oldStructures = ParsedStructures[filename];
                    ParsedStructures[filename].Clear();
                }

                if (!ParsedTypes.ContainsKey(filename))
                    ParsedTypes.Add(filename, new List<string>());
                else
                {
                    oldTypes = ParsedTypes[filename];
                    ParsedTypes[filename].Clear();
                }

                if (!FunctionDefinitions.ContainsKey(filename))
                    FunctionDefinitions.Add(filename, new Dictionary<string, FunctionDefinition>());
                else
                {
                    oldFunctionDefs = FunctionDefinitions[filename];
                    FunctionDefinitions[filename].Clear();
                }

                // TODO - get off the UI thread here?
                if (!ParseString(filename, text))
                {
                    if (oldFunctionNames != null)
                        ParsedFunctionNames[filename] = oldFunctionNames;

                    if (oldStructures != null)
                        ParsedStructures[filename] = oldStructures;

                    if (oldStructureDefinitions != null)
                        StructureDefinitions = oldStructureDefinitions;

                    if (oldFunctionDefs != null)
                        FunctionDefinitions[filename] = oldFunctionDefs;
                }

            }
            catch
            {
            }
        }

        private async System.Threading.Tasks.Task ParseFileFromTextBuffer(ITextBuffer textBuffer)
        {
            IVsTextBuffer bufferAdapter;
            textBuffer.Properties.TryGetProperty(typeof(IVsTextBuffer), out bufferAdapter);
            if (bufferAdapter != null)
            {
                await ThreadHelper.JoinableTaskFactory.SwitchToMainThreadAsync();
                var persistFileFormat = bufferAdapter as IPersistFileFormat;
                string filename = null;
                uint formatIndex;
                if (persistFileFormat != null)
                    persistFileFormat.GetCurFile(out filename, out formatIndex);

                if (filename != null)
                    ParseFileOnDisk(filename);
                else
                {
                    // TODO - this will slowly corrupt/overfill the completion hint dictionaries
                    // We should probably stop stuffing all this logic in static functions and
                    // write a proper wrapper around it.
                    ParseString("@@UnsavedFiles", textBuffer.CurrentSnapshot.GetText());
                }
            }
        }


        public void ParseTextBuffer(ITextBuffer textBuffer)
        {
            // TODO - cache this better?

            var task = ParseFileFromTextBuffer(textBuffer);
        }

        public void ParseProject(EnvDTE.DTE dte)
        {
            GlobalScope = null;

            var task = ParseFilesFromCurrentSolution(dte);
        }

        public void GetAvailableFunctionNames(List<string> functionNames)
        {
            if (ParsedFunctionNames == null)
                return;

            foreach(var kvp in ParsedFunctionNames)
                functionNames.AddRange(kvp.Value);
        }

        public void GetAvailableStructureNames(List<string> structureNames)
        {
            if (ParsedStructures == null)
                return;

            foreach (var kvp in ParsedStructures)
                structureNames.AddRange(kvp.Value);
        }

        public void GetAvailableTypeNames(List<string> typeNames)
        {
            if (ParsedTypes == null)
                return;

            foreach (var kvp in ParsedTypes)
                typeNames.AddRange(kvp.Value);
        }

        public void GetAvailableFunctionSignatures(List<FunctionDefinition> functions)
        {
            if (FunctionDefinitions == null)
                return;

            foreach (var filekvp in FunctionDefinitions)
            {
                foreach (var funckvp in filekvp.Value)
                    functions.Add(funckvp.Value);
            }
        }

        internal void GetAvailableVariables(List<Variable> variables, string document, int line, int column)
        {
            foreach (var scope in ParsedScopes)
            {
                if (scope.Key.Equals(document))
                {
                    foreach (var s in scope.Value)
                    {
                        if (s.StartLine > line)
                            continue;

                        if (s.EndLine < line)
                            continue;

                        if ((s.StartLine == line) && (s.StartColumn > column))
                            continue;

                        if ((s.EndLine == line) && (s.EndColumn < column))
                            continue;

                        variables.AddRange(s.Variables);
                    }
                }
            }


            if (GlobalScope == null)
                return;

            variables.AddRange(GlobalScope.Variables);
        }

        public void Reset()
        {
            GlobalScope = null;
            ParsedFunctionNames = null;
            ParsedStructures = null;
            ParsedTypes = null;
            FunctionDefinitions = null;
            StructureDefinitions = null;
            ParsedScopeStack = new Stack<List<Variable>>();
        }

        private void AddScope(string filename, int beginline, int endline, int begincol, int endcol)
        {
            if (ParsedScopes == null)
                ParsedScopes = new Dictionary<string, List<Scope>>();

            if (!ParsedScopes.ContainsKey(filename))
                ParsedScopes.Add(filename, new List<Scope>());

            var s = new Scope { StartLine = beginline, StartColumn = begincol, EndLine = endline, EndColumn = endcol };
            s.Variables.AddRange(ParsedScopeStack.Pop());
            ParsedScopes[filename].Add(s);
        }

        internal StructureDefinition GetStructureDefinition(string name)
        {
            if (StructureDefinitions == null || !StructureDefinitions.ContainsKey(name))
                return null;

            return StructureDefinitions[name];
        }
    }
}

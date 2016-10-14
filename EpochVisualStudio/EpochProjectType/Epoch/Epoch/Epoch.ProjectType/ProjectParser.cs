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

namespace Epoch.ProjectParser
{
    public class ProjectParser
    {
        private static Dictionary<string, List<string>> ParsedFunctionNames = null;
        private static Dictionary<string, List<string>> ParsedStructures = null;

        private static Dictionary<string, Dictionary<string, FunctionDefinition>> FunctionDefinitions = null;
        private static Dictionary<string, StructureDefinition> StructureDefinitions = null;

        private class StructureDefinition
        {
            public List<StructureMember> Members = null;
        };

        public class FunctionDefinition
        {
            public string FunctionName;

            public List<FunctionParameter> Parameters = null;
            public string ReturnType;

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

        private class StructureMember
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

        private static CharacterClass LexerClassify(char c, CharacterClass currentclass)
        {
            if ("abcdef0123456789".Contains(c))
            {
                if (currentclass == CharacterClass.Literal)
                    return CharacterClass.Literal;

                return CharacterClass.Identifier;
            }

            if (c == 'x')
            {
                if (currentclass == CharacterClass.Literal)
                    return CharacterClass.Literal;

                return CharacterClass.Identifier;
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

        private static bool IsValidPunctuation(string token)
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


        private static List<string> Lex(string code)
        {
            List<string> tokens = new List<string>();

            int index = 0;
            int lasttokenstart = 0;

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
                        tokens.Add(code.Substring(lasttokenstart, index - lasttokenstart));
                }
                else if (state == CharacterClass.Punctuation)
                {
                    if (char.IsWhiteSpace(c))
                        state = CharacterClass.White;
                    else if (LexerClassify(c, state) != CharacterClass.Punctuation)
                        state = LexerClassify(c, state);

                    tokens.Add(code.Substring(lasttokenstart, index - lasttokenstart));
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
                                tokens.Add(potentialtoken.Substring(0, potentialtoken.Length - 1));
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
                                tokens.Add(potentialtoken.Substring(0, potentialtoken.Length - 1));
                                lasttokenstart = index - 1;
                            }
                        }

                        tokens.Add(code.Substring(lasttokenstart, index - lasttokenstart));
                    }
                }
                else if (state == CharacterClass.Comment)
                {
                    if (c == '\r')
                        state = CharacterClass.White;
                    else if (c == '\n')
                        state = CharacterClass.White;
                }
                else if (state == CharacterClass.StringLiteral)
                {
                    if (c == '\"')
                    {
                        state = CharacterClass.White;
                        tokens.Add(code.Substring(lasttokenstart, index - lasttokenstart + 1));
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
                        tokens.Add(code.Substring(lasttokenstart, index - lasttokenstart));
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
            }

            if ((lasttokenstart < code.Length) && (state != CharacterClass.White))
                tokens.Add(code.Substring(lasttokenstart, code.Length - lasttokenstart));

            return tokens;
        }

        private static bool ParseTemplateParameters(List<string> tokens)
        {
            bool hasparams = true;

            while (hasparams)
            {
                tokens.RemoveRange(0, 2);

                if (tokens[0] == ">")
                    hasparams = false;
                else if (tokens[0] == ",")
                    tokens.RemoveAt(0);
                else
                    return false;
            }

            return true;
        }

        private static int ScanTemplateArgs(List<string> tokens, int initialconsume)
        {
            int consumed = initialconsume;
            bool hasargs = true;
            while (hasargs)
            {
                ++consumed;

                if (tokens[consumed] == ">")
                    hasargs = false;
                else if (tokens[consumed] == ",")
                    ++consumed;
                else
                    return 0;
            }

            return consumed;
        }

        private static int ParseTemplateArgs(List<string> tokens, int initialconsume)
        {
            int consumed = initialconsume;

            bool hasargs = true;
            while (hasargs)
            {
                ++consumed;

                if (tokens[consumed] == ">")
                    hasargs = false;
                else if (tokens[consumed] == ",")
                    ++consumed;
                else
                    return 0;

            }

            return consumed;
        }

        private static void ParseSumTypeBases(List<string> tokens)
        {
            bool hasbases = true;
            do
            {
                if (tokens[1] == "<")
                {
                    int baselookahead = ParseTemplateArgs(tokens, 2);
                    tokens.RemoveRange(0, baselookahead + 2);
                }
                else
                {
                    tokens.RemoveRange(0, 2);
                }

                hasbases = (tokens[1] == "|");
            } while (hasbases);
        }

        private static bool ParseSumType(string filename, List<string> tokens)
        {
            if (tokens[0] != "type")
                return false;

            string sumtypename = tokens[1];
            if (tokens[2] == "<")
            {
                tokens.RemoveRange(0, 3);
                if (!ParseTemplateParameters(tokens))
                    return false;

                if (tokens[1] != ":")
                    return false;
            }
            else if (tokens[2] != ":")
                return false;
            else if (tokens[4] != "|")
                return false;
            else
                tokens.RemoveAt(0);

            tokens.RemoveRange(0, 2);

            // TODO - separate list of sum types/aliases instead?
            ParsedStructures[filename].Add(sumtypename);

            ParseSumTypeBases(tokens);

            tokens.RemoveAt(0);

            return true;
        }

        private static bool ParseStrongAlias(string filename, List<string> tokens)
        {
            if (tokens[0] != "type")
                return false;

            if (tokens[2] != ":")
                return false;

            tokens.RemoveAt(0);

            // TODO - separate list of sum types/aliases instead?
            ParsedStructures[filename].Add(tokens[0]);

            tokens.RemoveRange(0, 3);

            return true;
        }

        private static bool ParseWeakAlias(string filename, List<string> tokens)
        {
            if (tokens[0] != "alias")
                return false;

            if (tokens[2] != "=")
                return false;

            tokens.RemoveAt(0);

            // TODO - separate list of sum types/aliases instead?
            ParsedStructures[filename].Add(tokens[0]);

            tokens.RemoveRange(0, 3);
            return true;
        }

        private static bool ParseStructure(string filename, List<string> tokens)
        {
            bool templated = false;

            if (tokens[0] != "structure")
                return false;

            string structurename = tokens[1];

            if (tokens[2] == "<")
            {
                tokens.RemoveRange(0, 3);
                if (!ParseTemplateParameters(tokens))
                    return false;

                templated = true;

                if (tokens[1] == ":")
                    return false;
            }
            else if (tokens[2] != ":")
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
                if (tokens[0] == "(")
                {
                    tokens.RemoveAt(0);

                    // TODO - members?
                    tokens.RemoveAt(0);

                    if (tokens[0] != ":")
                        return false;

                    tokens.RemoveAt(0);

                    bool moreparams = true;
                    while (moreparams)
                    {
                        tokens.RemoveAt(0);
                        if (tokens[0] != ",")
                        {
                            moreparams = false;
                        }
                        else
                            tokens.RemoveAt(0);
                    }

                    if (tokens[0] == "->")
                    {
                        tokens.RemoveAt(0);
                        tokens.RemoveAt(0);
                    }

                    if (tokens[0] != ")")
                    {
                        return false;
                    }

                    tokens.RemoveAt(0);
                }
                else
                {
                    bool isref = false;

                    string membertype = tokens[0];

                    tokens.RemoveAt(0);

                    string membername = tokens[0];

                    tokens.RemoveAt(0);

                    if (membername == "<")
                    {
                        int memberlookahead = ParseTemplateArgs(tokens, 0);
                        if (memberlookahead > 0)
                        {
                            tokens.RemoveRange(0, memberlookahead + 1);
                            membername = tokens[0];
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
                        membername = tokens[0];
                        tokens.RemoveAt(0);
                    }

                    if (!StructureDefinitions.ContainsKey(structurename))
                    {
                        StructureDefinitions.Add(structurename, new StructureDefinition());
                        StructureDefinitions[structurename].Members = new List<StructureMember>();
                    }

                    var member = new StructureMember();
                    member.Name = membername;
                    member.Type = membertype;

                    if (isref)
                        member.Type += " ref";

                    StructureDefinitions[structurename].Members.Add(member);
                }

                if (tokens[0] != ",")
                    moremembers = false;
                else
                    tokens.RemoveAt(0);
            }

            return true;
        }

        private static bool ParseGlobalBlock(string filename, List<string> tokens)
        {
            // TODO
            return false;
        }

        private static bool ParseTask(string filename, List<string> tokens)
        {
            // TODO
            return false;
        }

        private static bool ParseFunction(string filename, List<string> tokens)
        {
            if (tokens.Count < 2)
                return false;

            string functionname = tokens[0];

            if (tokens[1] == "<")
            {
                tokens.RemoveRange(0, 2);
                if (!ParseTemplateParameters(tokens))
                    return false;
            }

            if (tokens[1] != ":")
                return false;

            tokens.RemoveRange(0, 2);

            ParsedFunctionNames[filename].Add(functionname);

            if (tokens[0] != "[")
            {
                ParseFunctionParams(tokens, filename, functionname);
                ParseFunctionReturn(tokens, filename, functionname);
            }
            ParseFunctionTags(tokens);


            if (tokens.Count <= 0 || tokens[0] != "{")
                return true;

            tokens.RemoveAt(0);

            ParseCodeBlock(tokens);

            return true;
        }

        private static void ParseFunctionParams(List<string> tokens, string filename, string functionname)
        {
            if (tokens.Count == 0)
                return;

            var sig = GetOrCreateFunctionSignature(filename, functionname);

            while ((tokens[0] != "{") && (tokens[0] != "->"))
            {
                if (tokens[0] == "nothing")
                {
                    var nothingparam = new FunctionParameter();
                    nothingparam.Name = "";
                    nothingparam.Type = "nothing";
                    sig.Parameters.Add(nothingparam);

                    tokens.RemoveAt(0);
                }
                else if (tokens[0] == "(")
                {
                    tokens.RemoveAt(0);

                    // TODO - better representation of higher order functions
                    var funcparam = new FunctionParameter();
                    funcparam.Name = "";
                    funcparam.Type = "(" + tokens[0] + " : -> )";
                    sig.Parameters.Add(funcparam);

                    if (tokens[1] == ":")
                    {
                        tokens.RemoveRange(0, 2);

                        bool moreparams = true;
                        if (tokens[0] == ")")
                            moreparams = false;

                        while (moreparams)
                        {
                            tokens.RemoveAt(0);
                            if (tokens[0] == "ref")
                                tokens.RemoveAt(0);

                            if (tokens[0] != ",")
                                moreparams = false;
                            else
                                tokens.RemoveAt(0);
                        }

                        if (tokens[0] == "->")
                            tokens.RemoveRange(0, 2);

                        if (tokens[0] != ")")
                            return;
                    }

                    tokens.RemoveAt(0);
                }
                else if (HandleLiteralFunctionParam(tokens[0]))
                {
                    var literalparam = new FunctionParameter();
                    literalparam.Name = "";
                    literalparam.Type = tokens[0];
                    sig.Parameters.Add(literalparam);

                    tokens.RemoveAt(0);
                }
                else
                {
                    int lookahead = 0;
                    if (tokens[1] == "<")
                        lookahead = ParseTemplateArgs(tokens, 2);

                    string typetoken = string.Join(" ", tokens.GetRange(0, 1 + lookahead));
                    string nametoken = tokens[1 + lookahead];

                    if (nametoken == "ref")
                    {
                        tokens.RemoveAt(0);
                        nametoken = tokens[1];

                        typetoken += " ref";
                    }

                    tokens.RemoveRange(0, 2 + lookahead);

                    var param = new FunctionParameter();
                    param.Type = typetoken;
                    param.Name = nametoken;
                    sig.Parameters.Add(param);
                }
            }

            if (tokens[0] != ",")
                return;

            tokens.RemoveAt(0);
        }

        private static void ParseFunctionReturn(List<string> tokens, string filename, string functionname)
        {
            if (tokens[0] != "->")
                return;

            tokens.RemoveAt(0);

            if (!ParseInitialization(tokens, filename, true, functionname))
            {
                ParseExpression(tokens);
                GetOrCreateFunctionSignature(filename, functionname).ReturnType = "<expr>";
            }
        }

        private static FunctionDefinition GetOrCreateFunctionSignature(string filename, string functionname)
        {
            if (FunctionDefinitions == null)
                FunctionDefinitions = new Dictionary<string, Dictionary<string, FunctionDefinition>>();

            if (!FunctionDefinitions.ContainsKey(filename))
                FunctionDefinitions.Add(filename, new Dictionary<string, FunctionDefinition>());

            var defs = FunctionDefinitions[filename];
            if (!defs.ContainsKey(functionname))
            {
                var newfunc = new FunctionDefinition();
                newfunc.FunctionName = functionname;
                newfunc.Parameters = new List<FunctionParameter>();
                newfunc.ReturnType = null;

                defs.Add(functionname, newfunc);
                return newfunc;
            }

            return defs[functionname];
        }

        private static void ParseFunctionTags(List<string> tokens)
        {
            if (tokens.Count <= 0)
                return;

            if (tokens[0] != "[")
                return;

            tokens.RemoveAt(0);

            while (tokens[0] != "]")
                ParseSingleFunctionTag(tokens);

            tokens.RemoveAt(0);
        }

        private static void ParseSingleFunctionTag(List<string> tokens)
        {
            if (tokens[1] == "(")
            {
                tokens.RemoveRange(0, 2);

                while (tokens[0] != ")")
                {
                    tokens.RemoveAt(0);

                    if (tokens[0] == ",")
                        tokens.RemoveAt(0);
                }

                tokens.RemoveAt(0);
            }
            else
                tokens.RemoveAt(0);

            if (tokens[0] == ",")
                tokens.RemoveAt(0);
        }


        private static bool HandleLiteralFunctionParam(string token)
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

        private static bool ParseInitialization(List<string> tokens, string filename, bool isfuncreturn, string functionname)
        {
            if (tokens.Count < 2)
                return false;

            if (tokens[1] == ".")
                return false;

            bool templated = false;
            int skipahead = 0;
            if (tokens[1] == "<")
            {
                skipahead = ScanTemplateArgs(tokens, 2);
                templated = true;
            }

            if (tokens[2 + skipahead] != "=")
                return false;

            if (templated)
                ParseTemplateArgs(tokens, 2);

            if (isfuncreturn)
                GetOrCreateFunctionSignature(filename, functionname).ReturnType = tokens[0];

            tokens.RemoveRange(0, 3 + skipahead);

            ParseExpression(tokens);
            while (tokens[0] == ",")
            {
                tokens.RemoveAt(0);
                ParseExpression(tokens);
            }

            return true;
        }

        private static bool ParseString(string filename, string code)
        {
            try
            {
                List<string> tokens = Lex(code);
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
                        return false;
                    }
                }
            }
            catch
            {
                return false;
            }

            return true;
        }


        private static bool ParseCodeBlock(List<string> tokens)
        {
            string token = tokens[0];
            while (token != "}")
            {
                if (ParseEntity(tokens))
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
                else if (ParseInitialization(tokens, null, false, null))
                {
                }
                else if (ParseAssignment(tokens))
                {
                }
                else
                    return false;

                token = tokens[0];

            }

            tokens.RemoveAt(0);

            return true;
        }


        private static bool ParseExpression(List<string> tokens)
        {
            bool matchedstatement = false;

            if (!ParseExpressionTerm(tokens, true, out matchedstatement))
                return false;

            if (matchedstatement && tokens[0] == ")")
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

        private static bool ParseExpressionTerm(List<string> tokens, bool startsexpr, out bool matchedstatement)
        {
            string term = tokens[0];
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

        private static bool ParseStatement(List<string> tokens, bool substatement)
        {
            int lookahead = 0;

            if (tokens.Count < 2)
                return false;

            if (tokens[1] == "<")
            {
                lookahead = ParseTemplateArgs(tokens, 2);
            }

            if (tokens[1 + lookahead] != "(")
                return false;

            tokens.RemoveRange(0, 2 + lookahead);
            while (tokens[0] != ")")
            {
                if (!ParseExpression(tokens))
                    return false;

                if (tokens[0] == ",")
                    tokens.RemoveAt(0);
            }

            tokens.RemoveAt(0);
            return true;
        }

        private static bool ParseExpressionOperator(List<string> tokens)
        {
            if (tokens.Count <= 0)
                return false;

            string op = tokens[0];
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

        private static bool ParsePreopStatement(List<string> tokens, bool substatement)
        {
            bool recognized = false;
            string potential = tokens[0];

            if (potential == "++")
                recognized = true;
            else if (potential == "--")
                recognized = true;

            if (recognized)
            {
                tokens.RemoveAt(0);
                tokens.RemoveAt(0);

                while (tokens[0] == ".")
                {
                    tokens.RemoveAt(0);
                    tokens.RemoveAt(0);
                }

                return true;
            }

            return false;
        }


        private static bool ParseEntity(List<string> tokens)
        {
            string entityname = tokens[0];
            if (entityname == "if")
            {
                if (tokens[1] == "(")
                {
                    tokens.RemoveRange(0, 2);

                    ParseExpression(tokens);
                    tokens.RemoveAt(0);

                    ParseEntityCode(tokens);

                    while (tokens[0] == "elseif")
                    {
                        tokens.RemoveRange(0, 2);
                        ParseExpression(tokens);
                        tokens.RemoveAt(0);
                        ParseEntityCode(tokens);
                    }

                    if (tokens[0] == "else")
                    {
                        tokens.RemoveAt(0);
                        ParseEntityCode(tokens);
                    }

                    return true;
                }
            }
            else if (entityname == "while")
            {
                if (tokens[1] == "(")
                {
                    tokens.RemoveRange(0, 2);

                    ParseExpression(tokens);
                    tokens.RemoveAt(0);

                    return ParseEntityCode(tokens);
                }
            }

            return false;
        }

        private static bool ParsePostopStatement(List<string> tokens)
        {
            bool recognized = false;

            string operand = tokens[0];
            int operandlength = 1;
            while (tokens[operandlength] == ".")
                operandlength += 2;

            string potential = tokens[operandlength];
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


        private static bool ParseAssignment(List<string> tokens)
        {
            int lhslength = 1;
            while (tokens[lhslength] == ".")
                lhslength += 2;

            string assignmenttoken = tokens[lhslength];
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
                while (tokens[lhslength] == ".")
                    lhslength += 2;

                if (tokens[lhslength] == "=")
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

        private static bool ParseEntityCode(List<string> tokens)
        {
            if (tokens[0] != "{")
                return false;

            tokens.RemoveAt(0);
            return ParseCodeBlock(tokens);
        }


        private async static System.Threading.Tasks.Task ParseFilesFromCurrentSolution(EnvDTE.DTE dte)
        {
            await ThreadHelper.JoinableTaskFactory.SwitchToMainThreadAsync();

            if (dte == null || dte.Solution == null)
                return;

            foreach (EnvDTE.Project project in dte.Solution.Projects)
            {
                foreach (EnvDTE.ProjectItem item in project.ProjectItems)
                {
                    if (item.FileCount == 1 && item.FileNames[0].EndsWith(".epoch", StringComparison.OrdinalIgnoreCase))
                    {
                        ParseFileOnDisk(item.FileNames[0]);
                    }
                }
            }
        }


        private static void ParseFileOnDisk(string filename)
        {
            try
            {
                string text = System.IO.File.ReadAllText(filename);

                List<string> oldFunctionNames = null;
                List<string> oldStructures = null;

                Dictionary<string, FunctionDefinition> oldFunctionDefs = null;

                Dictionary<string, StructureDefinition> oldStructureDefinitions = null;


                if (ParsedFunctionNames == null)
                    ParsedFunctionNames = new Dictionary<string, List<string>>();

                if (ParsedStructures == null)
                    ParsedStructures = new Dictionary<string, List<string>>();

                if (FunctionDefinitions == null)
                    FunctionDefinitions = new Dictionary<string, Dictionary<string, FunctionDefinition>>();


                if (StructureDefinitions == null)
                    StructureDefinitions = new Dictionary<string, StructureDefinition>();
                else
                    StructureDefinitions.Clear();


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

                if (!FunctionDefinitions.ContainsKey(filename))
                    FunctionDefinitions.Add(filename, new Dictionary<string, FunctionDefinition>());
                else
                {
                    oldFunctionDefs = FunctionDefinitions[filename];
                    FunctionDefinitions[filename].Clear();
                }

                // TODO - get off the UI thread here
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

        private async static System.Threading.Tasks.Task ParseFileFromTextBuffer(ITextBuffer textBuffer)
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


        public static void ParseTextBuffer(ITextBuffer textBuffer)
        {
            // TODO - cache this better?

            var task = ParseFileFromTextBuffer(textBuffer);
        }

        public static void ParseProject(EnvDTE.DTE dte)
        {
            var task = ParseFilesFromCurrentSolution(dte);
        }

        public static void GetAvailableFunctionNames(List<string> functionNames)
        {
            if (ParsedFunctionNames == null)
                return;

            foreach(var kvp in ParsedFunctionNames)
                functionNames.AddRange(kvp.Value);
        }

        public static void GetAvailableStructureNames(List<string> structureNames)
        {
            if (ParsedStructures == null)
                return;

            foreach (var kvp in ParsedStructures)
                structureNames.AddRange(kvp.Value);
        }

        public static void GetAvailableFunctionSignatures(List<FunctionDefinition> functions)
        {
            if (FunctionDefinitions == null)
                return;

            foreach (var filekvp in FunctionDefinitions)
            {
                foreach (var funckvp in filekvp.Value)
                    functions.Add(funckvp.Value);
            }
        }
    }
}

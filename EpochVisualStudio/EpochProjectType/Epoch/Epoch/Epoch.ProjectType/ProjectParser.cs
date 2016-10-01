﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.VisualStudio.Text;


namespace Epoch.ProjectParser
{
    public class ProjectParser
    {
        private static List<string> ParsedFunctionNames = null;


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

        private static bool ParseSumType(List<string> tokens)
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

            // TODO - intellisense hook for types

            ParseSumTypeBases(tokens);

            tokens.RemoveAt(0);

            return true;
        }

        private static bool ParseStrongAlias(List<string> tokens)
        {
            if (tokens[0] != "type")
                return false;

            if (tokens[2] != ":")
                return false;

            tokens.RemoveAt(0);

            // TODO - intellisense hook for types

            tokens.RemoveRange(0, 3);

            return true;
        }

        private static bool ParseWeakAlias(List<string> tokens)
        {
            if (tokens[0] != "alias")
                return false;

            if (tokens[2] != "=")
                return false;

            tokens.RemoveAt(0);

            // TODO - intellisense hook for types

            tokens.RemoveRange(0, 3);
            return true;
        }

        private static bool ParseStructure(List<string> tokens)
        {
            // TODO
            return false;
        }

        private static bool ParseGlobalBlock(List<string> tokens)
        {
            // TODO
            return false;
        }

        private static bool ParseTask(List<string> tokens)
        {
            // TODO
            return false;
        }

        private static bool ParseFunction(List<string> tokens)
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

            ParsedFunctionNames.Add(functionname);

            if (tokens[0] != "[")
            {
                ParseFunctionParams(tokens);
                ParseFunctionReturn(tokens);
            }
            ParseFunctionTags(tokens);


            if (tokens[0] != "{")
                return true;

            tokens.RemoveAt(0);

            ParseCodeBlock(tokens);

            return true;
        }

        private static void ParseFunctionParams(List<string> tokens)
        {
            if (tokens.Count == 0)
                return;

            while ((tokens[0] != "{") && (tokens[0] != "->"))
            {
                if (tokens[0] == "nothing")
                {
                    tokens.RemoveAt(0);
                }
                else if (tokens[0] == "(")
                {
                    tokens.RemoveAt(0);

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
                    tokens.RemoveAt(0);
                }
                else
                {
                    int lookahead = 0;
                    if (tokens[1] == "<")
                        lookahead = ParseTemplateArgs(tokens, 2);

                    string nametoken = tokens[1 + lookahead];
                    tokens.RemoveRange(0, 2 + lookahead);

                    if (nametoken == "ref")
                        tokens.RemoveAt(0);
                }
            }

            if (tokens[0] != ",")
                return;

            tokens.RemoveAt(0);
        }

        private static void ParseFunctionReturn(List<string> tokens)
        {
            if (tokens[0] != "->")
                return;

            tokens.RemoveAt(0);

            if (!ParseInitialization(tokens))
                ParseExpression(tokens);
        }

        private static void ParseFunctionTags(List<string> tokens)
        {
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

        private static bool ParseInitialization(List<string> tokens)
        {
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

            tokens.RemoveRange(0, 3 + skipahead);

            ParseExpression(tokens);
            while (tokens[0] == ",")
            {
                tokens.RemoveAt(0);
                ParseExpression(tokens);
            }

            return true;
        }

        private static bool ParseString(string code)
        {
            try
            {
                List<string> tokens = Lex(code);
                while (tokens.Count > 0)
                {
                    if (ParseSumType(tokens))
                    {
                    }
                    else if (ParseStrongAlias(tokens))
                    {
                    }
                    else if (ParseWeakAlias(tokens))
                    {
                    }
                    else if (ParseStructure(tokens))
                    {
                    }
                    else if (ParseGlobalBlock(tokens))
                    {
                    }
                    else if (ParseTask(tokens))
                    {
                    }
                    else if (ParseFunction(tokens))
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


        private static void ParseCodeBlock(List<string> tokens)
        {
            // TODO - parse code correctly
            while (tokens[0] != "}")
                tokens.RemoveAt(0);

            tokens.RemoveAt(0);
        }


        private static bool ParseExpression(List<string> tokens)
        {
            // TODO - parse expressions
            return false;
        }



        public static void ParseTextBuffer(ITextBuffer textBuffer)
        {
            // TODO - cache this
            // TODO - merge with results of the entire project

            var oldParsedFunctionNames = ParsedFunctionNames;

            ParsedFunctionNames = new List<string>();
            string code = textBuffer.CurrentSnapshot.GetText();

            if (!ParseString(code))
            {
                ParsedFunctionNames = oldParsedFunctionNames;
            }
        }

        public static void GetAvailableFunctionNames(List<string> functionNames)
        {
            if (ParsedFunctionNames == null)
                return;

            functionNames.AddRange(ParsedFunctionNames);
        }
    }
}
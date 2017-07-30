using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class FunctionSignature
    {
        public Token Name;

        public List<FunctionOverload> Overloads = new List<FunctionOverload>();



        private static bool IsLiteralFunctionParam(Token token)
        {
            if (token.Text == "0")
                return true;

            if (token.Text == "0.0")
                return true;

            if (token.Text.Contains('.'))
            {
                float ignored;
                if (float.TryParse(token.Text, out ignored))
                    return true;
            }
            else
            {
                int ignored;
                if (int.TryParse(token.Text, out ignored))
                    return true;
            }

            return false;
        }


        private static List<FunctionOverload.Parameter> ParseFunctionParams(ParseSession parser, int starttoken, out int consumedtokens)
        {
            var ret = new List<FunctionOverload.Parameter>();

            consumedtokens = starttoken;
            int totaltokens = starttoken;

            while (!parser.CheckToken(totaltokens, "{") && !parser.CheckToken(totaltokens, "->"))
            {
                if (parser.CheckToken(totaltokens, "nothing"))
                {
                    var signature = new TypeSignature();

                    ret.Add(new FunctionOverload.Parameter { Name = "nothing", Type = signature });
                    ++totaltokens;
                }
                else if (parser.CheckToken(totaltokens, "("))
                {
                    ++totaltokens;

                    var higherordername = parser.PeekToken(totaltokens);

                    if (parser.CheckToken(totaltokens + 1, ":"))
                    {
                        totaltokens += 2;

                        bool moreparams = true;
                        if (parser.CheckToken(totaltokens, ")"))
                            moreparams = false;

                        while (moreparams)
                        {
                            ++totaltokens;
                            if (parser.CheckToken(totaltokens, "ref"))
                                ++totaltokens;

                            if (!parser.CheckToken(totaltokens, ","))
                                break;

                            ++totaltokens;
                        }
                    }

                    if (parser.CheckToken(totaltokens, "->"))
                        totaltokens += 2;

                    if (!parser.CheckToken(totaltokens, ")"))
                        return null;

                    ++totaltokens;

                    var signature = new TypeSignature();        // TODO - implement higher order function signatures
                    ret.Add(new FunctionOverload.Parameter { Name = higherordername.Text, Type = signature });
                }
                else if (IsLiteralFunctionParam(parser.PeekToken(totaltokens)))
                {
                    // TODO - better literal support

                    var signature = new TypeSignature();
                    ret.Add(new FunctionOverload.Parameter { Name = parser.PeekToken(totaltokens).Text, Type = signature });

                    ++totaltokens;
                }
                else
                {
                    int begintoken = totaltokens;

                    if (parser.CheckToken(totaltokens + 1, "<"))
                    {
                        var paramtype = parser.PeekToken(totaltokens);
                        totaltokens += 2;
                        if (!parser.ParseTemplateArguments(totaltokens, paramtype, out totaltokens))
                            return null;
                    }
                    else
                        ++totaltokens;

                    var paramname = parser.PeekToken(totaltokens);
                    if (parser.CheckToken(totaltokens, "ref"))
                    {
                        ++totaltokens;
                        paramname = parser.PeekToken(totaltokens);
                    }

                    var signature = TypeSignature.Construct(parser, begintoken, totaltokens);
                    ret.Add(new FunctionOverload.Parameter { Name = paramname.Text, Type = signature });

                    ++totaltokens;
                }

                if (!parser.CheckToken(totaltokens, ","))
                {
                    consumedtokens = totaltokens;
                    return ret;
                }

                ++totaltokens;
            }

            return ret;
        }


        private static Variable ParseFunctionReturn(ParseSession parser, int starttoken, out int consumedtokens)
        {
            consumedtokens = starttoken;
            int totaltokens = starttoken;

            if (!parser.CheckToken(totaltokens, "->"))
                return null;

            ++totaltokens;

            var variable = Variable.Parse(parser, totaltokens, out totaltokens, Variable.Origins.Return);
            if (variable == null)
            {
                var expr = Expression.Parse(parser, totaltokens, out totaltokens);
                if (expr == null)
                    return null;

                // TODO - return something sane for expressions
                consumedtokens = totaltokens;
                return null;
            }

            consumedtokens = totaltokens;
            return variable;
        }


        private static List<FunctionOverload.Tag> ParseFunctionTags(ParseSession parser, int starttoken, out int consumedtokens)
        {
            consumedtokens = starttoken;
            int totaltokens = starttoken;

            if (!parser.CheckToken(totaltokens, "["))
                return null;

            ++totaltokens;

            var ret = new List<FunctionOverload.Tag>();

            while (!parser.CheckToken(totaltokens, "]"))
            {
                ret.Add(new FunctionOverload.Tag { Name = parser.PeekToken(totaltokens) });

                if (parser.CheckToken(totaltokens + 1, "("))
                {
                    // TODO - proper tag param handling
                    totaltokens += 2;

                    while (!parser.CheckToken(totaltokens, ")"))
                    {
                        ++totaltokens;

                        if (parser.CheckToken(totaltokens, ","))
                            ++totaltokens;
                    }
                }
                ++totaltokens;

                if (parser.CheckToken(totaltokens, ","))
                    ++totaltokens;
            }

            ++totaltokens;

            consumedtokens = totaltokens;
            return ret;
        }


        private static bool ParseEntity(ParseSession parser, int starttoken, out int consumedtokens)
        {
            consumedtokens = starttoken;
            int totaltokens = starttoken;

            if (parser.CheckToken(totaltokens, "if"))
            {
                if (!parser.CheckToken(totaltokens + 1, "("))
                    return false;

                totaltokens += 2;

                var expr = Expression.Parse(parser, totaltokens, out totaltokens);
                if (expr == null)
                    return false;

                while (parser.CheckToken(totaltokens, ")"))
                    ++totaltokens;

                if (!parser.CheckToken(totaltokens, "{"))
                    return false;

                ++totaltokens;
                ParseCodeBlock(parser, totaltokens, out totaltokens);

                while (parser.CheckToken(totaltokens, "elseif"))
                {
                    totaltokens += 2;
                    var condexpr = Expression.Parse(parser, totaltokens, out totaltokens);
                    if (condexpr == null)
                        return false;

                    while (parser.CheckToken(totaltokens, ")"))
                        ++totaltokens;

                    if (!parser.CheckToken(totaltokens, "{"))
                        return false;

                    ++totaltokens;
                    ParseCodeBlock(parser, totaltokens, out totaltokens);
                }

                if (parser.CheckToken(totaltokens, "else"))
                {
                    ++totaltokens;
                    if (!parser.CheckToken(totaltokens, "{"))
                        return false;

                    ++totaltokens;
                    ParseCodeBlock(parser, totaltokens, out totaltokens);
                }

                consumedtokens = totaltokens;
                return true;
            }
            else if (parser.CheckToken(totaltokens, "while"))
            {
                if (!parser.CheckToken(totaltokens + 1, "("))
                    return false;

                totaltokens += 2;

                while (parser.CheckToken(totaltokens, ")"))
                    ++totaltokens;

                var expr = Expression.Parse(parser, totaltokens, out totaltokens);
                ++totaltokens;

                if (!parser.CheckToken(totaltokens, "{"))
                    return false;

                ++totaltokens;
                ParseCodeBlock(parser, totaltokens, out totaltokens);

                consumedtokens = totaltokens;
                return true;
            }

            return false;
        }

        private static bool ParseAssignment(ParseSession parser, int starttoken, out int consumedtokens)
        {
            consumedtokens = starttoken;
            int totaltokens = starttoken;

            ++totaltokens;
            while (parser.CheckToken(totaltokens, "."))
                totaltokens += 2;

            if (!parser.CheckToken(totaltokens, "=")
                && !parser.CheckToken(totaltokens, "+=")
                && !parser.CheckToken(totaltokens, "-="))
            {
                return false;
            }

            var assignmenttoken = parser.PeekToken(totaltokens);
            ++totaltokens;
            
            bool haschain = true;
            while (haschain)
            {
                while (parser.CheckToken(totaltokens + 1, "."))
                    totaltokens += 2;

                if (parser.CheckToken(totaltokens + 1, "="))
                {
                    if (assignmenttoken.Text != "=")
                        return false;

                    ++totaltokens;
                }
                else
                    haschain = false;
            }

            var expr = Expression.Parse(parser, totaltokens, out totaltokens);
            if (expr == null)
                return false;

            consumedtokens = totaltokens;
            return true;
        }


        private static void ParseCodeBlock(ParseSession parser, int starttoken, out int consumedtokens)
        {
            consumedtokens = starttoken;
            int totaltokens = starttoken;
            Token startBrace = parser.PeekToken(totaltokens);

            while (!parser.CheckToken(totaltokens, "}"))
            {
                if (ParseEntity(parser, totaltokens, out totaltokens)
                    || parser.ParsePreopStatement(totaltokens, out totaltokens)
                    || parser.ParsePostopStatement(totaltokens, out totaltokens)
                    || parser.ParseStatement(totaltokens, out totaltokens))
                {
                    parser.ConsumeTokens(totaltokens);
                    totaltokens = 0;
                    continue;
                }

                var variable = Variable.Parse(parser, totaltokens, out totaltokens, Variable.Origins.Local);
                if (variable != null)
                {
                    // TODO - stash variable definition in lexical scope
                    parser.ConsumeTokens(totaltokens);
                    totaltokens = 0;
                    continue;
                }

                if (ParseAssignment(parser, totaltokens, out totaltokens))
                {
                    parser.ConsumeTokens(totaltokens);
                    totaltokens = 0;
                    continue;
                }

                consumedtokens = totaltokens;
                return;
            }

            Token endBrace = parser.PeekToken(totaltokens);

            // TODO - add lexical scope
            ++totaltokens;
            consumedtokens = totaltokens;
        }


        public static FunctionSignature Parse(ParseSession parser)
        {
            var nametoken = parser.PeekToken(0);
            if (nametoken == null || string.IsNullOrEmpty(nametoken.Text))
                return null;

            int totaltokens = 1;
            if (parser.CheckToken(totaltokens, "<"))
            {
                ++totaltokens;
                if (!parser.ParseTemplateParameters(totaltokens, nametoken, out totaltokens))
                    return null;
            }

            if (!parser.CheckToken(totaltokens, ":"))
                return null;

            ++totaltokens;

            var overload = new FunctionOverload();

            if (!parser.CheckToken(totaltokens, "["))
            {
                var paramlist = ParseFunctionParams(parser, totaltokens, out totaltokens);
                var funcreturn = ParseFunctionReturn(parser, totaltokens, out totaltokens);

                overload.Parameters = paramlist;
                overload.ReturnType = funcreturn?.Type;
            }

            var tags = ParseFunctionTags(parser, totaltokens, out totaltokens);
            overload.Tags = tags;

            if (parser.CheckToken(totaltokens, "{"))
            {
                ++totaltokens;
                ParseCodeBlock(parser, totaltokens, out totaltokens);
            }

            parser.ConsumeTokens(totaltokens);

            var ret = new FunctionSignature { Name = nametoken, Overloads = new List<FunctionOverload>() };
            ret.Overloads.Add(overload);

            return ret;
        }
    }

    class FunctionOverload
    {
        public class Parameter
        {
            public string Name;
            public TypeSignature Type;
        }

        public class Tag
        {
            public class TagParam
            {
                public List<Token> AssociatedTokens;
            }

            public Token Name;
            public List<TagParam> Params;
        }

        public List<Parameter> Parameters;
        public TypeSignatureInstantiated ReturnType;
        public List<Tag> Tags;
    }
}

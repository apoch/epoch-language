//
// The Epoch Language Project
// Visual Studio Integration/Extension
//
// Wrapper class for parsing a function signature from a token stream.
// Function signatures consist of the function's name, optional parameters,
// optional return variable/expression, and optional tags. Tags can have
// further parameters as necessary.
//

using System.Collections.Generic;
using System.Linq;

namespace EpochVSIX.Parser
{
    class FunctionSignature
    {
        public Token Name;

        public List<FunctionOverload> Overloads = new List<FunctionOverload>();



        public override string ToString()
        {
            return string.Join("\r\n\r\n", Overloads.Select(o => o.Format(Name.Text)).ToArray());
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
                    var signature = new TypeSignatureInstantiated();

                    ret.Add(new FunctionOverload.Parameter { Name = parser.PeekToken(totaltokens), Type = signature });
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

                    var signature = new TypeSignatureInstantiated();        // TODO - implement higher order function signatures
                    ret.Add(new FunctionOverload.Parameter { Name = higherordername, Type = signature });
                }
                else if (parser.PeekToken(totaltokens).IsLiteralFunctionParam())
                {
                    // TODO - better literal support

                    var signature = new TypeSignatureInstantiated();
                    ret.Add(new FunctionOverload.Parameter { Name = parser.PeekToken(totaltokens), Type = signature });

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

                    var signature = TypeSignatureInstantiated.Construct(parser, begintoken, totaltokens);
                    ret.Add(new FunctionOverload.Parameter { Name = paramname, Type = signature });

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
                var tag = new FunctionOverload.Tag { Name = parser.PeekToken(totaltokens) };

                if (parser.CheckToken(totaltokens + 1, "("))
                {
                    totaltokens += 2;

                    tag.Params = new List<FunctionOverload.Tag.TagParam>();
                    var p = new FunctionOverload.Tag.TagParam { AssociatedTokens = new List<Token>() };

                    while (!parser.CheckToken(totaltokens, ")"))
                    {
                        p.AssociatedTokens.Add(parser.PeekToken(totaltokens));
                        ++totaltokens;

                        if (parser.CheckToken(totaltokens, ","))
                        {
                            tag.Params.Add(p);
                            ++totaltokens;
                            p = new FunctionOverload.Tag.TagParam { AssociatedTokens = new List<Token>() };
                        }
                    }

                    tag.Params.Add(p);
                }
                ++totaltokens;

                ret.Add(tag);

                if (parser.CheckToken(totaltokens, ","))
                    ++totaltokens;
            }

            ++totaltokens;

            consumedtokens = totaltokens;
            return ret;
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
                overload.ReturnName = funcreturn?.Name?.Text;
                overload.ReturnType = funcreturn?.Type;
            }

            var tags = ParseFunctionTags(parser, totaltokens, out totaltokens);
            overload.Tags = tags;

            if (parser.CheckToken(totaltokens, "{"))
            {
                ++totaltokens;
                var scope = LexicalScope.Parse(parser, null, totaltokens, out totaltokens);
                overload.Scope = scope;

                if (overload.Scope != null && overload.Parameters != null)
                {
                    foreach (var p in overload.Parameters)
                    {
                        var v = new Variable { Name = p.Name, Type = p.Type, Origin = Variable.Origins.Parameter };
                        overload.Scope.Variables.Add(v);
                    }
                }
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
            public Token Name;
            public TypeSignatureInstantiated Type;

            public override string ToString()
            {
                if (Name.Text == "nothing")
                    return "nothing";

                return $"{Type} {Name}";
            }
        }

        public class Tag
        {
            public class TagParam
            {
                public List<Token> AssociatedTokens;

                public override string ToString()
                {
                    return string.Join(" ", AssociatedTokens.Select(t => t.Text));
                }
            }

            public Token Name;
            public List<TagParam> Params;

            public override string ToString()
            {
                string p = "";
                if (Params != null && Params.Count > 0)
                    p = $"({string.Join(", ", Params)})";

                return $"{Name.Text}{p}";
            }
        }

        public List<Parameter> Parameters;
        public TypeSignatureInstantiated ReturnType;
        public string ReturnName;
        public List<Tag> Tags;
        public LexicalScope Scope;

        public string Format(string basename)
        {
            string paramlist = Parameters == null ? "" : string.Join(", ", Parameters);
            string retname = ReturnName == null ? "" : $" {ReturnName}";
            string ret = ReturnType == null ? "" : $" -> {ReturnType}{retname}";
            string tags = Tags == null ? "" : $" [{string.Join(", ", Tags)}]";
            return $"{basename} : {paramlist}{ret}{tags}";
        }
    }
}

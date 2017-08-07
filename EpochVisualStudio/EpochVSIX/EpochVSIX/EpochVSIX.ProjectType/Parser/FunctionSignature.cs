//
// The Epoch Language Project
// Visual Studio Integration/Extension
//
// Wrapper class for parsing a function signature from a token stream.
// Function signatures consist of the function's name, optional parameters,
// optional return variable/expression, and optional tags. Tags can have
// further parameters as necessary.
//
// All functions, regardless of overloading, have exactly one signature.
// This signature is named by the (potentially overloaded) token used to
// identify the first instance of the function seen in code. For the sake
// of simplicity, all signatures have one or more overloads. If additional
// instances are seen, overloads are added to the appropriate signature.
//
// This setup is slightly more wasteful in terms of storage but drastically
// simplifies the lookup logic for IntelliSense purposes. Since our main
// objective here is to provide fast lookups, this trade-off is justified.
//

using System.Collections.Generic;
using System.Linq;

namespace EpochVSIX.Parser
{

    //
    // A signature completely describes a function definition and its overloads.
    //
    // All functions have exactly one signature and at least one overload. This
    // differs from the compiler implementation, as noted above, for the sake
    // of performance when doing IntelliSense lookups.
    //
    class FunctionSignature
    {
        //
        // The uniquely-identifying name of the function.
        //
        // Note that all overloads share the same name token, since
        // by definition they share a textually identical name also.
        //
        public Token Name;


        //
        // The list of recognized overloads for this function.
        //
        // There will always be at least one overload for a valid function,
        // as detailed in comments elsewhere.
        //
        public List<FunctionOverload> Overloads = new List<FunctionOverload>();


        //
        // Format a function signature as a string.
        //
        // Each overload is given a line to express its unique signature,
        // in the order of definition in the source program. Note that an
        // overloaded function with implementations in multiple files will
        // technically work fine, but the ordering may be volatile.
        //
        public override string ToString()
        {
            return string.Join("\r\n", Overloads.Select(o => o.Format(Name.Text)).ToArray());
        }


        //
        // Helper routine for parsing a set of function parameters from a token stream.
        //
        // Passes back the index of the next token to inspect, as well as a list of parameters
        // extracted from the token stream, if applicable. May return null in case of a syntax
        // error.
        //
        private static List<FunctionOverload.Parameter> ParseFunctionParams(ParseSession parser, int starttoken, out int consumedtokens)
        {
            var ret = new List<FunctionOverload.Parameter>();

            consumedtokens = starttoken;
            int totaltokens = starttoken;

            while (!parser.CheckToken(totaltokens, "{") && !parser.CheckToken(totaltokens, "->"))
            {
                //
                // Handle the "nothing" case since we don't need to parse any identifiers in addition
                // to the actual "nothing" keyword. This should come first to avoid backtracking in a
                // later parse step.
                //
                if (parser.CheckToken(totaltokens, "nothing"))
                {
                    var signature = new TypeSignatureInstantiated();

                    ret.Add(new FunctionOverload.Parameter { Name = parser.PeekToken(totaltokens), Type = signature });
                    ++totaltokens;
                }
                
                //
                // Handle higher-order functions.
                //
                else if (parser.CheckToken(totaltokens, "("))
                {
                    ++totaltokens;

                    var higherordername = parser.PeekToken(totaltokens);

                    // TODO - don't treat syntax elements as optional if they are technically required!
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

                //
                // Handle functions with literal parameters (for pattern matching).
                //
                else if (parser.PeekToken(totaltokens).IsLiteralFunctionParam())
                {
                    // TODO - better literal support

                    var signature = new TypeSignatureInstantiated();
                    ret.Add(new FunctionOverload.Parameter { Name = parser.PeekToken(totaltokens), Type = signature });

                    ++totaltokens;
                }

                //
                // Handle the general case of "type [ref] identifier[,]" format parameters.
                //
                // Also handles the presence of template arguments attached to the parameter
                // type, as necessary.
                //
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

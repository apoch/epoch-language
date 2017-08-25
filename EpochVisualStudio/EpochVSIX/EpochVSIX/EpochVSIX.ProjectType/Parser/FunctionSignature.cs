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
                else if (IsLiteralFunctionParam(parser.PeekToken(totaltokens)))
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

        //
        // Helper routine for checking if a function parameter is a literal.
        //
        // Literals are allowed for use in pattern matching calls.
        //
        private static bool IsLiteralFunctionParam(Token token)
        {
            if (token.Text.Contains("."))
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

        //
        // Helper routine for parsing a series of function tags from a token stream.
        //
        // Function tag lists are denoted by square brackets. Each tag is a single token
        // followed by an optional parenthesized list of parameters. Both sorts of lists
        // are delimited by commas. Parameters may be literal values only, including but
        // not limited to string literals. Expressions are not permitted.
        //
        // Passes back the index of the next token to inspect. Returns null if there are
        // no tags present. Presently does not handle syntax errors nicely.
        //
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

        //
        // Helper routine for parsing a function from a token stream.
        // This helper will also extract the code body of the function.
        //
        // Consumes tokens on success. Returns null if a function definition
        // is not matched. Success returns a function signature with exactly
        // one overload; callers are expected to merge this with any other
        // applicable overload lists.
        //
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
                overload.Parameters = ParseFunctionParams(parser, totaltokens, out totaltokens);
                overload.Return = FunctionReturn.Parse(parser, totaltokens, out totaltokens);
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


    //
    // Wrapper for representing the return of a function.
    //
    // In a different language I might express this using a union type
    // or sum type instead, but oh well. If ReturnVariable is set to a
    // non-null value, the function is assumed to return a named typed
    // variable. Otherwise, ReturnExpression might be set, which means
    // that the function returns a computed expression - a common case
    // for short helper functions, especially arithmetic operations.
    //
    // A null FunctionReturn OR a FunctionReturn with both members set
    // to null signals a function with no return, i.e. a procedure.
    //
    class FunctionReturn
    {
        public Variable ReturnVariable;
        public Expression ReturnExpression;


        //
        // Format a function return as a nice string
        //
        public override string ToString()
        {
            if (ReturnVariable != null)
                return ReturnVariable.ToStringNoComments();

            // Expressions currently do not store anything, so formatting
            // them as a string is kind of pointless. Instead we just use
            // a sort of placeholder string.
            if (ReturnExpression != null)
                return "<expression>";

            return "";
        }


        //
        // Helper routine for parsing a function return from a token stream.
        //
        // Passes back the index of the next token to examine. Returns null if
        // no function return is provided or if there is a syntax error.
        //
        internal static FunctionReturn Parse(ParseSession parser, int starttoken, out int consumedtokens)
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

                consumedtokens = totaltokens;
                return new FunctionReturn { ReturnExpression = expr };
            }

            consumedtokens = totaltokens;
            return new FunctionReturn { ReturnVariable = variable };
        }
    }

    //
    // Wrapper for describing a function overload.
    //
    // As noted above this wrapper will have at least one instance for every
    // function in the source code even if it is not technically overloaded. 
    //
    // There's not much logic here since really all the class needs to do is
    // hold some metadata and format the occasional string.
    //
    class FunctionOverload
    {
        //
        // Wrapper for representing a function parameter.
        //
        // Parameters are set up to be a name and a type signature.
        // Note the use of an "instantiated" type signature (i.e. any
        // templates involved are taking instance arguments).
        //
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

        //
        // Wrapper for representing a function tag.
        //
        // Tags are handled as opaque names plus optional parameters.
        // Parameters are currently stored as raw token sequences, not
        // actual parsed code.
        //
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

        //
        // Member data for a function overload
        //
        public List<Parameter> Parameters;
        public FunctionReturn Return;
        public List<Tag> Tags;
        public LexicalScope Scope;

        //
        // Helper for formatting an overload signature into a string.
        //
        // Note that we do not just use ToString() here, because technically
        // the overload's name is unutterable. Instead we take a "base name"
        // parameter which controls the name of the function in the rendered
        // text. This spares us the need to store the exact same name string
        // on every single overload.
        //
        public string Format(string basename)
        {
            string paramlist = Parameters == null ? "" : string.Join(", ", Parameters);
            string ret = Return == null ? "" : $" -> {Return}";
            string tags = Tags == null ? "" : $" [{string.Join(", ", Tags)}]";
            return $"{basename} : {paramlist}{ret}{tags}";
        }
    }
}

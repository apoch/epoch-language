//
// The Epoch Language Project
// Visual Studio Integration/Extension
//
// Miscellaneous helper routines for parsing code.
//
// The routines here are candidates for splitting out into helper classes
// like the existing wrappers for Expressions, Variable initializations,
// and so on. Mostly they are here to keep things simple; if the associated
// code constructs would someday benefit from tracking additional state,
// or more complex representations in the IntelliSense layer, the parsing
// can and should be factored out into dedicated classes.
//

namespace EpochVSIX.Parser
{
    class CodeHelpers
    {

        //
        // Helper routine for parsing an assignment statement.
        //
        // Returns true if an assignment was seen in the token stream, false
        // otherwise. A false return does not indicate a syntax error. Also
        // passes back the index of the next unexamined token.
        //
        // Assignments can have associated operations, such as "a += b".
        // They can also be "chained" as in "a = b = c". However these two
        // features may not be used in conjunction with one another.
        //
        internal static bool ParseAssignment(ParseSession parser, int starttoken, out int consumedtokens)
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
                        throw new SyntaxError("Chained assignments must all use the = assignment operator.", parser.PeekToken(totaltokens + 1));

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


        //
        // Helper routine for parsing a code entity from a token stream.
        //
        // This terminology is mostly legacy from an older implementation of Epoch.
        // An entity is really just a branch/flow-control construct such as a loop
        // or an if/else statement. Typically there is an expression which controls
        // execution and an associated code block.
        //
        internal static bool ParseEntity(ParseSession parser, LexicalScope parentscope, int starttoken, out int consumedtokens)
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
                    throw new SyntaxError("if() statement must be supplied with a valid Boolean-typed expression", parser.ReversePeekToken());

                while (parser.CheckToken(totaltokens, ")"))
                    ++totaltokens;

                if (!parser.CheckToken(totaltokens, "{"))
                    throw new SyntaxError("if() statement must be followed by a code block surrounded in braces { }", parser.PeekToken(totaltokens));

                ++totaltokens;
                LexicalScope.Parse(parser, parentscope, totaltokens, out totaltokens);

                while (parser.CheckToken(totaltokens, "elseif"))
                {
                    totaltokens += 2;
                    var condexpr = Expression.Parse(parser, totaltokens, out totaltokens);
                    if (condexpr == null)
                        throw new SyntaxError("elseif() statement must be supplied with a valid Boolean-typed expression", parser.ReversePeekToken());

                    while (parser.CheckToken(totaltokens, ")"))
                        ++totaltokens;

                    if (!parser.CheckToken(totaltokens, "{"))
                        throw new SyntaxError("elseif() statement must be followed by a code block surrounded in braces { }", parser.PeekToken(totaltokens));

                    ++totaltokens;
                    LexicalScope.Parse(parser, parentscope, totaltokens, out totaltokens);
                }

                if (parser.CheckToken(totaltokens, "else"))
                {
                    ++totaltokens;
                    if (!parser.CheckToken(totaltokens, "{"))
                        throw new SyntaxError("else statement must be followed by a code block surrounded in braces { }", parser.PeekToken(totaltokens));

                    ++totaltokens;
                    LexicalScope.Parse(parser, parentscope, totaltokens, out totaltokens);
                }

                consumedtokens = totaltokens;
                return true;
            }
            else if (parser.CheckToken(totaltokens, "while"))
            {
                if (!parser.CheckToken(totaltokens + 1, "("))
                    throw new SyntaxError("while() statement must be supplied with a valid Boolean-typed expression", parser.ReversePeekToken());

                totaltokens += 2;

                while (parser.CheckToken(totaltokens, ")"))
                    ++totaltokens;

                var expr = Expression.Parse(parser, totaltokens, out totaltokens);
                ++totaltokens;

                if (!parser.CheckToken(totaltokens, "{"))
                    throw new SyntaxError("while() statement must be followed by a code block surrounded in braces { }", parser.PeekToken(totaltokens));

                ++totaltokens;
                LexicalScope.Parse(parser, parentscope, totaltokens, out totaltokens);

                consumedtokens = totaltokens;
                return true;
            }

            return false;
        }
    }
}

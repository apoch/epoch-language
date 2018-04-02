//
// The Epoch Language Project
// Visual Studio Integration/Extension
//
// This class provides parsing for infix expressions in the Epoch syntax.
// Note that the actual expression is not really kept; primarily because
// it is not needed by the IntelliSense system, and secondarily because
// it would be a pain to maintain a parallel implementation of expressions
// in addition to that in the main language compiler. To minimize the odds
// of divergence, this module is strictly minimal and only does parsing
// and validation of expressions, not evaluation or manipulation. So don't
// be too surprised to discover no mention of operator precedences, for
// example! Also note that despite best intentions it is entirely likely
// that the compiler rejects slightly more expressions than this version
// of the code will, since we err on the side of simplicity and speed for
// interactive use rather than comprehensive correctness.
//

namespace EpochVSIX.Parser
{
    class Expression
    {


        //
        // Helper function to extract an operator from a token stream.
        //
        // Returns true if successful, false if no valid operator was
        // detected. Does not directly consume tokens; assume exactly
        // one token can be popped if true is returned.
        //
        // Note that this function will return false if a terminating
        // token (closing parens, etc.) is seen; this should not be
        // taken to indicate a syntax error.
        //
        private static bool ParseExpressionOperator(ParseSession parser, int starttoken)
        {
            var token = parser.PeekToken(starttoken);
            if (token == null)
                return false;

            string op = token.Text;
            if (op == ")")
                return false;

            if (op == ",")
                return false;

            if (op == "")
                return false;

            if (op.Length > 2)
                return false;

            if (op == ".")
                return true;
            else if (op == "+")
                return true;
            else if (op == "-")
                return true;
            else if (op == "*")
                return true;
            else if (op == "/")
                return true;
            else if (op == "==")
                return true;
            else if (op == "!=")
                return true;
            else if (op == ";")
                return true;
            else if (op == ">")
                return true;
            else if (op == "<")
                return true;
            else if (op == "&")
                return true;
            else if (op == "&&")
                return true;

            return false;
        }

        //
        // Helper function to extract an expression term from a token stream.
        //
        // Returns true if a term was found, and passes out the index of the
        // next token to examine. No tokens should be popped if this helper
        // returns false.
        //
        // Note that in cases where a terminating token is present, such as
        // a closing parenthesis or a comma, this will return false; however
        // this does not indicate a syntax error.
        //
        private static bool ParseExpressionTerm(ParseSession parser, bool atstart, int starttoken, out int consumedtokens, out bool matchedstatement)
        {
            matchedstatement = false;
            int totaltokens = starttoken;
            consumedtokens = totaltokens;

            if (parser.PeekToken(totaltokens) == null)
                return false;

            if (parser.CheckToken(totaltokens, ")"))
                return false;

            if (parser.CheckToken(totaltokens, ","))
                return false;

            if (parser.CheckToken(totaltokens, "("))
            {
                ++totaltokens;
                if (Parse(parser, totaltokens, out totaltokens) != null)
                {
                    ++totaltokens;
                    consumedtokens = totaltokens;
                    return true;
                }

                return false;
            }

            if (parser.CheckToken(totaltokens, "!"))
            {
                ++totaltokens;
                var ret = ParseExpressionTerm(parser, atstart, totaltokens, out totaltokens, out matchedstatement);

                if (matchedstatement && parser.CheckToken(totaltokens, ")"))
                    ++totaltokens;

                consumedtokens = totaltokens;

                return ret;
            }

            // The following check is a tiny optimization. Instead of trying to parse
            // a statement first and then eventually giving up, we first try checking
            // for common literals. This makes code with literals parse a fractional,
            // almost insignificant amount faster. It's actually a relic from the way
            // the compiler implements this logic. Maybe it makes sense to remove it,
            // and aim for simplicity over ideal efficiency. Profiling data is so far
            // insufficient to decide for sure, so this stays in the interim.
            if (parser.CheckToken(totaltokens, "false") || parser.CheckToken(totaltokens, "true") || parser.CheckToken(totaltokens, "0") || parser.CheckToken(totaltokens, "0.0"))
                ++totaltokens;
            else if (parser.ParsePreopStatement(totaltokens, out totaltokens))
            {
                consumedtokens = totaltokens;
                return true;
            }
            else if (parser.ParseStatement(totaltokens, out totaltokens))
            {
                matchedstatement = true;
                consumedtokens = totaltokens;
                return true;
            }
            else
            {
                ++totaltokens;
            }

            consumedtokens = totaltokens;
            return true;
        }


        //
        // Factory method for producing an Expression object from a token stream.
        //
        // Returns a valid Expression if one could be parsed, otherwise returns null.
        //
        // Note that the parsed expression is content-less, i.e. we don't preserve
        // any details about it. This is intentional, since IntelliSense providers
        // don't need to care about the details of an expression (yet).
        //
        // As a consequence of this, we can safely ignore things (like the relative
        // precedences of operators) because the tokens will parse the same way for
        // all cases we care about supporting. Some things like unary operators are
        // handled specially in the helper functions, but these should be viewed as
        // supporting the general goal of not interpreting the expression.
        //
        internal static Expression Parse(ParseSession parser, int starttoken, out int consumedtokens)
        {
            int totaltokens = starttoken;
            consumedtokens = totaltokens;
            bool matchedstatement = false;

            if (!ParseExpressionTerm(parser, true, totaltokens, out totaltokens, out matchedstatement))
                return null;

            if (matchedstatement && parser.CheckToken(totaltokens, ")"))
            {
            }
            else
            {
                while (ParseExpressionOperator(parser, totaltokens))
                {
                    ++totaltokens;
                    if (!ParseExpressionTerm(parser, false, totaltokens, out totaltokens, out matchedstatement))
                        return null;
                }
            }

            consumedtokens = totaltokens;
            return new Expression();
        }
    }
}

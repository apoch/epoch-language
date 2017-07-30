using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class Expression
    {

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

        private static bool ParseExpressionTerm(ParseSession parser, bool atstart, int starttoken, out int consumedtokens, out bool matchedstatement)
        {
            matchedstatement = false;
            int totaltokens = starttoken;
            consumedtokens = totaltokens;

            if (parser.CheckToken(totaltokens, ")"))
                return false;

            if (parser.CheckToken(totaltokens, ","))
                return false;

            if (parser.PeekToken(totaltokens) == null)
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

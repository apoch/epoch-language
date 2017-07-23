using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class SumType
    {
        internal class ParsedSumType
        {
            public Token Name;
            public SumType Type;
        }

        internal static ParsedSumType Parse(ParseSession parser)
        {
            int totaltokens = 0;
            if (!parser.CheckToken(0, "type"))
                return null;

            Token sumtypename = parser.PeekToken(1);
            if (sumtypename == null || string.IsNullOrEmpty(sumtypename.Text))
                return null;

            if (parser.CheckToken(2, "<"))
            {
                if (!parser.ParseTemplateParameters(3, sumtypename, out totaltokens))
                    return null;

                if (!parser.CheckToken(totaltokens, ":"))
                    return null;
            }
            else if (!parser.CheckToken(2, ":"))
                return null;
            else if (!parser.CheckToken(4, "|"))
                return null;
            else
                totaltokens = 3;

            totaltokens += 2;

            bool hasbases = true;
            do
            {
                if (parser.CheckToken(totaltokens + 1, "<"))
                {
                    var token = parser.PeekToken(totaltokens);
                    if (token == null)
                        return null;

                    if (!parser.ParseTemplateArguments(totaltokens + 2, token, out totaltokens))
                        return null;
                }
                else
                {
                    totaltokens += 2;
                }

                hasbases = parser.CheckToken(totaltokens + 1, "|");
            } while (hasbases);

            // Success! Consume everything and return the constructed result
            parser.ConsumeTokens(totaltokens);
            var sumtype = new SumType { };
            return new ParsedSumType { Name = sumtypename, Type = sumtype };
        }
    }
}

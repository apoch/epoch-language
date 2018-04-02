//
// The Epoch Language Project
// Visual Studio Integration/Extension
//
// Helper for parsing sum type definitions.
//

namespace EpochVSIX.Parser
{
    //
    // Wrapper class for a sum type definition.
    //
    // Note that this presently does not carry any state.
    // We may wish to attach some later on if things like
    // syntax hints and other IDE features would benefit.
    //
    class SumType
    {
        internal static ParsedObject<SumType> Parse(ParseSession parser)
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
                totaltokens = 2;

            do
            {
                ++totaltokens;

                if (parser.CheckToken(totaltokens + 1, "<"))
                {
                    var token = parser.PeekToken(totaltokens);
                    if (token == null)
                        return null;

                    if (!parser.ParseTemplateArguments(totaltokens + 2, token, out totaltokens))
                        return null;
                }
                else
                    ++totaltokens;

            } while (parser.CheckToken(totaltokens, "|"));

            // Success! Consume everything and return the constructed result
            parser.ConsumeTokens(totaltokens);
            var sumtype = new SumType { };
            return new ParsedObject<SumType> { Name = sumtypename, Object = sumtype };
        }
    }
}

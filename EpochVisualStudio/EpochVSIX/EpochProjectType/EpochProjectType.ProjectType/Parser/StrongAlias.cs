//
// The Epoch Language Project
// Visual Studio Integration/Extension
//
// Helper class for parsing strong type aliases from a token stream.
//

namespace EpochVSIX.Parser
{
    class StrongAlias
    {

        //
        // Parse a strong type alias from a token stream.
        //
        // Presently there is no associated data, although it would
        // probably be desirable to store the base type so that the
        // IntelliSense tips for the alias can show it.
        //
        internal static ParsedObject<StrongAlias> Parse(ParseSession parser)
        {
            if (!parser.CheckToken(0, "type"))
                return null;

            var nametoken = parser.PeekToken(1);
            if (nametoken == null)
                return null;

            if (!parser.CheckToken(2, ":"))
                return null;

            parser.ConsumeTokens(4);
            var aliastype = new StrongAlias { };
            return new ParsedObject<StrongAlias> { Name = nametoken, Object = aliastype };
        }

    }
}

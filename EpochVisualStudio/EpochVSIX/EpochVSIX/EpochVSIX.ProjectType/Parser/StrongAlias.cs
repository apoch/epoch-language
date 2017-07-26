using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class StrongAlias
    {

        internal class ParsedStrongAlias
        {
            public Token Name;
            public StrongAlias Type;
        }


        internal static ParsedStrongAlias Parse(ParseSession parser)
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
            return new ParsedStrongAlias { Name = nametoken, Type = aliastype };
        }

    }
}

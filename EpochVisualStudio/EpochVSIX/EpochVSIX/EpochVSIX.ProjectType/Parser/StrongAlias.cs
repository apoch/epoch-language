using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class StrongAlias
    {
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

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class WeakAlias
    {

        internal class ParsedWeakAlias
        {
            public Token Name;
            public WeakAlias Type;
        }


        internal static ParsedWeakAlias Parse(ParseSession parser)
        {
            if (!parser.CheckToken(0, "alias"))
                return null;

            var nametoken = parser.PeekToken(1);
            if (nametoken == null)
                return null;

            if (!parser.CheckToken(2, "="))
                return null;

            parser.ConsumeTokens(4);
            var aliastype = new WeakAlias { };
            return new ParsedWeakAlias { Name = nametoken, Type = aliastype };
        }

    }
}

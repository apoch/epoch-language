using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class SyntaxError : Exception
    {
        private Token OriginToken;

        public Token Origin
        {
            get { return OriginToken; }
        }

        public SyntaxError(string message, Token origin)
            : base(message)
        {
            OriginToken = origin;
        }
    }
}

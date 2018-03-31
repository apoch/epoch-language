//
// The Epoch Language Project
// Visual Studio Integration/Extension
//
// Basic exception type for wrapping syntax errors thrown by the parser.
//

using System;

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

namespace EpochVSIX.Parser
{
    class WeakAlias
    {
        internal static ParsedObject<WeakAlias> Parse(ParseSession parser)
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
            return new ParsedObject<WeakAlias> { Name = nametoken, Object = aliastype };
        }

    }
}

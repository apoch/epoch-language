using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class Variable
    {
        public enum Origins
        {
            Local,
            Parameter,
            Return,
            Global
        }

        public Token Name;
        public TypeSignatureInstantiated Type;
        public Origins Origin;


        internal static Variable Parse(ParseSession parser, int starttoken, out int consumedtokens, Origins origin)
        {
            consumedtokens = starttoken;

            if (parser.CheckToken(starttoken + 1, "."))
                return null;

            var basetypename = parser.PeekToken(starttoken);

            bool templated = false;
            int totaltokens = starttoken + 1;
            if (parser.CheckToken(totaltokens, "<"))
            {
                if (!parser.ParseTemplateArguments(totaltokens, basetypename, out totaltokens))
                    return null;

                templated = true;
            }

            if (!parser.CheckToken(totaltokens + 2, "="))
                return null;

            var type = TypeSignatureInstantiated.Construct(parser, starttoken, totaltokens);
            var variable = new Variable { Name = basetypename, Origin = origin, Type = type };

            totaltokens += 2;

            Expression.Parse(parser, totaltokens, out totaltokens);
            while (parser.CheckToken(totaltokens, ","))
            {
                ++totaltokens;
                Expression.Parse(parser, totaltokens, out totaltokens);
            }

            consumedtokens = totaltokens;

            return variable;
        }
    }
}

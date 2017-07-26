using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class GlobalBlock
    {

        public List<Variable> Variables;


        private GlobalBlock()
        {
            Variables = new List<Variable>();
        }


        internal static GlobalBlock Parse(ParseSession parser)
        {
            if (!parser.CheckToken(0, "global"))
                return null;

            if (!parser.CheckToken(1, "{"))
                return null;

            var block = new GlobalBlock();

            int totaltokens = 2;
            do
            {
                var variable = Variable.Parse(parser, totaltokens, out totaltokens, Variable.Origins.Global);
                if (variable != null)
                    block.Variables.Add(variable);
                else
                    break;

            } while (!parser.CheckToken(totaltokens, "}"));

            parser.ConsumeTokens(totaltokens);
            return block;
        }
    }
}

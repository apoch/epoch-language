using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class GlobalBlock
    {

        private List<Variable> Variables;


        private GlobalBlock()
        {
            Variables = new List<Variable>();
        }


        internal void AugmentProject(Project project)
        {
            foreach (var variable in Variables)
            {
                project.RegisterGlobalVariable(variable);
            }
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
                    return null;

                parser.ConsumeTokens(totaltokens);
                totaltokens = 0;
            } while (!parser.CheckToken(totaltokens, "}"));

            ++totaltokens;
            parser.ConsumeTokens(totaltokens);
            return block;
        }
    }
}

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class ParseSession
    {

        private LexSession Lexer;


        public ParseSession(LexSession lexer)
        {
            Lexer = lexer;
        }

        public void AugmentProject(Project project)
        {
            while (!Lexer.Empty)
            {
                var peek = Lexer.PeekToken(0);
                if ((peek != null) && (peek.Text == "structure"))
                {
                    var structurename = Lexer.PeekToken(1);
                    if (structurename != null)
                        project.RegisterStructureType(structurename, new Structure());

                    Lexer.ConsumeTokens(2);
                }
                else
                {
                    Lexer.ConsumeTokens(1);
                }
            }
        }
    }
}

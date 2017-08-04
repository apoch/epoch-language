using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EpochVSIX.Parser
{
    class LexicalScope
    {
        public SourceFile File;

        public int StartLine;
        public int StartColumn;
        public int EndLine;
        public int EndColumn;

        public List<Variable> Variables = new List<Variable>();

        public LexicalScope ParentScope = null;
        public List<LexicalScope> ChildScopes = null;


        internal static LexicalScope Parse(ParseSession parser, LexicalScope parentscope, int starttoken, out int consumedtokens)
        {
            consumedtokens = starttoken;
            int totaltokens = starttoken;
            Token afterStartBrace = parser.PeekToken(totaltokens);

            if (afterStartBrace == null)
            {
                parser.ConsumeTokens(starttoken);
                throw new SyntaxError("Missing closing }", parser.ReversePeekToken());
            }

            var ret = new LexicalScope();
            ret.File = afterStartBrace.File;
            ret.StartLine = afterStartBrace.Line;
            ret.StartColumn = afterStartBrace.Column;
            ret.ParentScope = parentscope;

            if (parentscope != null)
            {
                if (parentscope.ChildScopes == null)
                    parentscope.ChildScopes = new List<LexicalScope>();

                parentscope.ChildScopes.Add(ret);
            }

            while (!parser.CheckToken(totaltokens, "}"))
            {
                if (CodeHelpers.ParseEntity(parser, ret, totaltokens, out totaltokens)
                    || parser.ParsePreopStatement(totaltokens, out totaltokens)
                    || parser.ParsePostopStatement(totaltokens, out totaltokens)
                    || parser.ParseStatement(totaltokens, out totaltokens))
                {
                    parser.ConsumeTokens(totaltokens);
                    totaltokens = 0;
                    continue;
                }

                var variable = Variable.Parse(parser, totaltokens, out totaltokens, Variable.Origins.Local);
                if (variable != null)
                {
                    ret.Variables.Add(variable);
                    parser.ConsumeTokens(totaltokens);
                    totaltokens = 0;
                    continue;
                }

                if (CodeHelpers.ParseAssignment(parser, totaltokens, out totaltokens))
                {
                    parser.ConsumeTokens(totaltokens);
                    totaltokens = 0;
                    continue;
                }

                consumedtokens = totaltokens;
                return null;
            }

            Token endBrace = parser.PeekToken(totaltokens);

            ++totaltokens;
            consumedtokens = totaltokens;

            ret.EndLine = endBrace.Line;
            ret.EndColumn = endBrace.Column;
            return ret;
        }

    }
}

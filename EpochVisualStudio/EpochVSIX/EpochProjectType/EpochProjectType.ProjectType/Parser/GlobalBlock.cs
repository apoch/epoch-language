//
// The Epoch Language Project
// Visual Studio Integration/Extension
//
// Wrapper object for a list of global variables.
//
// Global state is one of those things you'd really rather not have
// to have in a language, but it's just so handy.... A lot of code,
// especially in the contemporary Epoch compiler, is reliant on the
// global block. Global variables are visible in every function and
// have a lifetime starting just prior to the call to entrypoint().
//
// For the purposes of the IntelliSense provider, a global block is
// just a list of a bunch of variable declarations wrapped in a top
// level code block prefixed by the token "global".
//
// Technically, having multiple global blocks (in a single compiled
// project, regardless of file layout) is illegal, so we don't need
// the formal wrapper in a strict sense. But it makes the parsing a
// little cleaner, so there's that.
//

using System.Collections.Generic;

namespace EpochVSIX.Parser
{
    //
    // This wrapper holds a list of global variables and provides
    // logic for parsing a global block from a token stream.
    //
    class GlobalBlock
    {

        private List<Variable> Variables;


        private GlobalBlock()
        {
            Variables = new List<Variable>();
        }


        //
        // Helper for registering a list of globals with a Project.
        //
        internal void AugmentProject(Project project)
        {
            foreach (var variable in Variables)
            {
                project.RegisterGlobalVariable(variable);
            }
        }

        //
        // Helper for parsing a list of global variables.
        //
        // Consumes tokens as it goes; returns null if a block is not
        // matched, and proceeds until the block is closed. Also returns
        // null if a syntax error occurs anywhere in the block.
        //
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

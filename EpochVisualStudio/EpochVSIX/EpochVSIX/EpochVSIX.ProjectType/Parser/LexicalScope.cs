//
// The Epoch Language Project
// Visual Studio Integration/Extension
//
// Class for managing a lexical scope, its variables, and its hierarchy.
//
// A lexical scope for the purposes of the IntelliSense parser is defined
// as a start and end position in a specific file. Inside that range, any
// variables defined "belong" to that scope. Scopes may be nested in code
// so this class implements a parent/child hierarchy for representing the
// nesting. Additionally, this class can parse a scope and its body given
// a token stream.
//

using System.Collections.Generic;

namespace EpochVSIX.Parser
{
    //
    // Representation of a lexical scope, its endpoints, and its variables.
    //
    // Note that a scope does not store data on statements or entities inside
    // the body of the scope, except insofar as those constructs introduce an
    // additional lexical scope (or several). The main focus of this class is
    // to allow IntelliSense to map a code location in a given file over to a
    // set of variables which may be legally accessed at that location.
    //
    class LexicalScope
    {
        //
        // A lexical scope is uniquely identified by the tuple of:
        //  - A file
        //  - A start line
        //  - A start column
        //  - An end line
        //  - An end column
        //
        public SourceFile File;
        public int StartLine;
        public int StartColumn;
        public int EndLine;
        public int EndColumn;


        //
        // The meat of the scope - its defined variables
        //
        public List<Variable> Variables = new List<Variable>();


        //
        // The hierarchy of the scope - its immediate containing scope
        // as well as any child scopes opened inside its lexical span.
        //
        // Note that this is only one layer of hieararchy; grandparent
        // and grandchild scopes are not stored directly. However they
        // can be reached trivially by traversing the hierarchy.
        //
        public LexicalScope ParentScope = null;
        public List<LexicalScope> ChildScopes = null;


        //
        // Helper routine to parse a lexical scope in its entirety given a token stream.
        //
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

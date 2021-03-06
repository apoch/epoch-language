﻿//
// The Epoch Language Project
// Visual Studio Integration/Extension
//
// Wrapper for parsing code from a token stream via a lexer.
//
// This module is sort of a high level driver for parsing code
// from the token stream produced by a LexSession object. Some
// helpers are provided for common parsing routines as well. A
// given chunk of code is parsed as if it were a complete code
// file, i.e. no fractional or incremental parsing is done for
// the time being. One parser session is expected to exist per
// file of the project being parsed. All of the resulting data
// is accumulated in an instance of the Project class. Exactly
// one instance of Project is expected to exist per each .EPRJ
// Visual Studio project file. Multiple projects may of course
// exist per solution file.
//
// Most actual parsing logic is farmed out to specific classes
// that recognize the associated tokens for a given construct.
// For example, variable declarations are parsed using methods
// of the Variable class.
//
// A secondary role of this class is to glue together the code
// parsing process with the Visual Studio UI - e.g. to provide
// IntelliSense errors and other code-time hints.
//

using Microsoft.VisualStudio.Shell;
using System;

namespace EpochVSIX.Parser
{
    //
    // Wrapper class for parsing a single file worth of code.
    //
    // Assumes a one-to-one relationship with an underlying LexSession
    // for doing lexical analysis on the input code. This module holds
    // a number of helpers for common parsing needs, which can be used
    // freely by other parser-helper objects.
    //
    class ParseSession
    {

        //
        // Helper wrapper for piping VS services to the error list provider.
        //
        internal class ErrorListHelper : IServiceProvider
        {
            public object GetService(Type serviceType)
            {
                return Package.GetGlobalService(serviceType);
            }
        }


        //
        // Binding to the lexer used to retrieve tokens for parsing.
        //
        private LexSession Lexer;

        //
        // Helper for wiring up parser errors to the VS UI.
        //
        private ErrorListProvider ErrorProvider;


        //
        // Hacky hint used to track syntax error starting locations.
        //
        private Token LastConsumedToken = null;


        //
        // Construct and initialize a new parsing session, using the given lexer.
        //
        // Also sets up error provision infrastructure, so that exceptions thrown
        // during the parsing process can be routed to the VS UI (as IntelliSense
        // errors or other "fix me" hints).
        //
        public ParseSession(LexSession lexer)
        {
            Lexer = lexer;

            var helper = new ErrorListHelper();
            ErrorProvider = new ErrorListProvider(helper);
            ErrorProvider.ProviderName = "Epoch Language";
            ErrorProvider.ProviderGuid = new Guid("b95d8222-cdfc-44b4-9635-585db8a10b9f");          // VsPackage.cs - PackageGuid
        }

        //
        // Parse the code that was provided in construction, and stash the results
        // in the given Project instance. This routine should do its best to avoid
        // leaking exceptions generated by the parsing process itself, since those
        // errors will propagate upward until VS detects them and turns off future
        // operations by the VSIX package (until restarted).
        //
        public bool AugmentProject(Project project)
        {
            try
            {
                while (!Lexer.Empty)
                {
                    var sumtype = SumType.Parse(this);
                    if (sumtype != null)
                    {
                        project.RegisterSumType(sumtype.Name, sumtype.Object);
                        continue;
                    }

                    var strongalias = StrongAlias.Parse(this);
                    if (strongalias != null)
                    {
                        project.RegisterStrongAlias(strongalias.Name, strongalias.Object);
                        continue;
                    }

                    var weakalias = WeakAlias.Parse(this);
                    if (weakalias != null)
                    {
                        project.RegisterWeakAlias(weakalias.Name, weakalias.Object);
                        continue;
                    }

                    var structure = Structure.Parse(this);
                    if (structure != null)
                    {
                        project.RegisterStructureType(structure.Name, structure.Object);
                        continue;
                    }

                    var globals = GlobalBlock.Parse(this);
                    if (globals != null)
                    {
                        globals.AugmentProject(project);
                        continue;
                    }

                    var function = FunctionSignature.Parse(this);
                    if (function != null)
                    {
                        project.RegisterFunction(function);
                        continue;
                    }

                    if (!Lexer.Empty)
                        throw new SyntaxError("Syntax error", PeekToken(0));
                }
            }
            catch (SyntaxError ex)
            {
                var errorTask = new ErrorTask();
                errorTask.Text = ex.Message;
                errorTask.Category = TaskCategory.CodeSense;
                errorTask.ErrorCategory = TaskErrorCategory.Error;
                errorTask.Document = Lexer.FileName;
                errorTask.Line = (ex.Origin != null) ? (ex.Origin.Line) : 0;
                errorTask.Column = (ex.Origin != null) ? (ex.Origin.Column) : 0;
                errorTask.Navigate += project.NavigationHandler;

                ErrorProvider.Tasks.Add(errorTask);
                return false;
            }

            return true;
        }

        //
        // Check if the token at the given offset is an expected string.
        //
        // This is highly useful for determining if code is well-formed or
        // otherwise conforms to a specific code construct. Note that this
        // implementation specifically handles the case of null tokens put
        // out by the lexer, to hide the complexity from parser routines.
        //
        internal bool CheckToken(int offset, string expected)
        {
            if (Lexer.Empty)
                return false;

            var token = Lexer.PeekToken(offset);
            if (token == null)
                return false;

            return token.Text.Equals(expected);
        }

        //
        // Consume some number of tokens from the token stream.
        //
        // This is a bit more complex than it has to be in order to make a
        // best-effort guess at where parsing last successfully reached.
        //
        internal void ConsumeTokens(int originalcount)
        {
            int count = originalcount;
            Token t = null;
            while (t == null && count >= 0)
            {
                t = Lexer.PeekToken(count);
                --count;
            }

            if (t != null)
                LastConsumedToken = t;

            Lexer.ConsumeTokens(originalcount);
        }

        //
        // Retrieve a token with optional lookahead.
        //
        // This is basically just a pass-through to the lexer. To find
        // more information on what happens when a token is requested,
        // check out the LexSession documentation comments.
        //
        internal Token PeekToken(int offset)
        {
            if (Lexer.Empty)
                return null;

            return Lexer.PeekToken(offset);
        }


        //
        // Helper routine for parsing a pre-operator statement.
        //
        // An example of such a statement would be "++count" - where the
        // operator (++) occurs prior to the operand (count). Because of
        // Epoch's strict rules for how such a statement may be uttered,
        // parsing this case is pretty trivial.
        //
        internal bool ParsePreopStatement(int startoffset, out int consumedtokens)
        {
            consumedtokens = startoffset;
            bool recognized = false;
            var token = PeekToken(startoffset);
            if (token == null)
                return false;

            string potential = token.Text;

            if (potential == "++")
                recognized = true;
            else if (potential == "--")
                recognized = true;

            if (recognized)
            {
                do
                {
                    startoffset += 2;
                } while (CheckToken(startoffset, "."));

                consumedtokens = startoffset;
                return true;
            }

            return false;
        }

        //
        // Helper routine for parsing a post-operator statement.
        //
        // These statements take the form "count++" - the operator (++)
        // trails the operand (count). As with pre-operator statements,
        // the rules are simple and strict, so parsing is again easy.
        //
        internal bool ParsePostopStatement(int startoffset, out int consumedtokens)
        {
            consumedtokens = startoffset;
            int totaltokens = startoffset;

            var token = PeekToken(totaltokens);
            if (token == null)
                return false;

            while (CheckToken(totaltokens + 1, "."))
                ++totaltokens;

            totaltokens += 2;

            var potential = PeekToken(totaltokens);
            if (potential == null)
                return false;

            if (potential.Text == "++" || potential.Text == "--")
            {
                consumedtokens = totaltokens;
                return true;
            }

            return false;
        }

        //
        // Helper routine for parsing a function invocation statement.
        //
        // These "regular" statements are optionally decorated with template
        // arguments, and take an arbitrary number of expressions. Note that
        // this routine is not designed to sanity check the statement, so as
        // long as it parses correctly, it will pass through. This means the
        // parameters might be wrong in number, type, position, and so on.
        //
        internal bool ParseStatement(int startoffset, out int consumedtokens)
        {
            consumedtokens = startoffset;
            int totaltokens = consumedtokens;

            if (CheckToken(totaltokens + 1, "<"))
            {
                var token = PeekToken(totaltokens);
                totaltokens += 2;
                if (!ParseTemplateArguments(totaltokens, token, out totaltokens))
                    return false;
            }
            else
                ++totaltokens;

            if (!CheckToken(totaltokens, "("))
                return false;

            ++totaltokens;
            while (!CheckToken(totaltokens, ")"))
            {
                var expr = Expression.Parse(this, totaltokens, out totaltokens);
                if (expr == null)
                    return false;

                if (CheckToken(totaltokens, ","))
                    ++totaltokens;
            }

            ++totaltokens;
            consumedtokens = totaltokens;
            return true;
        }


        //
        // Helper routine for parsing template parameters.
        //
        // Template parameters can be thought of as specifying the
        // types and positions of valid template arguments. Put in
        // concrete terms:
        //
        //   <type T>
        //
        // Expresses a template with one parameter, T. In order to
        // conform to the template, any provided template argument
        // must be a type. So:
        //
        //   <integer>
        //
        // Expresses a template argument that satisfies the above.
        // Meanwhile:
        //
        //   <42, "test">
        //
        // Does not satisfy the template, for two reasons. Firstly
        // the argument count is wrong (two versus one). Secondly,
        // the arguments do not name types - they are literals.
        //
        // Template parameters and arguments may appear in various
        // places in Epoch syntax, so they are factored out into a
        // pair of helper routines for parsing purposes.
        //
        internal bool ParseTemplateParameters(int startoffset, Token basetypename, out int totaltokens)
        {
            totaltokens = startoffset;
            bool hasparams = true;

            while (hasparams)
            {
                totaltokens += 2;

                if (CheckToken(totaltokens, ">"))
                {
                    ++totaltokens;
                    hasparams = false;
                }
                else if (CheckToken(totaltokens, ","))
                    ++totaltokens;
                else
                    return false;
            }

            return true;
        }

        //
        // Helper routine for parsing template arguments.
        //
        // See the comments on ParseTemplateParameters for a description
        // of the relationship between template parameters and arguments
        // as well as a clarifying example.
        //
        internal bool ParseTemplateArguments(int startoffset, Token basetypename, out int consumedtokens)
        {
            consumedtokens = startoffset;
            int totaltokens = startoffset;
            bool hasargs = true;

            while (hasargs)
            {
                ++totaltokens;

                if (CheckToken(totaltokens, ">"))
                    hasargs = false;
                else if (CheckToken(totaltokens, ","))
                    ++totaltokens;
                else
                    return false;

            }

            ++totaltokens;
            consumedtokens = totaltokens;
            return true;
        }

        //
        // Retrieve the last successfully consumed token.
        //
        // Used when generating error messages to point to the last
        // valid code that was parsed. Sloppy and hacky, but not so
        // egregious as to warrant changing it any time soon.
        //
        internal Token ReversePeekToken()
        {
            return LastConsumedToken;
        }
    }
}

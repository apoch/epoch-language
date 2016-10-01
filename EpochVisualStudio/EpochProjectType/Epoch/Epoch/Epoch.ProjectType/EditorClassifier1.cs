//------------------------------------------------------------------------------
// <copyright file="EditorClassifier1.cs" company="Company">
//     Copyright (c) Company.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.ComponentModel.Composition;
using Microsoft.VisualStudio.Text;
using Microsoft.VisualStudio.Text.Classification;
using Microsoft.VisualStudio.Utilities;
using Epoch.ProjectParser;

namespace EpochVS
{
    internal class EditorClassifier1 : IClassifier
    {
        private readonly IClassificationTypeRegistryService classificationRegistry;
        private readonly HashSet<string> TypeKeywords;
        private readonly HashSet<string> LiteralKeywords;

        internal EditorClassifier1(IClassificationTypeRegistryService registry)
        {
            classificationRegistry = registry;

            TypeKeywords = new HashSet<string>()
            {
                "integer",
                "integer16",
                "integer64",
                "boolean",
                "string",
                "real",
                "buffer",
                "nothing",
                "ref",
                "structure",
                "type",
            };

            LiteralKeywords = new HashSet<string>()
            {
                "true",
                "false"
            };
        }

        #region IClassifier

#pragma warning disable 67

        /// <summary>
        /// An event that occurs when the classification of a span of text has changed.
        /// </summary>
        /// <remarks>
        /// This event gets raised if a non-text change would affect the classification in some way,
        /// for example typing /* would cause the classification to change in C# without directly
        /// affecting the span.
        /// </remarks>
        public event EventHandler<ClassificationChangedEventArgs> ClassificationChanged;

#pragma warning restore 67


        private enum State
        {
            Default,
            Comment,
            Type,
            StringLiteral,
            Symbol,
            Literal,
            HexLiteral,
            IdentifierFunction,
            IdentifierUDT,
        }

        public IList<ClassificationSpan> GetClassificationSpans(SnapshotSpan span)
        {
            var result = new List<ClassificationSpan>();

            if (span.Length == 0)
                return result;

            int startline = span.Start.GetContainingLine().LineNumber;
            int endline = span.End.GetContainingLine().LineNumber;

            for (int lineindex = startline; lineindex <= endline; ++lineindex)
            {
                var line = span.Snapshot.GetLineFromLineNumber(lineindex);
                var text = line.GetText();

                State state = State.Default;
                int statestart = 0;

                for(int i = 0; i < text.Length; ++i)
                {
                    int newstart = i;
                    State prevstate = state;

                    string ch = text.Substring(i, 1);
                    string chnext = (i < text.Length - 1) ? text.Substring(i + 1, 1) : "";

                    switch (state)
                    {
                        case State.Default:
                            if (ch == "/" && chnext == "/")
                                state = State.Comment;
                            else if (ch == "\"")
                                state = State.StringLiteral;
                            else if (ch == ":")
                                state = State.Symbol;
                            else if (ch == "-" && chnext == ">")
                            {
                                ++i;
                                state = State.Symbol;
                            }
                            else if (ch == "0" && chnext == "x")
                            {
                                ++i;
                                state = State.HexLiteral;
                            }
                            else if ("0123456789".Contains(ch))
                                state = State.Literal;
                            else if (" \t\r\n".Contains(ch))
                                state = State.Default;
                            else if (char.IsLetterOrDigit(ch[0]))
                                state = State.Type;

                            break;

                        case State.Comment:
                            break;

                        case State.Type:
                            if (!char.IsLetterOrDigit(ch[0]) && ch != "_")
                                state = State.Default;
                            break;

                        case State.IdentifierFunction:
                            if (!char.IsLetterOrDigit(ch[0]) && ch != "_")
                                state = State.Default;
                            break;

                        case State.StringLiteral:
                            if (ch == "\"")
                            {
                                ++i;
                                state = State.Default;
                            }
                            break;

                        case State.Symbol:
                            state = State.Default;
                            break;

                        case State.Literal:
                            if (!"0123456789.".Contains(ch))
                                state = State.Default;
                            break;

                        case State.HexLiteral:
                            if (!"01234567890abcdef".Contains(ch))
                                state = State.Default;
                            break;
                    }

                    if (state != prevstate)
                    {
                        UpdateState(line, prevstate, i, statestart, result, span);

                        statestart = newstart;
                    }
                }

                UpdateState(line, state, text.Length, statestart, result, span);
            }
            return result;
        }

        private void UpdateState(ITextSnapshotLine line, State prevstate, int i, int statestart, IList<ClassificationSpan> result, SnapshotSpan span)
        {
            IClassificationType classification = null;

            if (prevstate == State.Type || prevstate == State.IdentifierFunction)
                prevstate = ClassifyToken(line.GetText().Substring(statestart, i - statestart));

            switch (prevstate)
            {
                case State.Comment:
                    classification = classificationRegistry.GetClassificationType("comment");
                    break;

                case State.StringLiteral:
                    classification = classificationRegistry.GetClassificationType("string");
                    break;

                case State.Literal:
                    classification = classificationRegistry.GetClassificationType("number");
                    break;

                case State.HexLiteral:
                    classification = classificationRegistry.GetClassificationType("number");
                    break;

                case State.Type:
                    classification = classificationRegistry.GetClassificationType("keyword");
                    break;

                case State.IdentifierFunction:
                    classification = classificationRegistry.GetClassificationType("cppFunction");
                    break;

                case State.IdentifierUDT:
                    classification = classificationRegistry.GetClassificationType("class name");
                    break;
            }

            if (classification != null)
                result.Add(new ClassificationSpan(new SnapshotSpan(span.Snapshot, new Span(line.Start + statestart, i - statestart)), classification));

        }

        private State ClassifyToken(string token)
        {
            if (token.Length <= 0)
                return State.Default;

            if (TypeKeywords.Contains(token))
                return State.Type;

            if (LiteralKeywords.Contains(token))
                return State.Literal;

            List<string> functionNames = new List<string>();
            ProjectParser.GetAvailableFunctionNames(functionNames);
            if (functionNames.Contains(token))
                return State.IdentifierFunction;

            List<string> structureNames = new List<string>();
            ProjectParser.GetAvailableStructureNames(structureNames);
            if (structureNames.Contains(token))
                return State.IdentifierUDT;

            return State.Default;
        }

        #endregion
    }

    internal static class FileAndContentTypeDefinitions
    {
        [Export]
        [Name("EpochFile")]
        [BaseDefinition("text")]
        internal static ContentTypeDefinition EpochContentTypeDefinition;

        [Export]
        [FileExtension(".epoch")]
        [ContentType("EpochFile")]
        internal static FileExtensionToContentTypeDefinition EpochFileExtensionDefinition;
    }
}

﻿//------------------------------------------------------------------------------
// <copyright file="EpochClassifier.cs" company="Company">
//     Copyright (c) Company.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.ComponentModel.Composition;
using Microsoft.VisualStudio.Text;
using Microsoft.VisualStudio.Text.Classification;
using Microsoft.VisualStudio.Utilities;

namespace EpochVSIX
{
    /// <summary>
    /// Classifier that classifies all text as an instance of the "EpochClassifier" classification type.
    /// </summary>
    internal class EpochClassifier : IClassifier
    {
        private readonly IClassificationTypeRegistryService classificationRegistry;
        private readonly HashSet<string> TypeKeywords;
        private readonly HashSet<string> LiteralKeywords;

        internal Parser.Project ParsedProject;

        internal EpochClassifier(IClassificationTypeRegistryService registry, Parser.Project project)
        {
            ParsedProject = project;
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
                "if",
                "elseif",
                "else",
                "while",
                "global",
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
            IdentifierType,
        }

        public IList<ClassificationSpan> GetClassificationSpans(SnapshotSpan span)
        {
            return GetAllSpans(span, false);
        }

        public IList<ClassificationSpan> GetAllSpans(SnapshotSpan span, bool includeUnclassified)
        {
            var result = new List<ClassificationSpan>();

            if (span.Length == 0)
                return result;

            if (ParsedProject == null)
                span.Snapshot.TextBuffer.Properties.TryGetProperty(typeof(Parser.Project), out ParsedProject);

            if (ParsedProject != null)
                ParsedProject.ParseIfOutdated();

            int startline = span.Start.GetContainingLine().LineNumber;
            int endline = span.End.GetContainingLine().LineNumber;

            for (int lineindex = startline; lineindex <= endline; ++lineindex)
            {
                var line = span.Snapshot.GetLineFromLineNumber(lineindex);
                var text = line.GetText();

                State state = State.Default;
                int statestart = 0;

                for (int i = 0; i < text.Length; ++i)
                {
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
                        UpdateState(line, prevstate, i, statestart, result, span, includeUnclassified);

                        statestart = i;
                    }
                }

                UpdateState(line, state, text.Length, statestart, result, span, includeUnclassified);
            }
            return result;
        }

        private void UpdateState(ITextSnapshotLine line, State prevstate, int i, int statestart, IList<ClassificationSpan> result, SnapshotSpan span, bool includeUnclassified)
        {
            IClassificationType classification = null;

            if (prevstate == State.Type || prevstate == State.IdentifierFunction)
                prevstate = ClassifyToken(line.GetText().Substring(statestart, i - statestart));

            switch (prevstate)
            {
                case State.Default:
                    classification = null;
                    break;

                case State.Comment:
                    classification = classificationRegistry.GetClassificationType("comment");        // TODO - provide custom classifications for Epoch code highlighting
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

                case State.IdentifierType:
                    classification = classificationRegistry.GetClassificationType("cppType");
                    break;
            }

            if (classification != null)
                result.Add(new ClassificationSpan(new SnapshotSpan(span.Snapshot, new Span(line.Start + statestart, i - statestart)), classification));
            else if (includeUnclassified)
                result.Add(new ClassificationSpan(new SnapshotSpan(span.Snapshot, new Span(line.Start + statestart, i - statestart)), classificationRegistry.GetClassificationType("text")));

        }

        private State ClassifyToken(string token)
        {
            if (token.Length <= 0)
                return State.Default;

            if (TypeKeywords.Contains(token))
                return State.Type;

            if (LiteralKeywords.Contains(token))
                return State.Literal;

            if (ParsedProject != null)
            {
                if (ParsedProject.IsRecognizedFunction(token))
                    return State.IdentifierFunction;

                if (ParsedProject.IsRecognizedStructureType(token))
                    return State.IdentifierUDT;

                if (ParsedProject.IsRecognizedType(token))
                    return State.IdentifierType;
            }

            return State.Default;
        }

        #endregion
    }

    internal static class FileAndContentTypeDefinitions
    {

        // Disable "Field is never assigned to..." compiler's warning. Justification: the field is assigned by MEF.
#pragma warning disable 649

        [Export]
        [Name("EpochFile")]
        [BaseDefinition("text")]
        internal static ContentTypeDefinition EpochContentTypeDefinition;

        [Export]
        [FileExtension(".epoch")]
        [ContentType("EpochFile")]
        internal static FileExtensionToContentTypeDefinition EpochFileExtensionDefinition;

#pragma warning restore 649

    }
}

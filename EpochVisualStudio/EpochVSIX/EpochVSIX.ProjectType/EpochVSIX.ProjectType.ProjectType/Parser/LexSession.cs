//
// The Epoch Language Project
// Visual Studio Integration/Extension
//
// Implementation of a basic lexical analysis session.
//
// Lexical analysis divides a stream of input characters into a stream
// of tokens, where each token is an atomic unit of text, punctuation,
// or literal data such as numeric constants. A convenient side effect
// of this pass is that whitespace is removed, meaning that each token
// can be considered a logical blob regardless of source formatting.
//
// Another key effect of lexical analysis is the removal of commenting
// from the input text. This ensures that the next pass, i.e. parsing,
// can operate on a "purified" stream of tokens, without needing extra
// complexity to handle ignored characters from the input source.
//
// Note that currently this is the lexical analysis implementation for
// the IntelliSense parser, but NOT the lexer used by syntax coloring.
// For highlighting we use a slightly faster and simpler form of lexer
// that is designed to spit out highlighting spans (instead of tokens)
// which Visual Studio consumes directly. In the future it is probably
// worth considering merging the two implementations.
//

using System.Collections.Generic;
using System.Linq;

namespace EpochVSIX.Parser
{
    //
    // Lexical analysis implementation.
    //
    // This class is designed to operate semi-lazily. Given an input file
    // descriptor and its contents as a string, the lexer will divide the
    // buffer into tokens on demand, as requested by calling PeekToken or
    // ConsumeTokens. The code supports lookahead for an arbitrary number
    // of tokens. As tokens are requested, they are cached off for later.
    // When a token is no longer needed, e.g. if it has been successfully
    // parsed, it can be "consumed" using ConsumeTokens.
    //
    // While it may seem like an optimization, this design was chosen for
    // simplicity and ease-of-correctness more than speed. That said it's
    // not without performance perks.
    //
    // In particular, this design does not affect total parsing or lexing
    // duration - but it does dramatically improve the mean time to first
    // response, which can lead to a perception of better latency.
    //
    // Since we don't have to lex the entire document to return the first
    // few tokens from it, parsing can begin almost immediately after the
    // text buffer is loaded from disk. Our peak memory usage is also one
    // of the beneficiaries of this arrangement, in that we don't need to
    // lex more than the lookahead limit at any given time. (Note that an
    // unprincipled caller can lookahead without bound, which would serve
    // to compromise that particular benefit.)
    //
    // When it comes down to it, though, there's not much fancy about the
    // implementation. It's just a moderately large state machine with an
    // ad hoc structure versus a more rigid formalism.
    //
    class LexSession
    {

        private enum CharacterClass
        {
            White,                  // All whitespace characters; the lexer ensures none of these are part of any token.
            Comment,                // All text inside a code comment; tokens will never contain comments or fragments of comments.
            Identifier,             // Some kind of identifier or textual token, such as a function name, type, or keyword.
            Punctuation,            // Special symbols used in some syntactic constructs.
            PunctuationCompound,    // Punctuation symbols which span more than one character.
            Literal,                // An undelimited literal, such as a number or Boolean value.
            StringLiteral,          // A delimited string literal, i.e. surrounded by double quotes.
        };

        //
        // Source data to be lexed
        //
        private SourceFile File;
        private string Buffer;

        //
        // Cache of lexed tokens (for lookahead)
        //
        private List<Token> TokenCache;


        //
        // Internal lexical analysis state
        //
        private int LexIndex;
        private int LastTokenStart;
        private int LastLineIndex;
        private int CurrentLineIndex;
        private int CurrentColumnIndex;

        private CharacterClass LexState = CharacterClass.White;
        private CharacterClass PreviousState = CharacterClass.White;


        //
        // Construct and initiatilize a lexical analysis session.
        //
        public LexSession(SourceFile file, string buffer)
        {
            File = file;
            Buffer = buffer;
            TokenCache = new List<Token>();

            LexIndex = 0;
            LastTokenStart = 0;
            LastLineIndex = 0;
            CurrentLineIndex = 0;
            CurrentColumnIndex = 0;
        }

        
        //
        // Returns true if the input buffer has been fully lexed.
        //
        public bool Empty
        {
            get { return (Buffer == null) || (LexIndex >= Buffer.Length); }
        }

        //
        // Obtain the file path of the input data.
        //
        // Currently we always lex from data on disk, but this may
        // change once we have better support for unsaved files.
        //
        public string FileName
        {
            get { return File.Path; }
        }


        //
        // Peek ahead in the lexed token stream.
        //
        // Does not necessarily imply true "lookahead" since the
        // offset may be zero. But lookahead is a common use case
        // for this function. This will lazily lex tokens into the
        // token cache until the peek request can be satisfied.
        //
        public Token PeekToken(int offset)
        {
            while (TokenCache.Count <= offset)
                LexAdditionalToken();

            return TokenCache[offset];
        }

        //
        // Consume or "discard" tokens from the stream.
        //
        // Typically called after a series of tokens has been
        // successfully parsed into some syntactic construct.
        //
        public void ConsumeTokens(int numtokens)
        {
            while (TokenCache.Count < numtokens)
                LexAdditionalToken();

            TokenCache.RemoveRange(0, numtokens);
        }


        //
        // Internal helper for lexing a single token from the input data.
        //
        // This routine contains the actual lexical analysis state machine.
        // The implementation was converted from the one in the main Epoch
        // compiler. There are probably opportunities for cleanup.
        //
        // Note that if a token is requested but cannot be obtained from
        // the input data, this code will cache a null token. Callers and
        // users of the public API must be aware of the presence of nulls
        // in the token stream.
        //
        private void LexAdditionalToken()
        {
            int startcount = TokenCache.Count;

            while (!Empty)
            {
                char c = Buffer[LexIndex];

                if (LexState == CharacterClass.White)
                {
                    if ((c == '/') && (Buffer[LexIndex + 1] == '/'))
                    {
                        LexState = CharacterClass.Comment;
                    }
                    else if (!char.IsWhiteSpace(c))
                    {
                        LexState = LexerClassify(c, LexState);
                        LastTokenStart = LexIndex;
                    }
                    else if (c == '\n')
                    {
                        ++CurrentLineIndex;
                        CurrentColumnIndex = 0;
                    }
                }
                else if (LexState == CharacterClass.Identifier)
                {
                    bool notidentifier = false;
                    if (char.IsWhiteSpace(c))
                    {
                        notidentifier = true;
                        LexState = CharacterClass.White;
                    }
                    else if (LexerClassify(c, LexState) != CharacterClass.Identifier)
                    {
                        notidentifier = true;
                        LexState = LexerClassify(c, LexState);
                    }

                    if (notidentifier)
                        CacheToken(Buffer.Substring(LastTokenStart, LexIndex - LastTokenStart));
                }
                else if (LexState == CharacterClass.Punctuation)
                {
                    if (char.IsWhiteSpace(c))
                        LexState = CharacterClass.White;
                    else if (LexerClassify(c, LexState) != CharacterClass.Punctuation)
                        LexState = LexerClassify(c, LexState);

                    CacheToken(Buffer.Substring(LastTokenStart, LexIndex - LastTokenStart));
                    LastTokenStart = LexIndex;
                }
                else if (LexState == CharacterClass.PunctuationCompound)
                {
                    bool notcompound = false;
                    if (char.IsWhiteSpace(c))
                    {
                        notcompound = true;
                        LexState = CharacterClass.White;
                    }
                    else if (LexerClassify(c, LexState) != CharacterClass.PunctuationCompound)
                    {
                        notcompound = true;
                        LexState = LexerClassify(c, LexState);
                    }
                    else
                    {
                        if ((LexIndex - LastTokenStart) > 1)
                        {
                            string potentialtoken = Buffer.Substring(LastTokenStart, LexIndex - LastTokenStart);
                            if (!IsValidPunctuation(potentialtoken))
                            {
                                CacheToken(potentialtoken.Substring(0, potentialtoken.Length - 1));
                                LastTokenStart = LexIndex - 1;
                            }
                        }
                    }

                    if (notcompound)
                    {
                        if ((LexIndex - LastTokenStart) > 1)
                        {
                            string potentialtoken = Buffer.Substring(LastTokenStart, LexIndex - LastTokenStart);
                            if (!IsValidPunctuation(potentialtoken))
                            {
                                CacheToken(potentialtoken.Substring(0, potentialtoken.Length - 1));
                                LastTokenStart = LexIndex - 1;
                            }
                        }

                        CacheToken(Buffer.Substring(LastTokenStart, LexIndex - LastTokenStart));
                    }
                }
                else if (LexState == CharacterClass.Comment)
                {
                    if (c == '\r')
                        LexState = CharacterClass.White;
                    else if (c == '\n')
                    {
                        LexState = CharacterClass.White;
                        ++CurrentLineIndex;
                        CurrentColumnIndex = 0;
                    }
                }
                else if (LexState == CharacterClass.StringLiteral)
                {
                    if (c == '\"')
                    {
                        LexState = CharacterClass.White;
                        CacheToken(Buffer.Substring(LastTokenStart, LexIndex - LastTokenStart + 1));
                    }
                }
                else if (LexState == CharacterClass.Literal)
                {
                    bool notliteral = false;
                    if (char.IsWhiteSpace(c))
                    {
                        notliteral = true;
                        LexState = CharacterClass.White;
                    }
                    else if (LexerClassify(c, LexState) != CharacterClass.Literal)
                    {
                        notliteral = true;
                        LexState = LexerClassify(c, LexState);
                    }

                    if (notliteral)
                        CacheToken(Buffer.Substring(LastTokenStart, LexIndex - LastTokenStart));
                }

                // Hack for negated literals
                if (LexState == CharacterClass.PunctuationCompound)
                {
                    if (LexerClassify(Buffer[LexIndex + 1], LexState) == CharacterClass.Literal)
                        LexState = CharacterClass.Literal;
                }

                if (LexState != PreviousState)
                    LastTokenStart = LexIndex;

                PreviousState = LexState;
                ++LexIndex;

                if (CurrentLineIndex == LastLineIndex)
                    ++CurrentColumnIndex;

                LastLineIndex = CurrentLineIndex;

                if (TokenCache.Count != startcount)
                    return;
            }

            if ((LastTokenStart < Buffer.Length) && (LexState != CharacterClass.White))
                CacheToken(Buffer.Substring(LastTokenStart, Buffer.Length - LastTokenStart));

            if ((TokenCache.Count == startcount) && Empty)
            {
                TokenCache.Add(null);
            }
        }

        //
        // Internal helper for adding a token string to the cache.
        //
        private void CacheToken(string token)
        {
            var t = new Token
            {
                Text = token,
                Line = CurrentLineIndex,
                Column = CurrentColumnIndex,
                File = File
            };

            TokenCache.Add(t);
        }

        //
        // Internal helper - classifies a character for handling in the lexer state machine.
        //
        private CharacterClass LexerClassify(char c, CharacterClass currentclass)
        {
            if ("abcdefx".Contains(c))
            {
                if (currentclass == CharacterClass.Literal)
                    return CharacterClass.Literal;

                return CharacterClass.Identifier;
            }

            if ("0123456789".Contains(c))
            {
                if (currentclass == CharacterClass.Identifier)
                    return CharacterClass.Identifier;

                return CharacterClass.Literal;
            }

            if ("{}:(),;[]".Contains(c))
                return CharacterClass.Punctuation;

            if ("=&+-<>!".Contains(c))
                return CharacterClass.PunctuationCompound;

            if (c == '\"')
                return CharacterClass.StringLiteral;

            if (c == '.')
            {
                if (currentclass == CharacterClass.Literal)
                    return CharacterClass.Literal;

                if (currentclass == CharacterClass.PunctuationCompound)
                    return CharacterClass.PunctuationCompound;              // minor hack

                return CharacterClass.Punctuation;
            }

            if (char.IsWhiteSpace(c))
                return CharacterClass.White;

            return CharacterClass.Identifier;
        }

        //
        // Internal whitelist of valid compound punctuation.
        //
        private bool IsValidPunctuation(string token)
        {
            if (token == "==")
                return true;

            if (token == "!=")
                return true;

            if (token == "++")
                return true;

            if (token == "--")
                return true;

            if (token == "->")
                return true;

            if (token == "&&")
                return true;

            if (token == "+=")
                return true;

            if (token == "-=")
                return true;

            if (token == "=>")
                return true;

            return false;
        }
    }
}

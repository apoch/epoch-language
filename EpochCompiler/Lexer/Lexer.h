//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Lexical analyzer for parsing Epoch programs
//

#pragma once


typedef std::wstring::const_iterator positertype;


namespace Lexer
{
	enum tokenids 
	{
		ID_ANY = boost::spirit::lex::min_token_id + 10,
	};

	template <typename LexerT>
	struct EpochLexer : public boost::spirit::lex::lexer<LexerT>
	{
		typedef boost::spirit::lex::token_def<boost::spirit::lex::omit, wchar_t> ConsumedToken;
		typedef boost::spirit::lex::token_def<boost::iterator_range<positertype>, wchar_t> Token;

		EpochLexer();

		ConsumedToken StructureDef;
		ConsumedToken GlobalDef;
		ConsumedToken AliasDef;

		ConsumedToken OpenBracket;
		ConsumedToken CloseBracket;
		ConsumedToken OpenParens;
		ConsumedToken CloseParens;
		ConsumedToken Dot;
		ConsumedToken Comma;
		ConsumedToken Colon;
		ConsumedToken Arrow;
		ConsumedToken Ref;
		ConsumedToken OpenBrace;
		ConsumedToken CloseBrace;

		Token Equals;

		Token StringIdentifier;
		Token StringLiteral;
		Token IntegerLiteral;
		Token HexLiteral;
		Token RealLiteral;

		ConsumedToken Comment;
		ConsumedToken Whitespace;
	};

	typedef boost::spirit::lex::lexertl::token<positertype> TokenType;
	typedef boost::spirit::lex::lexertl::actor_lexer<TokenType> LexerType;

	typedef EpochLexer<LexerType> EpochLexerT;
	typedef EpochLexerT::iterator_type TokenIterT;

}


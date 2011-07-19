#pragma once

#include <sstream>


namespace Lexer
{

	template <typename LexerT>
	EpochLexer<LexerT>::EpochLexer()
	{
		StructureDef = L"\"structure\"";
		GlobalDef = L"\"global\"";

		StringIdentifier = L"[a-zA-Z]\\w*";
		StringLiteral = L"\\\"[^\\\"]*\\\"";
		HexLiteral = L"\"0x\"[0-9a-fA-F]+";
		IntegerLiteral = L"[\\-]?[0-9]+";
		RealLiteral = L"[\\-]?[0-9]+\".\"[0-9]+";

		Whitespace = L"\\s+";
		self += Whitespace[boost::spirit::lex::_pass = boost::spirit::lex::pass_flags::pass_ignore];

		Comment = L"\"//\".*$";
		self += Comment[boost::spirit::lex::_pass = boost::spirit::lex::pass_flags::pass_ignore];

		OpenBracket = L"\\{";
		CloseBracket = L"\\}";
		OpenParens = L"\\(";
		CloseParens = L"\\)";
		Dot = L"\".\"";
		Comma = L"\",\"";
		Equals = L"\"=\"";
		Colon = L"\":\"";
		Arrow = L"\"->\"";
		Ref = L"\"ref\"";
		OpenBrace = L"\\[";
		CloseBrace = L"\\]";

		self.add
			(Comma)
			(OpenParens)
			(CloseParens)
			(OpenBracket)
			(CloseBracket)
			(Equals)
			(Dot)
			(Colon)
			(Arrow)
			(OpenBrace)
			(CloseBrace)
			(L"[\\.\\+\\-=\\!]+")
			(IntegerLiteral)
			(HexLiteral)
			(RealLiteral)
			(StringLiteral)
			(Ref)
			(StructureDef)
			(GlobalDef)
			(StringIdentifier)
			(L".", ID_ANY)
		;
	}

}


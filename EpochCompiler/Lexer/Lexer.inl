#pragma once

#include <sstream>


namespace Lexer
{

	template <typename LexerT>
	EpochLexer<LexerT>::EpochLexer()
	{
    
		StructureDef = L"\"structure\"";
		GlobalDef = L"\"global\"";
		AliasDef = L"\"alias\"";
		TypeDef = L"\"type\"";

		StringIdentifier = L"[a-zA-Z]\\w*";
		StringLiteral = L"\\\"[^\\\"]*\\\"";
		HexLiteral = L"\"0x\"[0-9a-fA-F]+";
		IntegerLiteral = L"[\\-]?[0-9]+";
		RealLiteral = L"[\\-]?[0-9]+\".\"[0-9]+";

		Whitespace = L"\\s+";
        this->self += Whitespace[boost::spirit::lex::_pass = boost::spirit::lex::pass_flags::pass_ignore];

		Comment = L"\"//\".*$";
        this->self += Comment[boost::spirit::lex::_pass = boost::spirit::lex::pass_flags::pass_ignore];

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
		Pipe = L"\\|";

		this->self.add
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
			(Pipe)
			(HexLiteral)
			(RealLiteral)
			(StringLiteral)
			(L"[\\.\\+\\-=\\!]+")
			(IntegerLiteral)
			(StructureDef)
			(GlobalDef)
			(AliasDef)
			(TypeDef)
			(Ref)
			(StringIdentifier)
			(L".", ID_ANY)
		;
	}

}


//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// This module defines constants and a boost::spirit error handling
// functor for dealing with syntax and other parse errors.
//

#pragma once

namespace Parser
{

	//
	// Constants for tracking various types of syntax errors
	//
	enum SyntaxErrors
	{
		SYNTAXERROR_STATEMENTORCONTROL,

		SYNTAXERROR_FUNCTIONDEFINITION,
		SYNTAXERROR_VARIABLEDEFINITION,

		SYNTAXERROR_BLOCK,
		SYNTAXERROR_EXPRESSION,

		SYNTAXERROR_QUOTE,
		SYNTAXERROR_CLOSEBRACE,
		SYNTAXERROR_OPENPARENS,
		SYNTAXERROR_CLOSEPARENS,
		SYNTAXERROR_COMMA,

		SYNTAXERROR_INTEGERLITERAL,
		SYNTAXERROR_REALLITERAL,
		SYNTAXERROR_BOOLEANLITERAL,
		SYNTAXERROR_STRINGLITERAL,
	};


	//
	// Handle detected syntax errors - display an error message and halt parsing.
	//
	struct SyntaxErrorHandler
	{
		SyntaxErrorHandler(Parser::ParserState& state)
			: State(state)
		{ }

		template <typename ScannerType, typename ErrorType>
		boost::spirit::classic::error_status<> operator () (const ScannerType& scanner, const ErrorType& error) const
		{
			boost::spirit::classic::file_position pos = error.where.get_position();
			std::wstring filename = widen(pos.file);

			UI::OutputStream output;

			switch(error.descriptor)
			{
			case SYNTAXERROR_STATEMENTORCONTROL:
				output << L"Syntax error - expected a statement or control operator:\n";
				break;

			case SYNTAXERROR_FUNCTIONDEFINITION:
				output << L"Syntax error - expected function definition:\n";
				break;

			case SYNTAXERROR_BLOCK:
				output << L"Syntax error - expected a code block:\n";
				break;

			case SYNTAXERROR_VARIABLEDEFINITION:
				output << L"Syntax error - expected a variable definition:\n";
				break;

			case SYNTAXERROR_EXPRESSION:
				output << L"Syntax error - expected an expression:\n";
				break;

			case SYNTAXERROR_QUOTE:
				output << L"Syntax error - expected a closing quote:\n";
				break;

			case SYNTAXERROR_CLOSEBRACE:
				output << L"Syntax error - expected a closing brace:\n";
				break;

			case SYNTAXERROR_OPENPARENS:
				output << L"Syntax error - expected an opening parenthesis:\n";
				break;

			case SYNTAXERROR_CLOSEPARENS:
				output << L"Syntax error - expected a closing parenthesis:\n";
				break;

			case SYNTAXERROR_INTEGERLITERAL:
				output << L"Syntax error - expected an integer value:\n";
				break;

			case SYNTAXERROR_REALLITERAL:
				output << L"Syntax error - expected a real value:\n";
				break;

			case SYNTAXERROR_BOOLEANLITERAL:
				output << L"Syntax error - expected a boolean value:\n";
				break;

			case SYNTAXERROR_STRINGLITERAL:
				output << L"Syntax error - expected a string value:\n";
				break;

			default:
				output << L"Syntax error:\n";
				break;
			}

			output << L"File: " << filename << L" Line: " << pos.line << L" Column: " << pos.column << std::endl;

			State.DumpCodeLine(pos.line, pos.column, Config::TabWidth);
			State.ParseFailed = true;

			// Move past the broken line and continue parsing
			while(scanner.first.get_position().line <= pos.line)
				++scanner;

			return boost::spirit::classic::error_status<>(boost::spirit::classic::error_status<>::retry);
		}

	// Internal binding to state wrapper
	protected:
		Parser::ParserState& State;
	};

}


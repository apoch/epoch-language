//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parser error handling routines
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"

#include "User Interface/Output.h"

#include "Utility/Strings.h"

#include "Configuration/RuntimeOptions.h"


using namespace Parser;


//
// Display a fatal error message to the user.
//
// Most fatal errors should not actually stop the parsing process. We
// want to catch as many errors per pass as we can, and report them.
// A truly fatal, kill-everything-right-now failure should be handled
// by throwing an exception.
//
void ParserState::ReportFatalError(const char* what)
{
	ParseFailed = true;

	UI::OutputStream output;
	output << UI::lightred << what << UI::resetcolor << std::endl;
	const boost::spirit::classic::file_position pos = GetParsePosition().get_position();
	output << L"File: " << widen(pos.file) << L" Line: " << pos.line << L" Column: " << pos.column << std::endl;
	DumpCodeLine(pos.line, pos.column, Config::TabWidth);
}

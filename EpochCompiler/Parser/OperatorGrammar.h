#pragma once

#include "Compiler/Abstract Syntax Tree/Identifiers.h"


struct SymbolTable : public boost::spirit::qi::symbols<wchar_t, AST::IdentifierT>
{
};


#pragma once

#include "Compiler/AbstractSyntaxTree.h"


struct SymbolTable : public boost::spirit::qi::symbols<wchar_t, AST::IdentifierT>
{
};


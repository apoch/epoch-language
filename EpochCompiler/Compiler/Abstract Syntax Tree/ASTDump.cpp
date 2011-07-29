//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST traverser implementation that simply dumps the AST contents
// into human-readable textual format. Supports output to anything
// that is a std::wostream.
//

#include "pch.h"

#include "Compiler/Abstract Syntax Tree/ASTDump.h"
#include "Compiler/Abstract Syntax Tree/Literals.h"
#include "Compiler/Abstract Syntax Tree/Parenthetical.h"
#include "Compiler/Abstract Syntax Tree/Expression.h"
#include "Compiler/Abstract Syntax Tree/Statement.h"
#include "Compiler/Abstract Syntax Tree/Entities.h"

#include <iterator>			// for std::fill_n


using namespace ASTTraverse;


void DumpToStream::EntryHelper::operator () (AST::Undefined&)
{
	// Don't output anything; this generally should only happen
	// for statements with no parameters, e.g. foo()
}


void DumpToStream::EntryHelper::operator () (AST::Program& program)
{
	self->Indent();
	self->TheStream << L"Program AST" << std::endl;
	++self->Indentation;
}

void DumpToStream::ExitHelper::operator () (AST::Program& program)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of program" << std::endl;
}


void DumpToStream::EntryHelper::operator () (AST::Structure& structure)
{
	self->Indent();
	self->TheStream << L"Structure " << structure.Identifier << std::endl;
	++self->Indentation;
}

void DumpToStream::ExitHelper::operator () (AST::Structure& structure)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of structure" << std::endl;
}


void DumpToStream::EntryHelper::operator () (AST::StructureMemberVariable& variable)
{
	self->Indent();
	self->TheStream << L"Member variable " << variable.Name << L" of type " << variable.Type << std::endl;
}

void DumpToStream::EntryHelper::operator () (AST::StructureMemberFunctionRef& funcref)
{
	self->Indent();
	self->TheStream << L"Function reference " << funcref.Name << std::endl;
	// TODO - dump the signature here
}




void DumpToStream::EntryHelper::operator () (AST::Function& function)
{
	self->Indent();
	self->TheStream << L"Function " << function.Name << std::endl;
	++self->Indentation;
}

void DumpToStream::ExitHelper::operator () (AST::Function& function)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of function " << function.Name << std::endl;
}


void DumpToStream::EntryHelper::operator () (AST::FunctionParameter& param)
{
	self->Indent();
	self->TheStream << L"Parameter" << std::endl;
	++self->Indentation;
}

void DumpToStream::ExitHelper::operator () (AST::FunctionParameter& param)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of parameter" << std::endl;
}

void DumpToStream::EntryHelper::operator () (AST::NamedFunctionParameter& param)
{
	self->Indent();
	self->TheStream << param.Name << L" of type " << param.Type << std::endl;
}


void DumpToStream::EntryHelper::operator () (AST::Expression& expression)
{
	self->Indent();
	self->TheStream << L"Expression" << std::endl;
	++self->Indentation;
}

void DumpToStream::ExitHelper::operator () (AST::Expression& expression)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of expression" << std::endl;
}


void DumpToStream::EntryHelper::operator () (AST::ExpressionComponent& exprcomponent)
{
	self->Indent();
	self->TheStream << L"Expression component" << std::endl;
	++self->Indentation;
}

void DumpToStream::ExitHelper::operator () (AST::ExpressionComponent& exprcomponent)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of component" << std::endl;
}

void DumpToStream::EntryHelper::operator () (AST::ExpressionFragment& exprfragment)
{
	self->Indent();
	self->TheStream << L"Expression fragment" << std::endl;
	++self->Indentation;
}

void DumpToStream::ExitHelper::operator () (AST::ExpressionFragment& exprfragment)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of fragment" << std::endl;
}


void DumpToStream::EntryHelper::operator () (AST::Statement& statement)
{
	self->Indent();
	self->TheStream << L"Statement " << statement.Identifier << std::endl;
	++self->Indentation;
}

void DumpToStream::ExitHelper::operator () (AST::Statement& statement)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of statement" << std::endl;
}

void DumpToStream::EntryHelper::operator () (AST::PreOperatorStatement& statement)
{
	self->Indent();
	self->TheStream << L"Preopstatement " << statement.Operator;
	for(std::vector<AST::IdentifierT, Memory::OneWayAlloc<AST::IdentifierT> >::const_iterator iter = statement.Operand.Content->Container.begin(); iter != statement.Operand.Content->Container.end(); ++iter)
		self->TheStream << L" " << *iter;
	self->TheStream << std::endl;
}

void DumpToStream::ExitHelper::operator () (AST::PreOperatorStatement& statement)
{
}

void DumpToStream::EntryHelper::operator () (AST::PostOperatorStatement& statement)
{
	self->Indent();
	self->TheStream << L"Postopstatement ";
	for(std::vector<AST::IdentifierT, Memory::OneWayAlloc<AST::IdentifierT> >::const_iterator iter = statement.Operand.Content->Container.begin(); iter != statement.Operand.Content->Container.end(); ++iter)
		self->TheStream << L" " << *iter;
	self->TheStream  << statement.Operator << L" " << std::endl;
}

void DumpToStream::ExitHelper::operator () (AST::PostOperatorStatement& statement)
{
}

void DumpToStream::EntryHelper::operator () (AST::CodeBlockEntry& blockentry)
{
	self->Indent();
}

void DumpToStream::ExitHelper::operator () (AST::CodeBlockEntry& blockentry)
{
	self->TheStream << std::endl;
}

void DumpToStream::EntryHelper::operator () (AST::CodeBlock& block)
{
	self->Indent();
	self->TheStream << L"Code block" << std::endl;
	++self->Indentation;
}

void DumpToStream::ExitHelper::operator () (AST::CodeBlock& block)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of block" << std::endl;
}



void DumpToStream::EntryHelper::operator () (AST::Assignment& assignment)
{
	self->Indent();
	self->TheStream << L"Assignment using " << assignment.Operator << std::endl;
	++self->Indentation;
}

void DumpToStream::ExitHelper::operator () (AST::Assignment& assignment)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of assignment" << std::endl;
}



void DumpToStream::EntryHelper::operator () (AST::Entity& entity)
{
	self->Indent();
	self->TheStream << L"Entity " << entity.Identifier << std::endl;
	++self->Indentation;
}

void DumpToStream::ExitHelper::operator () (AST::Entity& entity)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of entity" << std::endl;
}


void DumpToStream::EntryHelper::operator () (AST::ChainedEntity& entity)
{
	self->Indent();
	self->TheStream << L"Chained Entity " << entity.Identifier << std::endl;
	++self->Indentation;
}

void DumpToStream::ExitHelper::operator () (AST::ChainedEntity& entity)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of chained entity" << std::endl;
}


void DumpToStream::EntryHelper::operator () (AST::PostfixEntity& entity)
{
	self->Indent();
	self->TheStream << L"Postfix Entity " << entity.Identifier << std::endl;
	++self->Indentation;
}

void DumpToStream::ExitHelper::operator () (AST::PostfixEntity& entity)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of postfix entity" << std::endl;
}


void DumpToStream::EntryHelper::operator () (AST::IdentifierT& identifier)
{
	self->Indent();
	self->TheStream << identifier << std::endl;
}

void DumpToStream::ExitHelper::operator () (AST::IdentifierT& identifier)
{
}


void DumpToStream::EntryHelper::operator () (AST::FunctionReferenceSignature& refsig)
{
	self->Indent();
	self->TheStream << L"Function signature named " << refsig.Identifier;
	self->TheStream << L" with parameter types:" << std::endl;
	++self->Indentation;

	for(std::vector<AST::IdentifierT, Memory::OneWayAlloc<AST::IdentifierT> >::const_iterator iter = refsig.ParamTypes.Content->Container.begin(); iter != refsig.ParamTypes.Content->Container.end(); ++iter)
	{
		self->Indent();
		self->TheStream << *iter << std::endl;
	}

	--self->Indentation;
	self->Indent();
	self->TheStream << L"Returning " << refsig.ReturnType << std::endl;
}


void DumpToStream::EntryHelper::operator () (Markers::FunctionReturnExpression&)
{
	self->Indent();
	self->TheStream << L"Function return" << std::endl;
	++self->Indentation;
}

void DumpToStream::ExitHelper::operator () (Markers::FunctionReturnExpression&)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of return" << std::endl;
}



void DumpToStream::Indent()
{
	std::fill_n(std::ostream_iterator<wchar_t, wchar_t>(TheStream), Indentation, L' ');
}


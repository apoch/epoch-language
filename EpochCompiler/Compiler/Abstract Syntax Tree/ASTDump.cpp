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


//
// Begin traversing a node with no defined type
//
// There are two primary situations in which this can occur. First, and most
// common, is when an optional AST fragment is omitted; for instance, when
// invoking a function with no parameters, the parameters branch of the AST
// will be Undefined.
//
// The second possible situation is if the parser has failed to synthesize
// a correct attribute for the AST nodes; this should generally be caught as
// a parse error and we should never get as far as walking the AST.
//
// In the former case, we don't want to emit anything, because that's the
// most valid representation of "nothing." In the latter case, it would be
// nice to throw an error, but unfortunately we don't have enough context to
// differentiate the two scenarios as this point, so we'll just be silent.
//
void DumpToStream::EntryHelper::operator () (AST::Undefined&)
{
	// Don't output anything
}


//
// Begin traversing a program node, which represents the root of the AST
//
// In practice we will only usually see one Program per, well, program;
// however, this might change in the future as support for separate compilation
// is added to the compiler. Therefore we do not enforce the assumption that
// Program nodes are singular.
//
void DumpToStream::EntryHelper::operator () (AST::Program& program)
{
	self->Indent();
	self->TheStream << L"Program AST" << std::endl;
	++self->Indentation;
}

//
// Finish traversing a program node
//
void DumpToStream::ExitHelper::operator () (AST::Program& program)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of program" << std::endl;
}


//
// Begin traversing a structure definition node
//
void DumpToStream::EntryHelper::operator () (AST::Structure& structure)
{
	self->Indent();
	self->TheStream << L"Structure " << structure.Identifier << std::endl;
	++self->Indentation;
}

//
// Finish traversing a structure definition node
//
void DumpToStream::ExitHelper::operator () (AST::Structure& structure)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of structure" << std::endl;
}


//
// Traverse a node defining a member variable in a structure
//
void DumpToStream::EntryHelper::operator () (AST::StructureMemberVariable& variable)
{
	self->Indent();
	self->TheStream << L"Member variable " << variable.Name << L" of type " << variable.Type << std::endl;
}

//
// Traverse a node defining a member function reference in a structure
//
void DumpToStream::EntryHelper::operator () (AST::StructureMemberFunctionRef& funcref)
{
	self->Indent();
	self->TheStream << L"Function reference " << funcref.Name << std::endl;
	// TODO - dump the signature here
}


//
// Begin traversing a node that defines a function
//
void DumpToStream::EntryHelper::operator () (AST::Function& function)
{
	self->Indent();
	self->TheStream << L"Function " << function.Name << std::endl;
	++self->Indentation;
}

//
// Finish traversing a node that defines a function
//
void DumpToStream::ExitHelper::operator () (AST::Function& function)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of function " << function.Name << std::endl;
}


//
// Begin traversing a node that defines a function parameter
//
void DumpToStream::EntryHelper::operator () (AST::FunctionParameter& param)
{
	self->Indent();
	self->TheStream << L"Parameter" << std::endl;
	++self->Indentation;
}

//
// Finish traversing a node that defines a function parameter
//
void DumpToStream::ExitHelper::operator () (AST::FunctionParameter& param)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of parameter" << std::endl;
}


//
// Traverse a node that corresponds to a named function parameter
//
// We can safely treat these as leaf nodes, hence the direct access of the
// node's properties to retrieve its contents.
//
void DumpToStream::EntryHelper::operator () (AST::NamedFunctionParameter& param)
{
	self->Indent();
	self->TheStream << param.Name << L" of type " << param.Type << std::endl;
}


//
// Begin traversing a node that corresponds to an expression
//
void DumpToStream::EntryHelper::operator () (AST::Expression& expression)
{
	self->Indent();
	self->TheStream << L"Expression" << std::endl;
	++self->Indentation;
}

//
// Finish traversing an expression node
//
void DumpToStream::ExitHelper::operator () (AST::Expression& expression)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of expression" << std::endl;
}

//
// Begin traversing an expression component node (see AST definitions for details)
//
void DumpToStream::EntryHelper::operator () (AST::ExpressionComponent& exprcomponent)
{
	self->Indent();
	self->TheStream << L"Expression component" << std::endl;
	++self->Indentation;
}

//
// Finish traversing an expression component node
//
void DumpToStream::ExitHelper::operator () (AST::ExpressionComponent& exprcomponent)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of component" << std::endl;
}

//
// Begin traversing an expression fragment node (see AST definitions for details)
//
void DumpToStream::EntryHelper::operator () (AST::ExpressionFragment& exprfragment)
{
	self->Indent();
	self->TheStream << L"Expression fragment" << std::endl;
	++self->Indentation;
}

//
// Finish traversing an expression fragment node
//
void DumpToStream::ExitHelper::operator () (AST::ExpressionFragment& exprfragment)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of fragment" << std::endl;
}


//
// Begin traversing a statement node
//
void DumpToStream::EntryHelper::operator () (AST::Statement& statement)
{
	self->Indent();
	self->TheStream << L"Statement " << statement.Identifier << std::endl;
	++self->Indentation;
}

//
// Finish traversing a statement node
//
void DumpToStream::ExitHelper::operator () (AST::Statement& statement)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of statement" << std::endl;
}


//
// Begin traversing a node corresponding to a pre-operation statement
//
void DumpToStream::EntryHelper::operator () (AST::PreOperatorStatement& statement)
{
	self->Indent();
	self->TheStream << L"Preopstatement " << statement.Operator << std::endl;
	++self->Indentation;
}

//
// Finish traversing a node corresponding to a pre-operation statement
//
void DumpToStream::ExitHelper::operator () (AST::PreOperatorStatement& statement)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of preopstatement" << std::endl;
}


//
// Begin traversing a node corresponding to a post-operation statement
//
void DumpToStream::EntryHelper::operator () (AST::PostOperatorStatement& statement)
{
	self->Indent();
	self->TheStream << L"Postopstatement " << statement.Operator << std::endl;
	++self->Indentation;
}

//
// Finish traversing a node corresponding to a post-operation statement
//
void DumpToStream::ExitHelper::operator () (AST::PostOperatorStatement& statement)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of postopstatement" << std::endl;
}


//
// Begin traversing a node containing a code block
//
void DumpToStream::EntryHelper::operator () (AST::CodeBlock& block)
{
	self->Indent();
	self->TheStream << L"Code block" << std::endl;
	++self->Indentation;
}

//
// Finish traversing a code block node
//
void DumpToStream::ExitHelper::operator () (AST::CodeBlock& block)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of block" << std::endl;
}


//
// Begin traversing a node corresponding to an assignment
//
void DumpToStream::EntryHelper::operator () (AST::Assignment& assignment)
{
	self->Indent();
	self->TheStream << L"Assignment using " << assignment.Operator << std::endl;
	++self->Indentation;
}

//
// Finish traversing a node corresponding to an assignment
//
void DumpToStream::ExitHelper::operator () (AST::Assignment& assignment)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of assignment" << std::endl;
}


//
// Begin traversing a node representing an entity invocation
//
void DumpToStream::EntryHelper::operator () (AST::Entity& entity)
{
	self->Indent();
	self->TheStream << L"Entity " << entity.Identifier << std::endl;
	++self->Indentation;
}

//
// Finish traversing a node representing an entity invocation
//
void DumpToStream::ExitHelper::operator () (AST::Entity& entity)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of entity" << std::endl;
}

//
// Begin traversing a node containing a chained entity invocation
//
void DumpToStream::EntryHelper::operator () (AST::ChainedEntity& entity)
{
	self->Indent();
	self->TheStream << L"Chained Entity " << entity.Identifier << std::endl;
	++self->Indentation;
}

//
// Finish traversing a node containing a chained entity invocation
//
void DumpToStream::ExitHelper::operator () (AST::ChainedEntity& entity)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of chained entity" << std::endl;
}

//
// Begin traversing a postfix entity invocation node
//
void DumpToStream::EntryHelper::operator () (AST::PostfixEntity& entity)
{
	self->Indent();
	self->TheStream << L"Postfix Entity " << entity.Identifier << std::endl;
	++self->Indentation;
}

//
// Finish traversing a postfix entity invocation node
//
void DumpToStream::ExitHelper::operator () (AST::PostfixEntity& entity)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of postfix entity" << std::endl;
}


//
// Traverse a node containing an identifier
//
// Note that this is not necessarily always a leaf node; identifiers can
// be attached to entity invocations, function calls, etc. and therefore
// this needs to be able to handle all situations in which an identifier
// might appear.
//
void DumpToStream::EntryHelper::operator () (AST::IdentifierT& identifier)
{
	self->Indent();
	self->TheStream << identifier << std::endl;
}


//
// Traverse a node representing a function reference signature; this is
// typically used when passing function references to higher-order functions
// in the program being compiled.
//
// We can safely treat this as an atomic leaf node, hence the direct access
// of the node's properties when generating output.
//
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


//
// Traverse a special marker that indicates the subsequent nodes belong
// to the return expression definition of a function
//
void DumpToStream::EntryHelper::operator () (Markers::FunctionReturnExpression&)
{
	self->Indent();
	self->TheStream << L"Function return" << std::endl;
	++self->Indentation;
}

//
// Finish traversing a function's return expression
//
void DumpToStream::ExitHelper::operator () (Markers::FunctionReturnExpression&)
{
	--self->Indentation;
	self->Indent();
	self->TheStream << L"End of return" << std::endl;
}


//
// Helper function: indent the output to the current level
//
void DumpToStream::Indent()
{
	std::fill_n(std::ostream_iterator<wchar_t, wchar_t>(TheStream), Indentation, L' ');
}


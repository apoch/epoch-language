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
// Helper functions for reducing boilerplate code
//
namespace
{

	//
	// Pad to the current indentation level, output some raw text,
	// then increase the indentation level for subsequent lines.
	//
	void PrintAndIndent(DumpToStream* self, const wchar_t* plaintext)
	{
		self->Indent();
		self->TheStream << plaintext << std::endl;
		++self->Indentation;
	}

	//
	// Variant of the above for outputting an identifier with the text
	//
	void PrintAndIndent(DumpToStream* self, const wchar_t* plaintext, const AST::IdentifierT& identifier)
	{
		self->Indent();
		self->TheStream << plaintext << identifier << std::endl;
		++self->Indentation;
	}

	//
	// Variant of the above with suffix text
	//
	void PrintAndIndent(DumpToStream* self, const wchar_t* plaintext, const AST::IdentifierT& identifier, const wchar_t* suffixtext)
	{
		self->Indent();
		self->TheStream << plaintext << identifier << suffixtext << std::endl;
		++self->Indentation;
	}

	//
	// Print a line involving two identifiers separated with some text
	//
	void Print(DumpToStream* self, const wchar_t* leadertext, const AST::IdentifierT& firstidentifier, const wchar_t* conjunctiontext, const AST::IdentifierT& secondidentifier)
	{
		self->Indent();
		self->TheStream << leadertext << firstidentifier << conjunctiontext << secondidentifier << std::endl;
	}

	void PrintAndIndent(DumpToStream* self, const wchar_t* leadertext, const AST::IdentifierT& firstidentifier, const wchar_t* conjunctiontext, const AST::IdentifierT& secondidentifier)
	{
		Print(self, leadertext, firstidentifier, conjunctiontext, secondidentifier);
		++self->Indentation;
	}

	//
	// Print a lone identifier
	//
	void Print(DumpToStream* self, const AST::IdentifierT& identifier)
	{
		self->Indent();
		self->TheStream << identifier << std::endl;
	}

	//
	// Print a sequence of identifiers
	//
	void Print(DumpToStream* self, const AST::IdentifierList& identifiers)
	{
		for(AST::IdentifierList::const_iterator iter = identifiers.Content->Container.begin(); iter != identifiers.Content->Container.end(); ++iter)
			Print(self, *iter);
	}

	//
	// Decrease the current indentation level, pad to that level,
	// then output some raw text.
	//
	void UnindentAndPrint(DumpToStream* self, const wchar_t* plaintext)
	{
		--self->Indentation;
		self->Indent();
		self->TheStream << plaintext << std::endl;
	}

	//
	// Variant of the above, including an identifier
	//
	void UnindentAndPrint(DumpToStream* self, const wchar_t* plaintext, const AST::IdentifierT& identifier)
	{
		--self->Indentation;
		self->Indent();
		self->TheStream << plaintext << identifier << std::endl;
	}

}


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
void DumpToStream::EntryHelper::operator () (AST::Program&)
{
	PrintAndIndent(self, L"Program AST");
}

//
// Finish traversing a program node
//
void DumpToStream::ExitHelper::operator () (AST::Program&)
{
	UnindentAndPrint(self, L"End of program");
}


//
// Begin traversing a structure definition node
//
void DumpToStream::EntryHelper::operator () (AST::Structure& structure)
{
	PrintAndIndent(self, L"Structure ", structure.Identifier);
}

//
// Finish traversing a structure definition node
//
void DumpToStream::ExitHelper::operator () (AST::Structure&)
{
	UnindentAndPrint(self, L"End of structure");
}


//
// Traverse a node defining a member variable in a structure
//
void DumpToStream::EntryHelper::operator () (AST::StructureMemberVariable& variable)
{
	Print(self, L"Member variable ", variable.Name, L" of type ", variable.Type);
}

//
// Traverse a node defining a member function reference in a structure
//
void DumpToStream::EntryHelper::operator () (AST::StructureMemberFunctionRef& funcref)
{
	PrintAndIndent(self, L"Function reference named ", funcref.Name, L" with parameter types:");
}


//
// Begin traversing a node that defines a function
//
void DumpToStream::EntryHelper::operator () (AST::Function& function)
{
	PrintAndIndent(self, L"Function ", function.Name);
}

//
// Finish traversing a node that defines a function
//
void DumpToStream::ExitHelper::operator () (AST::Function& function)
{
	UnindentAndPrint(self, L"End of function ", function.Name);
}


//
// Begin traversing a node that defines a function parameter
//
void DumpToStream::EntryHelper::operator () (AST::FunctionParameter&)
{
	PrintAndIndent(self, L"Parameter");
}

//
// Finish traversing a node that defines a function parameter
//
void DumpToStream::ExitHelper::operator () (AST::FunctionParameter&)
{
	UnindentAndPrint(self, L"End of parameter");
}


//
// Traverse a node that corresponds to a named function parameter
//
// We can safely treat these as leaf nodes, hence the direct access of the
// node's properties to retrieve its contents.
//
void DumpToStream::EntryHelper::operator () (AST::NamedFunctionParameter& param)
{
	Print(self, L"Parameter ", param.Name, L" of type ", param.Type);
}


void DumpToStream::EntryHelper::operator () (AST::FunctionTag& tag)
{
	PrintAndIndent(self, L"Tag ", tag.TagName);
}

void DumpToStream::ExitHelper::operator () (AST::FunctionTag&)
{
	UnindentAndPrint(self, L"End of tag");
}


//
// Begin traversing a node that corresponds to an expression
//
void DumpToStream::EntryHelper::operator () (AST::Expression&)
{
	PrintAndIndent(self, L"Expression");
}

//
// Finish traversing an expression node
//
void DumpToStream::ExitHelper::operator () (AST::Expression&)
{
	UnindentAndPrint(self, L"End of expression");
}

//
// Begin traversing an expression component node (see AST definitions for details)
//
void DumpToStream::EntryHelper::operator () (AST::ExpressionComponent&)
{
	PrintAndIndent(self, L"Expression component");
}

//
// Finish traversing an expression component node
//
void DumpToStream::ExitHelper::operator () (AST::ExpressionComponent&)
{
	UnindentAndPrint(self, L"End of component");
}

//
// Begin traversing an expression fragment node (see AST definitions for details)
//
void DumpToStream::EntryHelper::operator () (AST::ExpressionFragment&)
{
	PrintAndIndent(self, L"Expression fragment");
}

//
// Finish traversing an expression fragment node
//
void DumpToStream::ExitHelper::operator () (AST::ExpressionFragment&)
{
	UnindentAndPrint(self, L"End of fragment");
}


//
// Begin traversing a statement node
//
void DumpToStream::EntryHelper::operator () (AST::Statement& statement)
{
	PrintAndIndent(self, L"Statement ", statement.Identifier);
}

//
// Finish traversing a statement node
//
void DumpToStream::ExitHelper::operator () (AST::Statement&)
{
	UnindentAndPrint(self, L"End of statement");
}


//
// Begin traversing a node corresponding to a pre-operation statement
//
void DumpToStream::EntryHelper::operator () (AST::PreOperatorStatement& statement)
{
	PrintAndIndent(self, L"Preopstatement ", statement.Operator);
}

//
// Finish traversing a node corresponding to a pre-operation statement
//
void DumpToStream::ExitHelper::operator () (AST::PreOperatorStatement&)
{
	UnindentAndPrint(self, L"End of preopstatement");
}


//
// Begin traversing a node corresponding to a post-operation statement
//
void DumpToStream::EntryHelper::operator () (AST::PostOperatorStatement& statement)
{
	PrintAndIndent(self, L"Postopstatement ", statement.Operator);
}

//
// Finish traversing a node corresponding to a post-operation statement
//
void DumpToStream::ExitHelper::operator () (AST::PostOperatorStatement&)
{
	UnindentAndPrint(self, L"End of postopstatement");
}


//
// Begin traversing a node containing a code block
//
void DumpToStream::EntryHelper::operator () (AST::CodeBlock&)
{
	PrintAndIndent(self, L"Code block");
}

//
// Finish traversing a code block node
//
void DumpToStream::ExitHelper::operator () (AST::CodeBlock&)
{
	UnindentAndPrint(self, L"End of block");
}


//
// Begin traversing a node corresponding to an assignment
//
void DumpToStream::EntryHelper::operator () (AST::Assignment& assignment)
{
	PrintAndIndent(self, L"Assignment using ", assignment.Operator);
}

//
// Finish traversing a node corresponding to an assignment
//
void DumpToStream::ExitHelper::operator () (AST::Assignment&)
{
	UnindentAndPrint(self, L"End of assignment");
}


//
// Begin traversing a node corresponding to an initialization
//
void DumpToStream::EntryHelper::operator () (AST::Initialization& initialization)
{
	PrintAndIndent(self, L"Initialization of variable ", initialization.LHS, L" with type ", initialization.TypeSpecifier);
}

//
// Finish traversing a node corresponding to an initialization
//
void DumpToStream::ExitHelper::operator () (AST::Initialization&)
{
	UnindentAndPrint(self, L"End of initialization");
}


//
// Begin traversing a node representing an entity invocation
//
void DumpToStream::EntryHelper::operator () (AST::Entity& entity)
{
	PrintAndIndent(self, L"Entity ", entity.Identifier);
}

//
// Finish traversing a node representing an entity invocation
//
void DumpToStream::ExitHelper::operator () (AST::Entity&)
{
	UnindentAndPrint(self, L"End of entity");
}

//
// Begin traversing a node containing a chained entity invocation
//
void DumpToStream::EntryHelper::operator () (AST::ChainedEntity& entity)
{
	PrintAndIndent(self, L"Chained Entity ", entity.Identifier);
}

//
// Finish traversing a node containing a chained entity invocation
//
void DumpToStream::ExitHelper::operator () (AST::ChainedEntity&)
{
	UnindentAndPrint(self, L"End of chained entity");
}

//
// Begin traversing a postfix entity invocation node
//
void DumpToStream::EntryHelper::operator () (AST::PostfixEntity& entity)
{
	PrintAndIndent(self, L"Postfix Entity ", entity.Identifier);
}

//
// Finish traversing a postfix entity invocation node
//
void DumpToStream::ExitHelper::operator () (AST::PostfixEntity&)
{
	UnindentAndPrint(self, L"End of postfix entity");
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
	Print(self, identifier);
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
	PrintAndIndent(self, L"Function signature named ", refsig.Identifier, L" with parameter types:");
	Print(self, refsig.ParamTypes);
	UnindentAndPrint(self, L"Returning");
	// TODO - fix AST dump of function reference signature return type
}


//
// Traverse a special marker that indicates the subsequent nodes belong
// to the return expression definition of a function
//
void DumpToStream::EntryHelper::operator () (Markers::FunctionReturnExpression&)
{
	PrintAndIndent(self, L"Function return");
}

//
// Finish traversing a function's return expression
//
void DumpToStream::ExitHelper::operator () (Markers::FunctionReturnExpression&)
{
	UnindentAndPrint(self, L"End of return");
}


//
// Traverse a special marker that indicates we are entering a list of
// prefix operators applied to an expression component.
//
// There is nothing particularly special to do here.
//
void DumpToStream::EntryHelper::operator () (Markers::ExpressionComponentPrefixes&)
{
	// Nothing to do
}

//
// Finish traversing unary prefixes
//
void DumpToStream::ExitHelper::operator () (Markers::ExpressionComponentPrefixes&)
{
	// Nothing to do
}



void DumpToStream::EntryHelper::operator () (Markers::FunctionSignatureParams&)
{
	// Nothing to do
}

void DumpToStream::ExitHelper::operator () (Markers::FunctionSignatureParams&)
{
	// Nothing to do
}

void DumpToStream::EntryHelper::operator () (Markers::FunctionSignatureReturn&)
{
	// Nothing to do
}

void DumpToStream::ExitHelper::operator () (Markers::FunctionSignatureReturn&)
{
	// Nothing to do
}


//
// Helper function: indent the output to the current level
//
void DumpToStream::Indent()
{
	std::fill_n(std::ostream_iterator<wchar_t, wchar_t>(TheStream), Indentation, L' ');
}


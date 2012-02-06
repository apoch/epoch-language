//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST traverser declaration for the compilation pass which
// performs semantic validation and type inference on the
// input program code. This traverser converts the AST into
// an intermediate representation (IR) which is then used
// by the compiler to perform semantic checks etc.
//

#include "pch.h"

#include "Compiler/Passes/SemanticValidation.h"

#include "Compiler/Abstract Syntax Tree/Literals.h"
#include "Compiler/Abstract Syntax Tree/Parenthetical.h"
#include "Compiler/Abstract Syntax Tree/Expression.h"
#include "Compiler/Abstract Syntax Tree/Statement.h"
#include "Compiler/Abstract Syntax Tree/Entities.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Structure.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Function.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Assignment.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Statement.h"
#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Entity.h"

#include "Compiler/Session.h"

#include "Utility/Strings.h"

#include <algorithm>


//
// Internal helpers
//
namespace
{

	struct StringPooler
	{
		StringPooler(IRSemantics::Program& program, std::vector<StringHandle>& vec)
			: Program(program),
			  Container(vec)
		{ }

		void operator () (const AST::IdentifierT& identifier)
		{
			StringHandle handle = Program.AddString(std::wstring(identifier.begin(), identifier.end()));
			Container.push_back(handle);
		}

	private:
		IRSemantics::Program& Program;
		std::vector<StringHandle>& Container;
	};


	//
	// Compile-time helper: when a variable definition is encountered, this
	// helper adds the variable itself and its type metadata to the current
	// lexical scope.
	//
	void CompileConstructorStructure(IRSemantics::Statement& statement, IRSemantics::Program& program, IRSemantics::CodeBlock& activescope, bool inreturnexpr)
	{
		const IRSemantics::ExpressionAtomIdentifier* atom = dynamic_cast<const IRSemantics::ExpressionAtomIdentifier*>(statement.GetParameters()[0]->GetAtoms()[0]);

		VariableOrigin origin = (inreturnexpr ? VARIABLE_ORIGIN_RETURN : VARIABLE_ORIGIN_LOCAL);
		VM::EpochTypeID effectivetype = program.LookupType(statement.GetName());
		activescope.AddVariable(program.GetString(atom->GetIdentifier()), atom->GetIdentifier(), effectivetype, false, origin);
	}

}


//
// Validate semantics for a program
//
IRSemantics::Program* CompilerPasses::ValidateSemantics(AST::Program& program, StringPoolManager& strings, CompileSession& session)
{
	// Construct the semantic analysis pass
	ASTTraverse::CompilePassSemantics pass(strings, session);
	
	// Traverse the AST and convert it into the semantic IR
	ASTTraverse::DoTraversal(pass, program);

	// Perform compile-time code execution, e.g. constructors for populating lexical scopes
	if(!pass.CompileTimeCodeExecution())
		return NULL;

	// Perform type inference and decorate the IR with type information
	if(!pass.TypeInference())
		return NULL;

	// Perform type validation
	if(!pass.Validate())
		return NULL;

	// Success!
	return pass.DetachProgram();
}


using namespace ASTTraverse;


//
// Destruct and clean up a program semantic verification pass
//
CompilePassSemantics::~CompilePassSemantics()
{
	for(std::vector<IRSemantics::Structure*>::iterator iter = CurrentStructures.begin(); iter != CurrentStructures.end(); ++iter)
		delete *iter;

	for(std::vector<IRSemantics::Function*>::iterator iter = CurrentFunctions.begin(); iter != CurrentFunctions.end(); ++iter)
		delete *iter;

	for(std::vector<IRSemantics::Expression*>::iterator iter = CurrentExpressions.begin(); iter != CurrentExpressions.end(); ++iter)
		delete *iter;

	for(std::vector<IRSemantics::Assignment*>::iterator iter = CurrentAssignments.begin(); iter != CurrentAssignments.end(); ++iter)
		delete *iter;

	for(std::vector<IRSemantics::Statement*>::iterator iter = CurrentStatements.begin(); iter != CurrentStatements.end(); ++iter)
		delete *iter;

	for(std::vector<IRSemantics::CodeBlock*>::iterator iter = CurrentCodeBlocks.begin(); iter != CurrentCodeBlocks.end(); ++iter)
		delete *iter;

	for(std::vector<IRSemantics::Entity*>::iterator iter = CurrentEntities.begin(); iter != CurrentEntities.end(); ++iter)
		delete *iter;

	for(std::vector<IRSemantics::Entity*>::iterator iter = CurrentChainedEntities.begin(); iter != CurrentChainedEntities.end(); ++iter)
		delete *iter;

	for(std::vector<IRSemantics::Entity*>::iterator iter = CurrentPostfixEntities.begin(); iter != CurrentPostfixEntities.end(); ++iter)
		delete *iter;

	delete CurrentProgram;
}

//
// Perform compile-time code execution on a program
//
bool CompilePassSemantics::CompileTimeCodeExecution()
{
	if(CurrentProgram)
		return CurrentProgram->CompileTimeCodeExecution();

	return false;
}

//
// Perform type inference/type decoration on a program
//
bool CompilePassSemantics::TypeInference()
{
	if(CurrentProgram)
		return CurrentProgram->TypeInference();

	return false;
}

//
// Validate a program
//
bool CompilePassSemantics::Validate() const
{
	if(CurrentProgram)
		return CurrentProgram->Validate();

	return false;
}

//
// Detach and return the converted program IR
//
IRSemantics::Program* CompilePassSemantics::DetachProgram()
{
	IRSemantics::Program* p = NULL;
	std::swap(p, CurrentProgram);
	return p;
}


//
// Begin traversing a node with no defined type
//
void CompilePassSemantics::EntryHelper::operator () (AST::Undefined&)
{
	if (self->StateStack.empty() || !self->InFunctionReturn)
	{
		//
		// This is a failure of the parser to properly
		// capture incorrect programs prior to being
		// submitted for semantic validation.
		//
		// Undefined nodes are permitted in two situations:
		//  1. Empty programs
		//  2. Void function return expressions
		//
		// Any other presence of an undefined node represents
		// a mistake in the parser. Ensure that parser errors
		// are detected correctly and that programs which are
		// not fully parsed can not be submitted for semantic
		// validation passes.
		//
		throw InternalException("Undefined AST node in unexpected context");
	}
}


//
// Begin traversing a program node, which represents the root of the AST
//
void CompilePassSemantics::EntryHelper::operator () (AST::Program& program)
{
	if(self->CurrentProgram)
	{
		//
		// The parser has generated a program which contains
		// either a cyclic reference to itself or to another
		// AST. This is undesirable behavior; the parser can
		// produce multiple AST fragments for the purpose of
		// separate compilation but they must be merged into
		// a single program-wide AST prior to the performing
		// semantic analysis, or submitted as fully separate
		// tree structures within a compilation session.
		//
		throw InternalException("Re-entrant AST detected");
	}

	self->StateStack.push(CompilePassSemantics::STATE_PROGRAM);
	self->CurrentProgram = new IRSemantics::Program(self->Strings, self->Session);
}

//
// Finish traversing a program node
//
void CompilePassSemantics::ExitHelper::operator () (AST::Program& program)
{
	self->StateStack.pop();
}


//
// Begin traversing a structure definition node
//
void CompilePassSemantics::EntryHelper::operator () (AST::Structure& structure)
{
	self->CurrentStructures.push_back(new IRSemantics::Structure);
}

//
// Finish traversing a structure definition node
//
void CompilePassSemantics::ExitHelper::operator () (AST::Structure& structure)
{
	if(self->CurrentStructures.size() == 1)
	{
		std::auto_ptr<IRSemantics::Structure> irstruct(self->CurrentStructures.back());
		self->CurrentStructures.pop_back();
		
		StringHandle name = self->CurrentProgram->AddString(std::wstring(structure.Identifier.begin(), structure.Identifier.end()));
		self->CurrentProgram->Session.InfoTable.FunctionHelpers->insert(std::make_pair(name, &CompileConstructorStructure));
		
		FunctionSignature signature;
		signature.AddParameter(L"id", VM::EpochType_Identifier, false);
		const std::vector<std::pair<StringHandle, IRSemantics::StructureMember*> >& members = irstruct->GetMembers();
		for(std::vector<std::pair<StringHandle, IRSemantics::StructureMember*> >::const_iterator iter = members.begin(); iter != members.end(); ++iter)
			signature.AddParameter(self->CurrentProgram->GetString(iter->first), iter->second->GetEpochType(*self->CurrentProgram), false);

		self->CurrentProgram->Session.FunctionSignatures.insert(std::make_pair(name, signature));

		self->CurrentProgram->AddStructure(name, irstruct.release());
	}
	else
	{
		//
		// This is plain and simple a missing language feature.
		//
		// Nested structure definitions have been parsed and sent
		// to the AST for semantic validation, but the above code
		// is not configured to handle this. Implement the code.
		//
		throw InternalException("Support for nested structure definitions is not implemented.");
	}
}


//
// Traverse a node defining a member variable in a structure
//
void CompilePassSemantics::EntryHelper::operator () (AST::StructureMemberVariable& variable)
{
	if(self->CurrentStructures.empty())
	{
		//
		// This is a failure of the AST traversal.
		//
		// A structure member variable definition node has
		// been submitted for semantic validation, but not
		// in the context of a structure definition.
		//
		// Examine the AST generation and traversal logic.
		//
		throw InternalException("Attempted to traverse a structure member AST node in an invalid context");
	}

	StringHandle name = self->CurrentProgram->AddString(std::wstring(variable.Name.begin(), variable.Name.end()));
	StringHandle type = self->CurrentProgram->AddString(std::wstring(variable.Type.begin(), variable.Type.end()));

	std::auto_ptr<IRSemantics::StructureMemberVariable> member(new IRSemantics::StructureMemberVariable(type));
	self->CurrentStructures.back()->AddMember(name, member.release());
}

//
// Traverse a node defining a member function reference in a structure
//
void CompilePassSemantics::EntryHelper::operator () (AST::StructureMemberFunctionRef& funcref)
{
	if(self->CurrentStructures.empty())
	{
		//
		// This is a failure of the AST traversal.
		//
		// A structure member variable definition node has
		// been submitted for semantic validation, but not
		// in the context of a structure definition.
		//
		// Specifically, the member variable is a function
		// reference with the associated signature.
		//
		// Examine the AST generation and traversal logic.
		//
		throw InternalException("Attempted to traverse a structure member AST node in an invalid context");
	}

	StringHandle name = self->CurrentProgram->AddString(std::wstring(funcref.Name.begin(), funcref.Name.end()));
	StringHandle returntype = self->CurrentProgram->AddString(std::wstring(funcref.ReturnType.begin(), funcref.ReturnType.end()));

	std::vector<StringHandle> paramtypes;
	std::for_each(funcref.ParamTypes.begin(), funcref.ParamTypes.end(), StringPooler(*self->CurrentProgram, paramtypes));

	std::auto_ptr<IRSemantics::StructureMemberFunctionReference> member(new IRSemantics::StructureMemberFunctionReference(paramtypes, returntype));
	self->CurrentStructures.back()->AddMember(name, member.release());
}


//
// Begin traversing a node that defines a function
//
void CompilePassSemantics::EntryHelper::operator () (AST::Function& function)
{
	self->StateStack.push(CompilePassSemantics::STATE_FUNCTION);
	self->CurrentFunctions.push_back(new IRSemantics::Function);
}

//
// Finish traversing a node that defines a function
//
void CompilePassSemantics::ExitHelper::operator () (AST::Function& function)
{
	self->StateStack.pop();

	if(!self->CurrentFunctions.back()->GetCode())
	{
		std::auto_ptr<ScopeDescription> lexicalscope(new ScopeDescription(self->CurrentProgram->GetGlobalScope()));
		self->CurrentFunctions.back()->SetCode(new IRSemantics::CodeBlock(lexicalscope.release()));
	}

	const std::vector<StringHandle>& params = self->CurrentFunctions.back()->GetParameterNames();
	for(std::vector<StringHandle>::const_iterator iter = params.begin(); iter != params.end(); ++iter)
	{
		if(self->CurrentFunctions.back()->IsParameterLocalVariable(*iter))
		{
			VM::EpochTypeID type = self->CurrentFunctions.back()->GetParameterType(*iter, *self->CurrentProgram);
			bool isref = self->CurrentFunctions.back()->IsParameterReference(*iter);
			self->CurrentFunctions.back()->GetCode()->AddVariable(self->CurrentProgram->GetString(*iter), *iter, type, isref, VARIABLE_ORIGIN_PARAMETER);
		}
	}

	if(self->CurrentFunctions.size() == 1)
	{
		StringHandle name = self->CurrentProgram->CreateFunctionOverload(std::wstring(function.Name.begin(), function.Name.end()));
		self->CurrentProgram->AddFunction(name, self->CurrentFunctions.back());
		self->CurrentFunctions.pop_back();
	}
	else
	{
		//
		// This is just a missing language feature.
		//
		// Nested/inner functions should be supported eventually,
		// and in this case the parser has generated an AST which
		// contains such a function, but the above code isn't set
		// up to handle it.
		//
		throw InternalException("A nested (inner) function was produced by the parser, but support is not implemented");
	}
}


//
// Begin traversing a node that defines a function parameter
//
void CompilePassSemantics::EntryHelper::operator () (AST::FunctionParameter& param)
{
	self->StateStack.push(STATE_FUNCTION_PARAM);
}

//
// Finish traversing a node that defines a function parameter
//
void CompilePassSemantics::ExitHelper::operator () (AST::FunctionParameter& param)
{
	self->StateStack.pop();
}


//
// Traverse a node that corresponds to a named function parameter
//
// We can safely treat these as leaf nodes, hence the direct access of the
// node's properties to retrieve its contents.
//
void CompilePassSemantics::EntryHelper::operator () (AST::NamedFunctionParameter& param)
{
	if(self->CurrentFunctions.empty())
	{
		//
		// This is a failure of the AST traversal.
		//
		// A function parameter definition node has been
		// traversed outside the context of a function
		// definition.
		//
		// Examine the AST generation and traversal logic.
		//
		throw InternalException("Attempted to traverse a function parameter definition AST node in an invalid context");
	}


	StringHandle name = self->CurrentProgram->AddString(std::wstring(param.Name.begin(), param.Name.end()));
	StringHandle type = self->CurrentProgram->AddString(std::wstring(param.Type.begin(), param.Type.end()));

	// TODO - support for ref params
	std::auto_ptr<IRSemantics::FunctionParamNamed> irparam(new IRSemantics::FunctionParamNamed(type, false));
	self->CurrentFunctions.back()->AddParameter(name, irparam.release());
}


//
// Begin traversing a node that corresponds to an expression
//
void CompilePassSemantics::EntryHelper::operator () (AST::Expression& expression)
{
	self->StateStack.push(CompilePassSemantics::STATE_EXPRESSION);
	self->CurrentExpressions.push_back(new IRSemantics::Expression);
}

//
// Finish traversing an expression node
//
void CompilePassSemantics::ExitHelper::operator () (AST::Expression& expression)
{
	self->StateStack.pop();

	switch(self->StateStack.top())
	{
	default:
	case CompilePassSemantics::STATE_UNKNOWN:
		throw std::exception("Invalid parse state");		// TODO - better exceptions

	case CompilePassSemantics::STATE_STATEMENT:
		if(self->CurrentStatements.empty())
			throw std::exception("Invalid parse state");	// TODO - better exceptions

		self->CurrentStatements.back()->AddParameter(self->CurrentExpressions.back());
		self->CurrentExpressions.pop_back();
		break;

	case CompilePassSemantics::STATE_ASSIGNMENT:
		if(self->CurrentAssignments.empty())
			throw std::exception("Invalid parse state");	// TODO - better exceptions

		self->CurrentAssignments.back()->SetRHS(new IRSemantics::AssignmentChainExpression(self->CurrentExpressions.back()));
		self->CurrentExpressions.pop_back();
		break;

	case CompilePassSemantics::STATE_ENTITY:
		self->CurrentEntities.back()->AddParameter(self->CurrentExpressions.back());
		self->CurrentExpressions.pop_back();
		break;

	case CompilePassSemantics::STATE_CHAINED_ENTITY:
		self->CurrentChainedEntities.back()->AddParameter(self->CurrentExpressions.back());
		self->CurrentExpressions.pop_back();
		break;

	case CompilePassSemantics::STATE_FUNCTION_RETURN:
		// Nothing needs to be done; leave the expression on the state stack
		// When the FunctionReturnExpression marker is exited it will handle the expression correctly
		break;

	case CompilePassSemantics::STATE_FUNCTION_PARAM:
		if(self->CurrentFunctions.empty())
			throw std::exception("Invalid parse state");	// TODO - better exceptions

		{
			StringHandle paramname = self->CurrentProgram->AllocateAnonymousParamName();

			std::auto_ptr<IRSemantics::FunctionParamExpression> irparam(new IRSemantics::FunctionParamExpression(self->CurrentExpressions.back()));
			self->CurrentExpressions.pop_back();

			self->CurrentFunctions.back()->AddParameter(paramname, irparam.release());
		}
		break;
	}
}

//
// Begin traversing an expression component node (see AST definitions for details)
//
void CompilePassSemantics::EntryHelper::operator () (AST::ExpressionComponent& exprcomponent)
{
	self->StateStack.push(CompilePassSemantics::STATE_EXPRESSION_COMPONENT);
}

//
// Finish traversing an expression component node
//
void CompilePassSemantics::ExitHelper::operator () (AST::ExpressionComponent& exprcomponent)
{
	self->StateStack.pop();
}

//
// Begin traversing an expression fragment node (see AST definitions for details)
//
void CompilePassSemantics::EntryHelper::operator () (AST::ExpressionFragment& exprfragment)
{
	self->StateStack.push(CompilePassSemantics::STATE_EXPRESSION_FRAGMENT);

	std::wstring token(exprfragment.Operator.begin(), exprfragment.Operator.end());
	StringHandle opname = self->CurrentProgram->AddString(token);
	self->CurrentExpressions.back()->AddAtom(new IRSemantics::ExpressionAtomOperator(opname, token == L"."));
}

//
// Finish traversing an expression fragment node
//
void CompilePassSemantics::ExitHelper::operator () (AST::ExpressionFragment& exprfragment)
{
	self->StateStack.pop();
}


//
// Begin traversing a statement node
//
void CompilePassSemantics::EntryHelper::operator () (AST::Statement& statement)
{
	StringHandle namehandle = self->CurrentProgram->AddString(std::wstring(statement.Identifier.begin(), statement.Identifier.end()));

	self->StateStack.push(CompilePassSemantics::STATE_STATEMENT);
	self->CurrentStatements.push_back(new IRSemantics::Statement(namehandle));
}

//
// Finish traversing a statement node
//
void CompilePassSemantics::ExitHelper::operator () (AST::Statement& statement)
{
	self->StateStack.pop();

	switch(self->StateStack.top())
	{
	case CompilePassSemantics::STATE_EXPRESSION_COMPONENT:
		self->CurrentExpressions.back()->AddAtom(new IRSemantics::ExpressionAtomStatement(self->CurrentStatements.back()));
		self->CurrentStatements.pop_back();
		break;

	case CompilePassSemantics::STATE_CODE_BLOCK:
		self->CurrentCodeBlocks.back()->AddEntry(new IRSemantics::CodeBlockStatementEntry(self->CurrentStatements.back()));
		self->CurrentStatements.pop_back();
		break;

	default:
		throw std::exception("Invalid parse state");			// TODO - better exceptions
	}
}


//
// Begin traversing a node corresponding to a pre-operation statement
//
void CompilePassSemantics::EntryHelper::operator () (AST::PreOperatorStatement& statement)
{
	self->StateStack.push(CompilePassSemantics::STATE_PREOP_STATEMENT);
}

//
// Finish traversing a node corresponding to a pre-operation statement
//
void CompilePassSemantics::ExitHelper::operator () (AST::PreOperatorStatement& statement)
{
	self->StateStack.pop();

	StringHandle operatorname = self->CurrentProgram->AddString(std::wstring(statement.Operator.begin(), statement.Operator.end()));

	std::vector<StringHandle> operand;
	std::for_each(statement.Operand.begin(), statement.Operand.end(), StringPooler(*self->CurrentProgram, operand));

	std::auto_ptr<IRSemantics::PreOpStatement> irstatement(new IRSemantics::PreOpStatement(operatorname, operand));

	switch(self->StateStack.top())
	{
	case CompilePassSemantics::STATE_EXPRESSION_COMPONENT:
		{
			std::auto_ptr<IRSemantics::ParentheticalPreOp> irparenthetical(new IRSemantics::ParentheticalPreOp(irstatement.release()));
			self->CurrentExpressions.back()->AddAtom(new IRSemantics::ExpressionAtomParenthetical(irparenthetical.release()));
		}
		break;

	case CompilePassSemantics::STATE_CODE_BLOCK:
		self->CurrentCodeBlocks.back()->AddEntry(new IRSemantics::CodeBlockPreOpStatementEntry(irstatement.release()));
		break;

	default:
		throw std::exception("Invalid parse state");			// TODO - better exceptions
	}
}


//
// Begin traversing a node corresponding to a post-operation statement
//
void CompilePassSemantics::EntryHelper::operator () (AST::PostOperatorStatement& statement)
{
	self->StateStack.push(CompilePassSemantics::STATE_POSTOP_STATEMENT);
}

//
// Finish traversing a node corresponding to a post-operation statement
//
void CompilePassSemantics::ExitHelper::operator () (AST::PostOperatorStatement& statement)
{
	self->StateStack.pop();

	StringHandle operatorname = self->CurrentProgram->AddString(std::wstring(statement.Operator.begin(), statement.Operator.end()));

	std::vector<StringHandle> operand;
	std::for_each(statement.Operand.begin(), statement.Operand.end(), StringPooler(*self->CurrentProgram, operand));

	std::auto_ptr<IRSemantics::PostOpStatement> irstatement(new IRSemantics::PostOpStatement(operand, operatorname));

	switch(self->StateStack.top())
	{
	case CompilePassSemantics::STATE_EXPRESSION_COMPONENT:
		{
			std::auto_ptr<IRSemantics::ParentheticalPostOp> irparenthetical(new IRSemantics::ParentheticalPostOp(irstatement.release()));
			self->CurrentExpressions.back()->AddAtom(new IRSemantics::ExpressionAtomParenthetical(irparenthetical.release()));
		}
		break;

	case CompilePassSemantics::STATE_CODE_BLOCK:
		self->CurrentCodeBlocks.back()->AddEntry(new IRSemantics::CodeBlockPostOpStatementEntry(irstatement.release()));
		break;

	default:
		throw std::exception("Invalid parse state");			// TODO - better exceptions
	}
}


//
// Begin traversing a node containing a code block
//
void CompilePassSemantics::EntryHelper::operator () (AST::CodeBlock& block)
{
	bool owned = true;
	ScopeDescription* lexicalscope = NULL;

	switch(self->StateStack.top())
	{
	case CompilePassSemantics::STATE_PROGRAM:
		lexicalscope = self->CurrentProgram->GetGlobalScope();
		owned = false;
		break;

	case CompilePassSemantics::STATE_FUNCTION:
		lexicalscope = new ScopeDescription(self->CurrentProgram->GetGlobalScope());
		break;

	default:
		lexicalscope = new ScopeDescription(self->CurrentCodeBlocks.back()->GetScope());
		break;
	}

	self->CurrentCodeBlocks.push_back(new IRSemantics::CodeBlock(lexicalscope, owned));
	self->StateStack.push(CompilePassSemantics::STATE_CODE_BLOCK);
}

//
// Finish traversing a code block node
//
void CompilePassSemantics::ExitHelper::operator () (AST::CodeBlock& block)
{
	self->CurrentProgram->AllocateLexicalScopeName(self->CurrentCodeBlocks.back());

	self->StateStack.pop();
	
	switch(self->StateStack.top())
	{
	case CompilePassSemantics::STATE_CODE_BLOCK:
		self->CurrentCodeBlocks.back()->AddEntry(new IRSemantics::CodeBlockInnerBlockEntry(self->CurrentCodeBlocks.back()));
		break;

	case CompilePassSemantics::STATE_FUNCTION:
		self->CurrentFunctions.back()->SetCode(self->CurrentCodeBlocks.back());
		break;

	case CompilePassSemantics::STATE_PROGRAM:
		self->CurrentProgram->AddGlobalCodeBlock(self->CurrentCodeBlocks.back());
		break;

	case CompilePassSemantics::STATE_ENTITY:
		self->CurrentEntities.back()->SetCode(self->CurrentCodeBlocks.back());
		break;

	case CompilePassSemantics::STATE_CHAINED_ENTITY:
		self->CurrentChainedEntities.back()->SetCode(self->CurrentCodeBlocks.back());
		break;

	default:
		throw std::exception("Invalid parse state");				// TODO - better exceptions
	}

	self->CurrentCodeBlocks.pop_back();
}


//
// Begin traversing a node corresponding to an assignment
//
void CompilePassSemantics::EntryHelper::operator () (AST::Assignment& assignment)
{
	CompilePassSemantics::States state = self->StateStack.top();

	self->StateStack.push(CompilePassSemantics::STATE_ASSIGNMENT);

	StringHandle opname = self->CurrentProgram->AddString(std::wstring(assignment.Operator.begin(), assignment.Operator.end()));

	std::vector<StringHandle> lhs;
	std::for_each(assignment.LHS.begin(), assignment.LHS.end(), StringPooler(*self->CurrentProgram, lhs));

	switch(state)
	{
	case CompilePassSemantics::STATE_CODE_BLOCK:
		self->CurrentAssignments.push_back(new IRSemantics::Assignment(lhs, opname));
		break;

	case CompilePassSemantics::STATE_ASSIGNMENT:
		{
			std::auto_ptr<IRSemantics::Assignment> irassignment(new IRSemantics::Assignment(lhs, opname));
			std::auto_ptr<IRSemantics::AssignmentChainAssignment> irassignmentchain(new IRSemantics::AssignmentChainAssignment(irassignment.release()));
			self->CurrentAssignments.back()->SetRHSRecursive(irassignmentchain.release());
		}
		break;

	default:
		throw std::exception("Invalid parse state");			// TODO - better exceptions
	}
}

//
// Finish traversing a node corresponding to an assignment
//
void CompilePassSemantics::ExitHelper::operator () (AST::Assignment& assignment)
{
	self->StateStack.pop();
	
	if(self->StateStack.top() != CompilePassSemantics::STATE_CODE_BLOCK)
		throw std::exception("Invalid parse state");			// TODO - better exceptions

	self->CurrentCodeBlocks.back()->AddEntry(new IRSemantics::CodeBlockAssignmentEntry(self->CurrentAssignments.back()));
	self->CurrentAssignments.pop_back();
}


//
// Begin traversing a node representing an entity invocation
//
void CompilePassSemantics::EntryHelper::operator () (AST::Entity& entity)
{
	StringHandle entityname = self->CurrentProgram->AddString(std::wstring(entity.Identifier.begin(), entity.Identifier.end()));

	self->StateStack.push(CompilePassSemantics::STATE_ENTITY);
	self->CurrentEntities.push_back(new IRSemantics::Entity(entityname));
}

//
// Finish traversing a node representing an entity invocation
//
void CompilePassSemantics::ExitHelper::operator () (AST::Entity& entity)
{
	self->StateStack.pop();
	
	switch(self->StateStack.top())
	{
	case CompilePassSemantics::STATE_CODE_BLOCK:
		self->CurrentCodeBlocks.back()->AddEntry(new IRSemantics::CodeBlockEntityEntry(self->CurrentEntities.back()));
		self->CurrentEntities.pop_back();
		break;

	default:
		throw std::exception("Invalid parse state");			// TODO - better exceptions
	}
}

//
// Begin traversing a node containing a chained entity invocation
//
void CompilePassSemantics::EntryHelper::operator () (AST::ChainedEntity& entity)
{
	StringHandle entityname = self->CurrentProgram->AddString(std::wstring(entity.Identifier.begin(), entity.Identifier.end()));

	self->StateStack.push(CompilePassSemantics::STATE_CHAINED_ENTITY);
	self->CurrentChainedEntities.push_back(new IRSemantics::Entity(entityname));
}

//
// Finish traversing a node containing a chained entity invocation
//
void CompilePassSemantics::ExitHelper::operator () (AST::ChainedEntity& entity)
{
	self->StateStack.pop();
	
	if(self->StateStack.top() != CompilePassSemantics::STATE_ENTITY)
		throw std::exception("Invalid parse state");

	self->CurrentEntities.back()->AddChain(self->CurrentChainedEntities.back());
	self->CurrentChainedEntities.pop_back();
}

//
// Begin traversing a postfix entity invocation node
//
void CompilePassSemantics::EntryHelper::operator () (AST::PostfixEntity& entity)
{
	StringHandle entityname = self->CurrentProgram->AddString(std::wstring(entity.Identifier.begin(), entity.Identifier.end()));

	self->StateStack.push(CompilePassSemantics::STATE_POSTFIX_ENTITY);
	self->CurrentPostfixEntities.push_back(new IRSemantics::Entity(entityname));
}

//
// Finish traversing a postfix entity invocation node
//
void CompilePassSemantics::ExitHelper::operator () (AST::PostfixEntity& entity)
{
	self->StateStack.pop();
	
	switch(self->StateStack.top())
	{
	case CompilePassSemantics::STATE_CODE_BLOCK:
		self->CurrentCodeBlocks.back()->AddEntry(new IRSemantics::CodeBlockEntityEntry(self->CurrentPostfixEntities.back()));
		self->CurrentPostfixEntities.pop_back();
		break;

	default:
		throw std::exception("Invalid parse state");			// TODO - better exceptions
	}
}


//
// Traverse a node containing an identifier
//
// Note that this is not necessarily always a leaf node; identifiers can
// be attached to entity invocations, function calls, etc. and therefore
// this needs to be able to handle all situations in which an identifier
// might appear.
//
void CompilePassSemantics::EntryHelper::operator () (AST::IdentifierT& identifier)
{
	Substring raw(identifier.begin(), identifier.end());

	std::auto_ptr<IRSemantics::ExpressionAtom> iratom(NULL);

	if(raw.length() >= 2 && *raw.begin() == L'\"' && *raw.rbegin() == L'\"')
	{
		StringHandle handle = self->CurrentProgram->AddString(StripQuotes(raw));
		iratom.reset(new IRSemantics::ExpressionAtomLiteralString(handle));
	}
	else if(raw == L"true")
		iratom.reset(new IRSemantics::ExpressionAtomLiteralBoolean(true));
	else if(raw == L"false")
		iratom.reset(new IRSemantics::ExpressionAtomLiteralBoolean(false));
	else if(raw.find(L'.') != std::wstring::npos)
	{
		Real32 literalfloat;

		std::wistringstream convert(raw);
		if(!(convert >> literalfloat))
			throw std::exception("Invalid floating point literal");
		
		iratom.reset(new IRSemantics::ExpressionAtomLiteralReal32(literalfloat));
	}
	else
	{
		UInteger32 literalint;

		std::wistringstream convert(raw);
		if(convert >> literalint)
			iratom.reset(new IRSemantics::ExpressionAtomLiteralInteger32(static_cast<Integer32>(literalint)));
		else
		{
			StringHandle handle = self->CurrentProgram->AddString(raw);
			iratom.reset(new IRSemantics::ExpressionAtomIdentifier(handle));
		}
	}

	if(!iratom.get())
		throw std::exception("Unrecognized literal/token");			// TODO - better exceptions

	switch(self->StateStack.top())
	{
	case CompilePassSemantics::STATE_ASSIGNMENT:
		// TODO
		/*
		{
			std::auto_ptr<IRSemantics::ExpressionComponent> ircomponent(new IRSemantics::ExpressionComponent(std::vector<StringHandle>()));
			ircomponent->SetAtom(iratom.release());
			std::auto_ptr<IRSemantics::Expression> irexpression(new IRSemantics::Expression(ircomponent.release()));
			self->CurrentAssignments.back()->SetRHS(new IRSemantics::AssignmentChainExpression(irexpression.release()));
		}
		*/
		break;

	case CompilePassSemantics::STATE_EXPRESSION_COMPONENT_PREFIXES:
		self->CurrentExpressions.back()->AddAtom(new IRSemantics::ExpressionAtomOperator(dynamic_cast<IRSemantics::ExpressionAtomIdentifier*>(iratom.get())->GetIdentifier(), false));
		break;

	case CompilePassSemantics::STATE_EXPRESSION_COMPONENT:
	case CompilePassSemantics::STATE_EXPRESSION_FRAGMENT:
		self->CurrentExpressions.back()->AddAtom(iratom.release());
		break;

	case CompilePassSemantics::STATE_FUNCTION:
		self->CurrentFunctions.back()->SetName(self->CurrentProgram->AddString(raw));
		break;

	default:
		throw std::exception("Invalid parse state");			// TODO - better exceptions
	}
}


//
// Traverse a node representing a function reference signature; this is
// typically used when passing function references to higher-order functions
// in the program being compiled.
//
// We can safely treat this as an atomic leaf node, hence the direct access
// of the node's properties when generating output.
//
void CompilePassSemantics::EntryHelper::operator () (AST::FunctionReferenceSignature& refsig)
{
	if(self->CurrentFunctions.empty())
		throw std::exception("Invalid parse state");		// TODO - better exceptions

	StringHandle name = self->CurrentProgram->AddString(std::wstring(refsig.Identifier.begin(), refsig.Identifier.end()));
	StringHandle returntype = self->CurrentProgram->AddString(std::wstring(refsig.ReturnType.begin(), refsig.ReturnType.end()));

	std::vector<StringHandle> paramtypes;
	std::for_each(refsig.ParamTypes.begin(), refsig.ParamTypes.end(), StringPooler(*self->CurrentProgram, paramtypes));

	std::auto_ptr<IRSemantics::FunctionParamFuncRef> irparam(new IRSemantics::FunctionParamFuncRef(paramtypes, returntype));
	self->CurrentFunctions.back()->AddParameter(name, irparam.release());
}


//
// Traverse a special marker that indicates the subsequent nodes belong
// to the return expression definition of a function
//
void CompilePassSemantics::EntryHelper::operator () (Markers::FunctionReturnExpression&)
{
	self->StateStack.push(CompilePassSemantics::STATE_FUNCTION_RETURN);
	self->InFunctionReturn = true;
}

//
// Finish traversing a function's return expression
//
void CompilePassSemantics::ExitHelper::operator () (Markers::FunctionReturnExpression&)
{
	if(self->CurrentFunctions.empty())
		throw std::exception("Invalid parse state");			// TODO - better exceptions

	self->CurrentFunctions.back()->SetReturnExpression(self->CurrentExpressions.back());
	self->CurrentExpressions.pop_back();

	self->StateStack.pop();
	self->InFunctionReturn = false;
}


void CompilePassSemantics::EntryHelper::operator () (Markers::ExpressionComponentPrefixes&)
{
	self->StateStack.push(CompilePassSemantics::STATE_EXPRESSION_COMPONENT_PREFIXES);
}

void CompilePassSemantics::ExitHelper::operator () (Markers::ExpressionComponentPrefixes&)
{
	self->StateStack.pop();
}

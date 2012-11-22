//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class containing a code block
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Assignment.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Statement.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Entity.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"

#include "Compiler/Intermediate Representations/Semantic Validation/InferenceContext.h"

#include "Compiler/CompileErrors.h"
#include "Compiler/Exceptions.h"


using namespace IRSemantics;


//
// Construct and initialize a code block IR node
//
CodeBlock::CodeBlock(ScopeDescription* scope, bool ownsscope)
	: Scope(scope),
	  OwnsScope(ownsscope)
{
	if(!Scope)
	{
		//
		// This exception catches potential mistakes in the calling code.
		//
		// Code blocks are conceptually bound to scopes and cannot be created
		// or managed without an associated scope descriptor, even if the
		// scope itself is empty of any contents.
		//
		// Ownership semantics can vary - in some cases the code block owns
		// the scope descriptor and must free it upon destruction. However,
		// it is also possible for the scope to be shared, e.g. for global
		// scoped variable declaration blocks, or separately compiled namespace
		// implementations. We do not use a shared_ptr here in order to make
		// sure that the semantics are clearly expressed and followed.
		//
		throw InternalException("Contract failure: all code blocks must be bound to a non-null scope descriptor");
	}
}

//
// Destruct and clean up a code block IR node
//
CodeBlock::~CodeBlock()
{
	for(std::vector<CodeBlockEntry*>::iterator iter = Entries.begin(); iter != Entries.end(); ++iter)
		delete *iter;

	if(OwnsScope)
		delete Scope;
}

//
// Deep copy a code block and all of its contents
//
CodeBlock* CodeBlock::Clone() const
{
	CodeBlock* clone = new CodeBlock(OwnsScope ? new ScopeDescription(*Scope) : Scope, OwnsScope);
	for(std::vector<CodeBlockEntry*>::const_iterator iter = Entries.begin(); iter != Entries.end(); ++iter)
		clone->Entries.push_back((*iter)->Clone());

	return clone;
}


//
// Add an entry to the end of a code block body
//
void CodeBlock::AddEntry(CodeBlockEntry* entry)
{
	Entries.push_back(entry);
}


//
// Obtain the type of a variable active in this code block's scope,
// given the identifier handle of the variable. Might defer to any
// number of ancestor scopes to locate the actual variable.
//
Metadata::EpochTypeID CodeBlock::GetVariableTypeByID(StringHandle identifier) const
{
	return Scope->GetVariableTypeByID(identifier);
}

bool CodeBlock::GetVariableLocalOffset(StringHandle identifier, const std::map<Metadata::EpochTypeID, size_t>& sumtypesizes, size_t& outframes, size_t& outoffset, size_t& outsize) const
{
	return Scope->ComputeLocalOffset(identifier, sumtypesizes, outframes, outoffset, outsize);
}

bool CodeBlock::GetVariableParamOffset(StringHandle identifier, const std::map<Metadata::EpochTypeID, size_t>& sumtypesizes, size_t& outframes, size_t& outoffset, size_t& outsize) const
{
	return Scope->ComputeParamOffset(identifier, sumtypesizes, outframes, outoffset, outsize);
}

//
// Add a new variable to this code block's lexical scope
//
void CodeBlock::AddVariable(const std::wstring& identifier, StringHandle identifierhandle, StringHandle typenamehandle, Metadata::EpochTypeID type, bool isreference, VariableOrigin origin)
{
	Scope->AddVariable(identifier, identifierhandle, typenamehandle, type, isreference, origin);
}


//
// Perform code validation on all entries in this code block
//
bool CodeBlock::Validate(const Namespace& curnamespace) const
{
	bool valid = true;

	for(std::vector<CodeBlockEntry*>::const_iterator iter = Entries.begin(); iter != Entries.end(); ++iter)
	{
		if(!(*iter)->Validate(curnamespace))
			valid = false;
	}

	return valid;
}

//
// Request all entries in the code block to perform type inference
//
bool CodeBlock::TypeInference(Namespace& curnamespace, InferenceContext& context, CompileErrors& errors)
{
	InferenceContext newcontext(0, InferenceContext::CONTEXT_CODE_BLOCK);
	newcontext.FunctionName = context.FunctionName;

	bool valid = true;
	for(std::vector<CodeBlockEntry*>::iterator iter = Entries.begin(); iter != Entries.end(); ++iter)
	{
		if(!(*iter)->TypeInference(curnamespace, *this, newcontext, errors))
			valid = false;
	}

	return valid;
}

//
// Determine if the given identifier would cause shadowing in the current lexical scope
//
bool CodeBlock::ShadowingCheck(StringHandle identifier, CompileErrors& errors)
{
	if(Scope->HasVariable(identifier))
	{
		errors.SemanticError("Identifier already used for a variable in this scope");
		return true;
	}

	return false;
}


//
// Construct and initialize a wrapper for placing an Assignment IR node in a code block
//
CodeBlockAssignmentEntry::CodeBlockAssignmentEntry(Assignment* assignment)
	: MyAssignment(assignment)
{
}

//
// Destruct and clean up an assignment wrapper entry
//
CodeBlockAssignmentEntry::~CodeBlockAssignmentEntry()
{
	delete MyAssignment;
}

//
// Forward validation for an assignment entry to the actual IR node
//
bool CodeBlockAssignmentEntry::Validate(const Namespace& curnamespace) const
{
	if(!MyAssignment)
		return false;

	return MyAssignment->Validate(curnamespace);
}

//
// Forward type inference request for an assignment to the actual IR node
//
bool CodeBlockAssignmentEntry::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors)
{
	if(!MyAssignment)
		return false;

	return MyAssignment->TypeInference(curnamespace, activescope, context, errors);
}


//
// Construct and initialize a code block entry wrapper for a statement IR node
//
CodeBlockStatementEntry::CodeBlockStatementEntry(Statement* statement)
	: MyStatement(statement)
{
}

//
// Destruct and clean up a statement code block entry wrapper
//
CodeBlockStatementEntry::~CodeBlockStatementEntry()
{
	delete MyStatement;
}

//
// Forward validation request to the actual statement in the wrapper
//
bool CodeBlockStatementEntry::Validate(const Namespace& curnamespace) const
{
	if(!MyStatement)
		return false;

	return MyStatement->Validate(curnamespace);
}

//
// Forward type inference request to the actual statement in the wrapper
//
bool CodeBlockStatementEntry::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors)
{
	if(!MyStatement)
		return false;

	return MyStatement->TypeInference(curnamespace, activescope, context, 0, errors);
}


//
// Construct and initialize a code block entry wrapper for a pre-operator statement (++foo)
//
CodeBlockPreOpStatementEntry::CodeBlockPreOpStatementEntry(PreOpStatement* statement)
	: MyStatement(statement)
{
}

//
// Destruct and clean up a code block entry wrapper for a pre-operator statement
//
CodeBlockPreOpStatementEntry::~CodeBlockPreOpStatementEntry()
{
	delete MyStatement;
}

//
// Forward validation request to the actual pre-operator statement in the wrapper 
//
bool CodeBlockPreOpStatementEntry::Validate(const Namespace& curnamespace) const
{
	return MyStatement->Validate(curnamespace);
}

//
// Forward type inference request to the actual pre-operator statement in the wrapper
//
bool CodeBlockPreOpStatementEntry::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors)
{
	if(!MyStatement)
		return false;

	return MyStatement->TypeInference(curnamespace, activescope, context, errors);
}


//
// Construct and initialize a code block entry wrapper for a post-operator statement (foo++)
//
CodeBlockPostOpStatementEntry::CodeBlockPostOpStatementEntry(PostOpStatement* statement)
	: MyStatement(statement)
{
}

//
// Destruct and clean up a post-operator statement wrapper
//
CodeBlockPostOpStatementEntry::~CodeBlockPostOpStatementEntry()
{
	delete MyStatement;
}

//
// Forward validation request to the actual post-operator statement in the wrapper
//
bool CodeBlockPostOpStatementEntry::Validate(const Namespace& curnamespace) const
{
	return MyStatement->Validate(curnamespace);
}

//
// Forward type inference request to the actual post-operator statement in the wrapper
//
bool CodeBlockPostOpStatementEntry::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors)
{
	if(!MyStatement)
		return false;

	return MyStatement->TypeInference(curnamespace, activescope, context, errors);
}


//
// Construct and initialize a wrapper for an artificial lexical scope (inner code block)
//
CodeBlockInnerBlockEntry::CodeBlockInnerBlockEntry(CodeBlock* block)
	: MyCodeBlock(block)
{
}

//
// Destruct and clean up an artificial scope wrapper
//
CodeBlockInnerBlockEntry::~CodeBlockInnerBlockEntry()
{
	delete MyCodeBlock;
}

//
// Forward validation of an inner block to the actual code block IR node
//
bool CodeBlockInnerBlockEntry::Validate(const Namespace& curnamespace) const
{
	if(!MyCodeBlock)
		return false;

	return MyCodeBlock->Validate(curnamespace);
}

//
// Forward type inference request for an inner block to the actual IR node
//
// As an auxiliary function, adds the scope data for the code block to
// the program metadata so it can be found by the compiler and, later,
// by the VM/runtime itself.
//
bool CodeBlockInnerBlockEntry::TypeInference(Namespace& curnamespace, CodeBlock&, InferenceContext& context, CompileErrors& errors)
{
	if(!MyCodeBlock)
		return false;

	curnamespace.AddScope(MyCodeBlock->GetScope());
	return MyCodeBlock->TypeInference(curnamespace, context, errors);
}


//
// Construct and initialize a wrapper for an entity invocation
//
CodeBlockEntityEntry::CodeBlockEntityEntry(Entity* entity)
	: MyEntity(entity)
{
}

//
// Destruct and clean up a wrapper for an entity invocation
//
CodeBlockEntityEntry::~CodeBlockEntityEntry()
{
	delete MyEntity;
}

//
// Forward validation request to the actual entity invocation IR node
//
bool CodeBlockEntityEntry::Validate(const Namespace& curnamespace) const
{
	if(!MyEntity)
		return false;

	return MyEntity->Validate(curnamespace);
}

//
// Forward type inference request to the actual entity invocation IR node
//
bool CodeBlockEntityEntry::TypeInference(Namespace& curnamespace, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors)
{
	if(!MyEntity)
		return false;

	return MyEntity->TypeInference(curnamespace, activescope, context, errors);
}

//
// Deep copy a code block assignment entry
//
CodeBlockEntry* CodeBlockAssignmentEntry::Clone() const
{
	return new CodeBlockAssignmentEntry(MyAssignment->Clone());
}

//
// Deep copy a code block statement entry
//
CodeBlockEntry* CodeBlockStatementEntry::Clone() const
{
	return new CodeBlockStatementEntry(MyStatement->Clone());
}

//
// Deep copy a code block pre-operation statement entry
//
CodeBlockEntry* CodeBlockPreOpStatementEntry::Clone() const
{
	return new CodeBlockPreOpStatementEntry(MyStatement->Clone());
}

//
// Deep copy a code block post-operation statement entry
//
CodeBlockEntry* CodeBlockPostOpStatementEntry::Clone() const
{
	return new CodeBlockPostOpStatementEntry(MyStatement->Clone());
}

//
// Deep copy a nested code block
//
CodeBlockEntry* CodeBlockInnerBlockEntry::Clone() const
{
	return new CodeBlockInnerBlockEntry(MyCodeBlock->Clone());
}

//
// Deep copy a code block entity entry
//
CodeBlockEntry* CodeBlockEntityEntry::Clone() const
{
	return new CodeBlockEntityEntry(MyEntity->Clone());
}


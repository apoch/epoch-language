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

#include "Compiler/Exceptions.h"


using namespace IRSemantics;


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

CodeBlock::~CodeBlock()
{
	for(std::vector<CodeBlockEntry*>::iterator iter = Entries.begin(); iter != Entries.end(); ++iter)
		delete *iter;

	if(OwnsScope)
		delete Scope;
}

void CodeBlock::AddEntry(CodeBlockEntry* entry)
{
	Entries.push_back(entry);
}


VM::EpochTypeID CodeBlock::GetVariableTypeByID(StringHandle identifier) const
{
	return Scope->GetVariableTypeByID(identifier);
}

void CodeBlock::AddVariable(const std::wstring& identifier, StringHandle identifierhandle, VM::EpochTypeID type, bool isreference, VariableOrigin origin)
{
	Scope->AddVariable(identifier, identifierhandle, type, isreference, origin);
}

bool CodeBlock::Validate(const Program& program) const
{
	bool valid = true;

	for(std::vector<CodeBlockEntry*>::const_iterator iter = Entries.begin(); iter != Entries.end(); ++iter)
	{
		if(!(*iter)->Validate(program))
			valid = false;
	}

	return valid;
}

bool CodeBlock::CompileTimeCodeExecution(Program& program, CompileErrors& errors)
{
	for(std::vector<CodeBlockEntry*>::iterator iter = Entries.begin(); iter != Entries.end(); ++iter)
	{
		if(!(*iter)->CompileTimeCodeExecution(program, *this, errors))
			return false;
	}
	
	return true;
}

bool CodeBlock::TypeInference(Program& program, InferenceContext& context, CompileErrors& errors)
{
	InferenceContext newcontext(0, InferenceContext::CONTEXT_CODE_BLOCK);
	newcontext.FunctionName = context.FunctionName;

	for(std::vector<CodeBlockEntry*>::iterator iter = Entries.begin(); iter != Entries.end(); ++iter)
	{
		if(!(*iter)->TypeInference(program, *this, newcontext, errors))
			return false;
	}

	return true;
}


CodeBlockAssignmentEntry::CodeBlockAssignmentEntry(Assignment* assignment)
	: MyAssignment(assignment)
{
}

CodeBlockAssignmentEntry::~CodeBlockAssignmentEntry()
{
	delete MyAssignment;
}

bool CodeBlockAssignmentEntry::Validate(const Program& program) const
{
	if(!MyAssignment)
		return false;

	return MyAssignment->Validate(program);
}

bool CodeBlockAssignmentEntry::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors)
{
	if(!MyAssignment)
		return false;

	return MyAssignment->TypeInference(program, activescope, context, errors);
}


CodeBlockStatementEntry::CodeBlockStatementEntry(Statement* statement)
	: MyStatement(statement)
{
}

CodeBlockStatementEntry::~CodeBlockStatementEntry()
{
	delete MyStatement;
}

bool CodeBlockStatementEntry::Validate(const Program& program) const
{
	if(!MyStatement)
		return false;

	return MyStatement->Validate(program);
}

bool CodeBlockStatementEntry::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, CompileErrors& errors)
{
	if(!MyStatement)
		return false;

	return MyStatement->CompileTimeCodeExecution(program, activescope, false, errors);
}

bool CodeBlockStatementEntry::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors)
{
	if(!MyStatement)
		return false;

	return MyStatement->TypeInference(program, activescope, context, 0, errors);
}


CodeBlockPreOpStatementEntry::CodeBlockPreOpStatementEntry(PreOpStatement* statement)
	: MyStatement(statement)
{
}

CodeBlockPreOpStatementEntry::~CodeBlockPreOpStatementEntry()
{
	delete MyStatement;
}

bool CodeBlockPreOpStatementEntry::Validate(const Program& program) const
{
	return MyStatement->Validate(program);
}

bool CodeBlockPreOpStatementEntry::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors)
{
	if(!MyStatement)
		return false;

	return MyStatement->TypeInference(program, activescope, context, errors);
}


CodeBlockPostOpStatementEntry::CodeBlockPostOpStatementEntry(PostOpStatement* statement)
	: MyStatement(statement)
{
}

CodeBlockPostOpStatementEntry::~CodeBlockPostOpStatementEntry()
{
	delete MyStatement;
}

bool CodeBlockPostOpStatementEntry::Validate(const Program& program) const
{
	return MyStatement->Validate(program);
}

bool CodeBlockPostOpStatementEntry::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors)
{
	if(!MyStatement)
		return false;

	return MyStatement->TypeInference(program, activescope, context, errors);
}


CodeBlockInnerBlockEntry::CodeBlockInnerBlockEntry(CodeBlock* block)
	: MyCodeBlock(block)
{
}

CodeBlockInnerBlockEntry::~CodeBlockInnerBlockEntry()
{
	delete MyCodeBlock;
}

bool CodeBlockInnerBlockEntry::Validate(const Program& program) const
{
	if(!MyCodeBlock)
		return false;

	return MyCodeBlock->Validate(program);
}

bool CodeBlockInnerBlockEntry::CompileTimeCodeExecution(Program& program, CodeBlock&, CompileErrors& errors)
{
	if(!MyCodeBlock)
		return false;

	return MyCodeBlock->CompileTimeCodeExecution(program, errors);
}

bool CodeBlockInnerBlockEntry::TypeInference(Program& program, CodeBlock&, InferenceContext& context, CompileErrors& errors)
{
	if(!MyCodeBlock)
		return false;

	program.AddScope(MyCodeBlock->GetScope());
	return MyCodeBlock->TypeInference(program, context, errors);
}


CodeBlockEntityEntry::CodeBlockEntityEntry(Entity* entity)
	: MyEntity(entity)
{
}

CodeBlockEntityEntry::~CodeBlockEntityEntry()
{
	delete MyEntity;
}

bool CodeBlockEntityEntry::Validate(const Program& program) const
{
	if(!MyEntity)
		return false;

	return MyEntity->Validate(program);
}

bool CodeBlockEntityEntry::CompileTimeCodeExecution(Program& program, CodeBlock& activescope, CompileErrors& errors)
{
	if(!MyEntity)
		return false;

	return MyEntity->CompileTimeCodeExecution(program, activescope, errors);
}

bool CodeBlockEntityEntry::TypeInference(Program& program, CodeBlock& activescope, InferenceContext& context, CompileErrors& errors)
{
	if(!MyEntity)
		return false;

	return MyEntity->TypeInference(program, activescope, context, errors);
}


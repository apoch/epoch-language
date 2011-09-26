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


using namespace IRSemantics;


CodeBlock::CodeBlock(ScopeDescription* scope)
	: Scope(scope)
{
	if(!Scope)
		throw std::exception("Code block must be bound to a scope!");		// TODO - better exceptions
}

CodeBlock::~CodeBlock()
{
	for(std::vector<CodeBlockEntry*>::iterator iter = Entries.begin(); iter != Entries.end(); ++iter)
		delete *iter;

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

bool CodeBlock::CompileTimeCodeExecution(Program& program)
{
	for(std::vector<CodeBlockEntry*>::iterator iter = Entries.begin(); iter != Entries.end(); ++iter)
	{
		if(!(*iter)->CompileTimeCodeExecution(program, *this))
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
	// TODO - type validation on assigments
	return true;
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

bool CodeBlockStatementEntry::CompileTimeCodeExecution(Program& program, CodeBlock& activescope)
{
	if(!MyStatement)
		return false;

	return MyStatement->CompileTimeCodeExecution(program, activescope, false);
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
	// TODO - type validation on preops
	return true;
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
	// TODO - type validation on postops
	return true;
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

bool CodeBlockInnerBlockEntry::CompileTimeCodeExecution(Program& program, CodeBlock& activescope)
{
	if(!MyCodeBlock)
		return false;

	return MyCodeBlock->CompileTimeCodeExecution(program);
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

bool CodeBlockEntityEntry::CompileTimeCodeExecution(Program& program, CodeBlock& activescope)
{
	if(!MyEntity)
		return false;

	return MyEntity->CompileTimeCodeExecution(program);
}


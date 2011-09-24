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


CodeBlock::~CodeBlock()
{
	for(std::vector<CodeBlockEntry*>::iterator iter = Entries.begin(); iter != Entries.end(); ++iter)
		delete *iter;
}

void CodeBlock::AddEntry(CodeBlockEntry* entry)
{
	Entries.push_back(entry);
}


VM::EpochTypeID CodeBlock::GetVariableTypeByID(StringHandle identifier) const
{
	// TODO - implement type tracking for all identifiers
	return VM::EpochType_Error;
}


CodeBlockAssignmentEntry::CodeBlockAssignmentEntry(Assignment* assignment)
	: MyAssignment(assignment)
{
}

CodeBlockAssignmentEntry::~CodeBlockAssignmentEntry()
{
	delete MyAssignment;
}


CodeBlockStatementEntry::CodeBlockStatementEntry(Statement* statement)
	: MyStatement(statement)
{
}

CodeBlockStatementEntry::~CodeBlockStatementEntry()
{
	delete MyStatement;
}


CodeBlockPreOpStatementEntry::CodeBlockPreOpStatementEntry(PreOpStatement* statement)
	: MyStatement(statement)
{
}

CodeBlockPreOpStatementEntry::~CodeBlockPreOpStatementEntry()
{
	delete MyStatement;
}


CodeBlockPostOpStatementEntry::CodeBlockPostOpStatementEntry(PostOpStatement* statement)
	: MyStatement(statement)
{
}

CodeBlockPostOpStatementEntry::~CodeBlockPostOpStatementEntry()
{
	delete MyStatement;
}


CodeBlockInnerBlockEntry::CodeBlockInnerBlockEntry(CodeBlock* block)
	: MyCodeBlock(block)
{
}

CodeBlockInnerBlockEntry::~CodeBlockInnerBlockEntry()
{
	delete MyCodeBlock;
}


CodeBlockEntityEntry::CodeBlockEntityEntry(Entity* entity)
	: MyEntity(entity)
{
}

CodeBlockEntityEntry::~CodeBlockEntityEntry()
{
	delete MyEntity;
}


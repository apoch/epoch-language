//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper for tracking debugging information about a loaded program
//

#include "pch.h"

#include "Parser/Debug Info Tables/DebugTable.h"
#include "Parser/Error Handling/ParserExceptions.h"

#include "Virtual Machine/Core Entities/Operation.h"


//
// Store the file position corresponding to a given operation
//
void DebugTable::TrackInstruction(const VM::Operation* op, const FileLocationInfo& fileinfo)
{
	while(op != NULL)
	{
		InstructionLocationTable[op] = fileinfo;
		op = op->GetNestedOperation();
	}
}


//
// Retrieve the file position that corresponds to a given operation
//
const FileLocationInfo& DebugTable::GetInstructionLocation(const VM::Operation* op) const
{
	std::map<const VM::Operation*, FileLocationInfo>::const_iterator iter = InstructionLocationTable.find(op);
	if(iter == InstructionLocationTable.end())
		throw Parser::ParserFailureException("Instruction is not recorded in the debug table");

	return iter->second;
}


//
// Record the name of a forked task
//
// Strictly speaking, the name of a forked task is not known at compile time,
// because it is a string variable that is evaluated at runtime. So what we
// store here is actually the raw source that provides the task name.
//
void DebugTable::TrackTaskName(const VM::Operations::ForkTask* forkop, const std::wstring& taskname)
{
	TaskNameTable[forkop] = taskname;
}


//
// Retrieve the code that generates the title of a forked task
//
const std::wstring& DebugTable::GetTaskName(const VM::Operations::ForkTask* forkop) const
{
	std::map<const VM::Operations::ForkTask*, std::wstring>::const_iterator iter = TaskNameTable.find(forkop);
	if(iter == TaskNameTable.end())
		throw Parser::ParserFailureException("Task is not recorded in the debug table");

	return iter->second;
}


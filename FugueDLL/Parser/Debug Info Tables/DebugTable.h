//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper for tracking debugging information about a loaded program
//

#pragma once


// Forward declarations
namespace VM
{
	class Operation;

	namespace Operations
	{
		class ForkTask;
	}
}


struct FileLocationInfo
{
	std::wstring FileName;
	unsigned Line;
	unsigned Column;
};


class DebugTable
{
// Information interface
public:
	void TrackInstruction(const VM::Operation* op, const FileLocationInfo& fileinfo);
	const FileLocationInfo& GetInstructionLocation(const VM::Operation* op) const;

	void TrackTaskName(const VM::Operation* forkop, const std::wstring& taskname);
	const std::wstring& GetTaskName(const VM::Operation* forkop) const;

// Internal tracking
private:
	std::map<const VM::Operation*, FileLocationInfo> InstructionLocationTable;
	std::map<const VM::Operation*, std::wstring> TaskNameTable;
};


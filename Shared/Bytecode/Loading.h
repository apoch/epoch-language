//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Functions for converting binary opcodes into VM objects
//

#pragma once

// Forward declarations
namespace VM
{
	class Program;
	class ScopeDescription;
	class FunctionBase;
	class Block;
}
class FileLoader;

// Dependencies
#include "Virtual Machine/Core Entities/Types/FunctionSignature.h"


class FileLoader
{
// Construction and destruction
public:
	FileLoader(const void* buffer);
	~FileLoader();

// Result retrieval
public:
	VM::Program* GetProgram()
	{ return LoadingProgram; }

// Internal helpers for cleanup
private:
	void Clean();

// Internal helpers for validation/sanity checks
private:
	void CheckCookie();
	void ExpectInstruction(unsigned char instruction);

// Internal helpers for converting bytecode into runtime operation objects
private:
	void GenerateOpFromByteCode(unsigned char instruction, VM::Block* newblock);

// Internal helpers for reading data chunks
private:
	Integer32 ReadNumber();
	Real ReadFloat();
	bool ReadFlag();
	std::string ReadNullTerminatedString();
	std::string ReadStringByLength(Integer32 len);
	unsigned char ReadInstruction();
	unsigned char PeekInstruction();

	VM::ScopeDescription* LoadScope(bool linktoglobal);
	void LoadGlobalInitBlock();
	VM::Block* LoadCodeBlock();
	VM::FunctionSignature LoadFunctionSignature();

	const std::wstring& WidenAndCache(const std::string& str);

	VM::ScopeDescription* RegisterScopeToDelete(VM::ScopeDescription* scope);
	VM::ScopeDescription* UnregisterScopeToDelete(VM::ScopeDescription* scope);

// Internal tracking
private:
	const UByte* Buffer;
	ptrdiff_t Offset;
	VM::Program* LoadingProgram;
	bool IsPrepass;

	std::map<ScopeID, VM::ScopeDescription*> ScopeIDMap;
	std::map<FunctionID, VM::FunctionBase*> FunctionIDMap;

	std::set<VM::ScopeDescription*> DeleteScopes;
};


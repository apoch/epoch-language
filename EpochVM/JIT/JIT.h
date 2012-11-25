#pragma once

namespace VM
{
	class VirtualMachine;
}


typedef void (__cdecl *JITExecPtr)(char** pstack, void* context);
void JITByteCode(const VM::VirtualMachine& ownervm, const Bytecode::Instruction* bytecode, size_t beginoffset, size_t endoffset, StringHandle alias);
void PopulateJITExecs(VM::VirtualMachine& ownervm);

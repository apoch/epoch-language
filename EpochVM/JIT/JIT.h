#pragma once

namespace VM
{
	class VirtualMachine;
}


typedef void (*JITExecPtr)(char** pstack, void* context);
JITExecPtr JITByteCode(const VM::VirtualMachine& ownervm, const Bytecode::Instruction* bytecode, size_t beginoffset, size_t endoffset);
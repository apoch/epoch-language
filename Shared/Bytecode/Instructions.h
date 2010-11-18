//
// The Epoch Language Project
// Shared Library Code
//
// Definitions for bytecode instruction values
//

#pragma once


namespace Bytecode
{

	typedef unsigned char Instruction;

	namespace Instructions
	{

		static const Instruction Halt = 0x00;
		static const Instruction NoOp = 0x01;

		static const Instruction Push = 0x02;
		static const Instruction BindRef = 0x03;
		static const Instruction Pop = 0x04;
		static const Instruction Read = 0x05;
		static const Instruction Assign = 0x06;

		static const Instruction Invoke = 0x07;
		static const Instruction InvokeIndirect = 0x08;
		static const Instruction Return = 0x09;
		static const Instruction SetRetVal = 0x0a;

		static const Instruction BeginEntity = 0x0b;
		static const Instruction EndEntity = 0x0c;
		static const Instruction BeginChain = 0x0d;
		static const Instruction EndChain = 0x0e;
		static const Instruction InvokeMeta = 0x0f;

		static const Instruction PoolString = 0x10;
		static const Instruction DefineLexicalScope = 0x11;
		static const Instruction PatternMatch = 0x12;

		static const Instruction AllocStructure = 0x13;
		static const Instruction DefineStructure = 0x14;
		static const Instruction CopyFromStructure = 0x15;
		static const Instruction CopyToStructure = 0x16;

		static const Instruction ReadRef = 0x17;
		static const Instruction BindMemberRef = 0x18;

	}

}


//
// The Epoch Language Project
// Shared Library Code
//
// Definitions for bytecode instruction values
//

#pragma once


namespace Bytecode
{

	// We prefer a direct use of a byte to the use of an enum because we don't
	// always want an enum's semantics during compilation of the compiler/VM.
	// For instance, we may wish to define custom instructions at some point
	// which do not fit into preset enum values but should still occupy a single
	// byte of space. This also helps to ensure that we don't accidentally widen
	// the enum past a byte and cause havoc with code that assumes byte widths.
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

		static const Instruction CopyBuffer = 0x19;

		static const Instruction Tag = 0x1a;

		static const Instruction CopyStructure = 0x1b;

		static const Instruction AssignThroughIdentifier = 0x1c;

		static const Instruction BindMemberByHandle = 0x1d;

		static const Instruction InvokeNative = 0x1e;

		static const Instruction SumTypeDef = 0x1f;

		static const Instruction TypeMatch = 0x20;

		static const Instruction ConstructSumType = 0x21;

		static const Instruction TempReferenceFromRegister = 0x22;

		static const Instruction AssignSumType = 0x23;

		static const Instruction ReadRefAnnotated = 0x24;

	}

}


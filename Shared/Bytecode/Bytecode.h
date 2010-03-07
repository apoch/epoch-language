//
// The Epoch Language Project
// FUGUE Bytecode assembler/disassembler
//
// Constant declarations for mapping Epoch Assembler into bytecode
//

#pragma once

namespace Bytecode
{
	extern const char* HeaderCookie;

	extern const unsigned char NullFlag;
	extern const unsigned char NoOp;
	extern const unsigned char ParentScope;
	extern const unsigned char Variables;
	extern const unsigned char Ghosts;
	extern const unsigned char Functions;
	extern const unsigned char CallDLL;
	extern const unsigned char Reference;
	extern const unsigned char StructureHints;
	extern const unsigned char BeginBlock;
	extern const unsigned char EndBlock;
	extern const unsigned char Invoke;
	extern const unsigned char PushOperation;
	extern const unsigned char PushIntegerLiteral;
	extern const unsigned char PushStringLiteral;
	extern const unsigned char AssignValue;
	extern const unsigned char GetValue;
	extern const unsigned char BindFunctionReference;
	extern const unsigned char SizeOf;
	extern const unsigned char IsNotEqual;
	extern const unsigned char If;
	extern const unsigned char While;
	extern const unsigned char BindReference;
	extern const unsigned char WhileCondition;
	extern const unsigned char Scope;
	extern const unsigned char GhostRecord;
	extern const unsigned char IsEqual;
	extern const unsigned char ElseIfWrapper;
	extern const unsigned char ElseIf;
	extern const unsigned char ExitIfChain;
	extern const unsigned char Members;
	extern const unsigned char StaticStrings;
	extern const unsigned char StringVars;
	extern const unsigned char TupleStaticData;
	extern const unsigned char IDCounter;
	extern const unsigned char StructureStaticData;
	extern const unsigned char TupleTypes;
	extern const unsigned char TupleHints;
	extern const unsigned char StructureTypes;
	extern const unsigned char StructureTypeMap;
	extern const unsigned char TupleTypeMap;
	extern const unsigned char EndScope;
	extern const unsigned char TypeCast;
	extern const unsigned char DebugWrite;
	extern const unsigned char PushRealLiteral;
	extern const unsigned char DoWhile;
	extern const unsigned char DivideReals;
	extern const unsigned char AddReals;
	extern const unsigned char SubReals;
	extern const unsigned char IsLesser;
	extern const unsigned char PushBooleanLiteral;
	extern const unsigned char AddIntegers;
	extern const unsigned char SubtractIntegers;
	extern const unsigned char IsGreater;
	extern const unsigned char DebugRead;
	extern const unsigned char TypeCastToString;
	extern const unsigned char ReadTuple;
	extern const unsigned char WriteTuple;
	extern const unsigned char ReadStructure;
	extern const unsigned char WriteStructure;
	extern const unsigned char GlobalBlock;
	extern const unsigned char Init;
	extern const unsigned char PushInteger16Literal;
	extern const unsigned char DivideIntegers;
	extern const unsigned char Concat;
	extern const unsigned char IsGreaterEqual;
	extern const unsigned char IsLesserEqual;
	extern const unsigned char DivideInteger16s;
	extern const unsigned char BitwiseOr;
	extern const unsigned char BitwiseAnd;
	extern const unsigned char BitwiseXor;
	extern const unsigned char BitwiseNot;
	extern const unsigned char LogicalOr;
	extern const unsigned char LogicalAnd;
	extern const unsigned char LogicalXor;
	extern const unsigned char LogicalNot;
	extern const unsigned char InvokeIndirect;
	extern const unsigned char Break;
	extern const unsigned char Return;
	extern const unsigned char BooleanLiteral;
	extern const unsigned char Futures;
	extern const unsigned char ReadStructureIndirect;
	extern const unsigned char BindStruct;
	extern const unsigned char WriteStructureIndirect;
	extern const unsigned char Constants;
	extern const unsigned char FunctionSignatureList;
	extern const unsigned char FunctionSignatureBegin;
	extern const unsigned char FunctionSignatureEnd;
	extern const unsigned char ForkTask;
	extern const unsigned char ResponseMaps;
	extern const unsigned char AcceptMessage;
	extern const unsigned char MultiplyIntegers;
	extern const unsigned char GetMessageSender;
	extern const unsigned char GetTaskCaller;
	extern const unsigned char SendTaskMessage;
	extern const unsigned char AcceptMessageFromMap;
	extern const unsigned char DebugCrashVM;
	extern const unsigned char Future;
	extern const unsigned char Map;
	extern const unsigned char Reduce;
	extern const unsigned char IntegerLiteral;
	extern const unsigned char ThreadPool;
	extern const unsigned char ForkThread;
	extern const unsigned char Handoff;
	extern const unsigned char ExtensionData;
	extern const unsigned char ReadArray;
	extern const unsigned char WriteArray;
	extern const unsigned char ConsArrayIndirect;
	extern const unsigned char ArrayLength;
	extern const unsigned char ParallelFor;
	extern const unsigned char HandoffControl;
	extern const unsigned char ArrayHints;
	extern const unsigned char ConsArray;
	extern const unsigned char Length;
}



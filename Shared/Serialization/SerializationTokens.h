//
// The Epoch Language Project
// Shared Library Code
//
// Constants for serializing code to Epoch Assembly format
//

#pragma once


namespace Serialization
{
	extern std::wstring EmptyString;

	// Utility operations
	extern std::wstring NoOp;

	// Function calling
	extern std::wstring Invoke;
	extern std::wstring InvokeIndirect;

	// Lexical scopes
	extern std::wstring Scope;
	extern std::wstring EndScope;
	extern std::wstring Variables;
	extern std::wstring Functions;
	extern std::wstring Ghosts;
	extern std::wstring GhostRecord;
	extern std::wstring ParentScope;
	extern std::wstring TupleTypes;
	extern std::wstring TupleTypeHints;
	extern std::wstring TupleTypeMap;
	extern std::wstring StructureTypes;
	extern std::wstring StructureTypeHints;
	extern std::wstring StructureTypeMap;
	extern std::wstring FunctionSignatureList;
	extern std::wstring FunctionSignatureBegin;
	extern std::wstring FunctionSignatureEnd;
	extern std::wstring Reference;
	extern std::wstring ResponseMaps;

	// Stack operations
	extern std::wstring PushIntegerLiteral;
	extern std::wstring PushInteger16Literal;
	extern std::wstring PushRealLiteral;
	extern std::wstring PushStringLiteral;
	extern std::wstring PushBooleanLiteral;
	extern std::wstring PushOperation;
	extern std::wstring BindReference;
	extern std::wstring BindFunctionReference;

	// Flow control
	extern std::wstring If;
	extern std::wstring ElseIfWrapper;
	extern std::wstring ElseIf;
	extern std::wstring ExitIfChain;
	extern std::wstring DoWhile;
	extern std::wstring While;
	extern std::wstring WhileCondition;
	extern std::wstring Break;
	extern std::wstring Return;

	// Variables and constants
	extern std::wstring AssignValue;
	extern std::wstring InitializeValue;
	extern std::wstring GetValue;
	extern std::wstring SizeOf;
	extern std::wstring Length;
	extern std::wstring IntegerConstant;
	extern std::wstring RealConstant;
	extern std::wstring BooleanConstant;

	// Tuples
	extern std::wstring ReadTuple;
	extern std::wstring AssignTuple;

	// Structures
	extern std::wstring ReadStructure;
	extern std::wstring ReadStructureIndirect;
	extern std::wstring AssignStructure;
	extern std::wstring AssignStructureIndirect;
	extern std::wstring BindStructMemberReference;

	// Lists
	extern std::wstring ConsList;
	extern std::wstring Map;
	extern std::wstring Reduce;

	// Message passing
	extern std::wstring AcceptMessage;
	extern std::wstring AcceptMessageFromMap;
	extern std::wstring SendTaskMessage;
	extern std::wstring GetTaskCaller;
	extern std::wstring GetMessageSender;

	// Built-in arithmetic operations
	extern std::wstring AddIntegers;
	extern std::wstring AddInteger16s;
	extern std::wstring AddReals;
	extern std::wstring SubtractIntegers;
	extern std::wstring SubtractInteger16s;
	extern std::wstring SubtractReals;
	extern std::wstring MultiplyIntegers;
	extern std::wstring MultiplyInteger16s;
	extern std::wstring MultiplyReals;
	extern std::wstring DivideIntegers;
	extern std::wstring DivideInteger16s;
	extern std::wstring DivideReals;
	extern std::wstring Negate;

	// Built-in comparison operations
	extern std::wstring IsEqual;
	extern std::wstring IsNotEqual;
	extern std::wstring IsGreater;
	extern std::wstring IsGreaterEqual;
	extern std::wstring IsLesser;
	extern std::wstring IsLesserEqual;

	// Built-in bitwise operations
	extern std::wstring BitwiseOr;
	extern std::wstring BitwiseAnd;
	extern std::wstring BitwiseXor;
	extern std::wstring BitwiseNot;

	// Built-in logic operations
	extern std::wstring LogicalOr;
	extern std::wstring LogicalAnd;
	extern std::wstring LogicalXor;
	extern std::wstring LogicalNot;

	// String operations
	extern std::wstring Concat;

	// Built in conversion/type-cast operations
	extern std::wstring TypeCast;
	extern std::wstring TypeCastToString;

	// Asynchronous tasks
	extern std::wstring ForkTask;
	extern std::wstring ForkFuture;

	// Debug operations
	extern std::wstring DebugWrite;
	extern std::wstring DebugRead;
	extern std::wstring DebugCrashVM;

	// External DLL calls
	extern std::wstring CallDLL;

	// Language extensions
	extern std::wstring Handoff;

	// Miscellaneous tokens
	extern std::wstring Null;
	extern std::wstring BeginBlock;
	extern std::wstring EndBlock;
	extern std::wstring True;
	extern std::wstring False;
	extern std::wstring StaticStrings;
	extern std::wstring StringVars;
	extern std::wstring Members;
	extern std::wstring IDCounter;
	extern std::wstring TupleStaticData;
	extern std::wstring StructureStaticData;
	extern std::wstring GlobalBlock;
	extern std::wstring Constants;
	extern std::wstring Futures;
	extern std::wstring ListTypes;
	extern std::wstring ListSizes;
}

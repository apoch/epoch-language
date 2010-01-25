//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Constant declarations for keywords used by the Epoch language
//

#pragma once


namespace Keywords
{

	//-------------------------------------------------------------------------------
	// Keyword constants
	//-------------------------------------------------------------------------------

	extern const wchar_t* If;
	extern const wchar_t* ElseIf;
	extern const wchar_t* Else;

	extern const wchar_t* Do;
	extern const wchar_t* While;

	extern const wchar_t* Break;
	extern const wchar_t* Return;

	extern const wchar_t* Assign;
	extern const wchar_t* ReadTuple;
	extern const wchar_t* AssignTuple;
	extern const wchar_t* ReadStructure;
	extern const wchar_t* AssignStructure;
	extern const wchar_t* Member;

	extern const wchar_t* Add;
	extern const wchar_t* Subtract;
	extern const wchar_t* Multiply;
	extern const wchar_t* Divide;

	extern const wchar_t* Concat;
	extern const wchar_t* Length;

	extern const wchar_t* Equal;
	extern const wchar_t* NotEqual;
	extern const wchar_t* Less;
	extern const wchar_t* Greater;
	extern const wchar_t* LessEqual;
	extern const wchar_t* GreaterEqual;
	
	extern const wchar_t* DebugWrite;
	extern const wchar_t* DebugRead;

	extern const wchar_t* Map;
	extern const wchar_t* Reduce;

	extern const wchar_t* Cast;

	extern const wchar_t* SizeOf;

	extern const wchar_t* Or;
	extern const wchar_t* And;
	extern const wchar_t* Xor;
	extern const wchar_t* Not;

	extern const wchar_t* Message;
	extern const wchar_t* AcceptMessage;
	extern const wchar_t* ResponseMap;
	extern const wchar_t* Caller;
	extern const wchar_t* Sender;

	extern const wchar_t* Var;
	extern const wchar_t* Integer;
	extern const wchar_t* Integer16;
	extern const wchar_t* String;
	extern const wchar_t* Boolean;
	extern const wchar_t* Real;
	extern const wchar_t* Tuple;
	extern const wchar_t* Structure;
	extern const wchar_t* List;
	extern const wchar_t* Buffer;

	extern const wchar_t* Reference;
	extern const wchar_t* Constant;

	extern const wchar_t* External;
	extern const wchar_t* Library;
	extern const wchar_t* Extension;

	extern const wchar_t* Function;
	extern const wchar_t* Global;

	extern const wchar_t* Task;
	extern const wchar_t* Thread;
	extern const wchar_t* ThreadPool;
	extern const wchar_t* Future;

	extern const wchar_t* True;
	extern const wchar_t* False;

	extern const wchar_t* EntryPoint;

	extern const wchar_t* Infix;
	extern const wchar_t* Alias;

	extern const wchar_t* Comment;
	extern const wchar_t* FunctionArrow;
	extern const wchar_t* NullFunctionArrow;
	extern const wchar_t* HexPrefix;

	extern const wchar_t* DebugCrashVM;


	//-------------------------------------------------------------------------------
	// Additional utility functions
	//-------------------------------------------------------------------------------

	const char* GetNarrowedKeyword(const wchar_t* keyword);
	const char* GetNarrowedKeyword(const char* keyword);

}


//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Semantic analysis state management class.
//

#pragma once

// Dependencies
#include "Virtual Machine/Core Entities/RValue.h"
#include "Virtual Machine/Core Entities/Variables/Variable.h"
#include "Virtual Machine/Core Entities/Types/FunctionSignature.h"

#include "Parser/Debug Info Tables/DebugTable.h"

#include <boost/spirit/include/classic.hpp>


// Forward declarations
namespace VM
{
	class Program;
	class Block;
	class ScopeDescription;
	class Operation;
	class ResponseMap;
	class StructureType;

	typedef std::auto_ptr<Operation> OperationPtr;
}


namespace Parser
{

	// Handy type shortcuts
	typedef boost::spirit::classic::position_iterator<const char*> ParsePosIter;

	//
	// Various slots for storing identifiers in the parser state machine
	//
	enum SavedStringIndex
	{
		SavedStringSlot_AcceptMsg,
		SavedStringSlot_IncDec,
		SavedStringSlot_OpAssign,
		SavedStringSlot_Alias,
		SavedStringSlot_InfixLValue,

		SavedStringSlot_Max			// Always leave this as the last enum!
	};

	//
	// Wrapper class for encapsulating the state of parsing and semantic analysis.
	// This class is responsible for accepting input from the parser grammar (via
	// boost::spirit) and converting it to executable code instructions.
	//
	class ParserState
	{
	// Construction and destruction
	public:
		ParserState();
		~ParserState();

	// Functions
	public:
		void RegisterUpcomingFunction(const std::wstring& functionname);
		void RegisterUpcomingFunctionPP(const std::wstring& functionname);

	// Function parameters
	public:
		void RegisterParam(const std::wstring& paramname);

		void RegisterUpcomingIntegerParam();
		void RegisterUpcomingInt16Param();
		void RegisterUpcomingRealParam();
		void RegisterUpcomingStringParam();
		void RegisterUpcomingBooleanParam();
		void RegisterUpcomingBufferParam();
		void RegisterUpcomingUnknownParam(const std::wstring& nameoftype);

		void RegisterParamIsReference();
		void RegisterParamIsArray();

	// Function returns
	public:
		void RegisterBeginningOfFunctionReturns();
		void ExitUnknownReturnConstructor();
		void FinishReturnConstructor();

		void RegisterIntegerReturn(const std::wstring& retname);
		void RegisterInt16Return(const std::wstring& retname);
		void RegisterRealReturn(const std::wstring& retname);
		void RegisterStringReturn(const std::wstring& retname);
		void RegisterBooleanReturn(const std::wstring& retname);
		void RegisterUnknownReturn(const std::wstring& rettype);
		void RegisterUnknownReturnName(const std::wstring& retname);
		void RegisterNullReturn();

		void RegisterReturnValue(Integer32 value);
		void RegisterReturnValue(Real value);
		void RegisterReturnValue(const std::wstring& value);
		void RegisterReturnValue(bool value);

	// Higher order functions
	public:
		void RegisterFunctionSignatureName(const std::wstring& identifier);
		void RegisterFunctionSignatureParam(VM::EpochVariableTypeID type);
		void RegisterFunctionSignatureParam(const std::wstring& nameoftype);
		void RegisterFunctionSignatureParamIsReference();
		void RegisterFunctionSignatureReturn(VM::EpochVariableTypeID type);
		void RegisterFunctionSignatureReturn(const std::wstring& nameoftype);
		void RegisterFunctionSignatureEnd();

	// External functions
	public:
		void RegisterExternalFunctionName(const std::wstring& functionname);
		void RegisterExternalDLL(const std::wstring& dllname);

		void RegisterExternalIntegerReturn();
		void RegisterExternalInt16Return();
		void RegisterExternalRealReturn();
		void RegisterExternalStringReturn();
		void RegisterExternalBooleanReturn();
		void RegisterExternalBufferReturn();
		void RegisterExternalNullReturn();

		void RegisterExternalParamAddressType(const std::wstring& paramtype);
		void RegisterExternalParamAddressName(const std::wstring& paramname);

	// Parameter passing
	public:
		void StartCountingParams();
		void PopParameterCount();
		void CountParameter();
		void PushOperationAsParameter(const std::wstring& operationname);
		void PushIdentifier(const std::wstring& identifier);
		void PushIdentifierAsParameter(const std::wstring& identifier);
		void PushIntegerLiteral(Integer32 value);
		void PushRealLiteral(Real value);
		void PushStringLiteral(const std::wstring& value);
		void PushStringLiteralNoStack(const std::wstring& value);
		void PushBooleanLiteral(bool value);

	// Variable definitions
	public:
		void RegisterUpcomingVariable(VM::EpochVariableTypeID type);
		void RegisterUpcomingInferredVariable();
		void RegisterVariableName(const std::wstring& variablename);
		void RegisterVariableValue();
		void RegisterUpcomingConstant();

	// Arrays
	public:
		void RegisterArrayVariable();
		void RegisterArrayType(const std::wstring& type);

	// Tuples
	public:
		void RegisterTupleType(const std::wstring& identifier);
		void RegisterTupleMember(const std::wstring& identifier, VM::EpochVariableTypeID type);
		void FinishTupleType();

	// Structures
	public:
		void RegisterStructureType(const std::wstring& identifier);
		void RegisterStructureMember(const std::wstring& identifier, VM::EpochVariableTypeID type);
		void RegisterStructureUnknownTypeName(const std::wstring& type);
		void RegisterStructureMemberUnknown(const std::wstring& identifier);
		void FinishStructureType();
		void IncrementMemberLevel();
		void ResetMemberLevel();

	// Blocks and flow control
	public:
		void RegisterControl(const std::wstring& controlname, bool preprocess);

		void PopDoWhileLoop();
		void PopDoWhileLoopPP();

		void RegisterEndOfWhileLoopConditional();
		void RegisterEndOfParallelFor();

		void EnterBlock();
		void EnterBlockPP();
		void ExitBlock();
		void ExitBlockPP();

		void EnterGlobalBlock();
		void ExitGlobalBlock();

	// Tasks and messaging
	public:
		void BeginTaskCode();
		void BeginThreadCode();

		void BeginResponseMap();
		void EndResponseMap();
		void RegisterResponseMapEntry();

		void RegisterUpcomingMessageDispatch(bool ispreparse);

		void RegisterIntegerMessageParam(const std::wstring& paramname);
		void RegisterInt16MessageParam(const std::wstring& paramname);
		void RegisterRealMessageParam(const std::wstring& paramname);
		void RegisterBooleanMessageParam(const std::wstring& paramname);
		void RegisterStringMessageParam(const std::wstring& paramname);

		void ResetMessageParamFlags();
		void RegisterArrayMessageParam();

		void StartMessageParamScope();
		void RegisterMessageDispatchCode();

		void PushCallerOperation();
		void PushSenderOperation();

		void RegisterThreadPool();

	// Infix operators
	public:
		void PushInfixOperator(const std::wstring& opname);
		void RegisterInfixOperand();
		void RegisterInfixOperandAsLValue(const std::wstring& lvaluename);
		void TerminateInfixExpression();
		void TerminateParenthetical();
		void ResetInfixTracking();
		void RegisterNotOperation();
		void RegisterNegateOperation();
		void UndoNegateOperation();

		std::wstring LookupInfixAlias(const std::wstring& opname) const;
		unsigned GetInfixPrecedence(const std::wstring& opname) const;

		void RegisterUserDefinedInfix();

		template<typename T>
		void SetUpUserInfixOps(T& target)
		{
			for(std::set<std::string>::const_iterator iter = UserInfixOperators.begin(); iter != UserInfixOperators.end(); ++iter)
				target.AddUserInfixOp(iter->c_str());
		}
		
		void RegisterOpAssignment();
		void RegisterOpAssignmentOperator(const std::wstring& op);

		void RegisterCompositeLValue();
		void RegisterMemberAccess(const std::wstring& membername);
		void RegisterMemberLValueAccess(const std::wstring& membername);
		void ResetMemberAccess();
		void ResetMemberAccessLValue();
		void ResetMemberAccessRValue();
		void FinalizeCompositeAssignment();

		void PreincrementVariable();
		void PredecrementVariable();
		void PostincrementVariable();
		void PostdecrementVariable();

		void HandleInlineIncDec();

	// Internal infix helpers
	private:
		bool FinalizeInfixExpression(bool isfirstrun, const VM::ScopeDescription& scope);
		
	// Type aliases
	public:
		void CreateTypeAlias(const std::wstring& type);

		template<typename T>
		void SetUpTypeAliases(T& target, ParserState& state)
		{
			for(std::map<std::string, VM::EpochVariableTypeID>::const_iterator iter = TypeAliases.begin(); iter != TypeAliases.end(); ++iter)
				target.AddTypeAlias(iter->first.c_str(), iter->second, state);
		}

		const char* ResolveAlias(const std::wstring& originalname) const;

	// Additional helpers
	public:
		void AddOperationDeferred(VM::OperationPtr op);
		void MergeDeferredOperations();

		void CacheTailOperations();
		void PushCachedOperations();

	// External libraries
	public:
		void RegisterLibrary(const std::wstring& filename);

	// Language extensions
	public:
		void RegisterExtension(const std::wstring& filename);

		void PushExtensionBlockKeyword(const std::wstring& keyword);
		void RegisterExtensionBlock();

		void RegisterEndOfExtensionControl();

	// Additional tracking
	public:
		void PushOperation(const std::wstring& operationname);
		void SaveStringIdentifier(const std::wstring& identifier, SavedStringIndex slotindex);
		void PushSavedIdentifier(SavedStringIndex slotindex);
		void SaveTaskName(const std::wstring& taskname);
		void QueueControlVariable(const std::wstring& varname, VM::EpochVariableTypeID type);

	// Bindings to raw source code for outputting hints when errors occur
	public:
		void SetCodeBuffer(Byte* buffer)
		{ CodeBuffer = buffer; }

		void DumpCodeLine(unsigned line, unsigned column, unsigned tabwidth) const;

	// Parse position tracking, used for outputting hints when errors occur
	public:
		void SetParsePosition(const ParsePosIter& pos);

		const ParsePosIter& GetParsePosition() const
		{ return ParsePosition; }

	// Error reporting
	public:
		void ReportFatalError(const char* what);

	// Access to the final parsed and execution-ready program
	public:
		VM::Program* GetParsedProgram() const
		{ return ParsedProgram; }

	// Flag for tracking success/failure of parse process
	public:
		bool ParseFailed;

	// Internal helper structures and type shortcuts
	private:

		//
		// Track a function's return variable and its initial value
		//
		struct ReturnValEntry
		{
			VM::EpochVariableTypeID Type;

			union
			{
				Integer32 IntegerValue;
				bool BooleanValue;
				Real RealValue;
			};
			std::wstring StringValue;
		};

		typedef std::pair<std::wstring, ReturnValEntry> FunctionRetEntry;
		typedef std::map<std::wstring, ReturnValEntry> FunctionRetMap;

	public:
		//
		// Entry on the analysis stack. The analyzer uses a stack
		// of these entries to keep track of what operations are
		// currently pending, as well as their parameters.
		//
		struct StackEntry
		{
			enum
			{
				STACKENTRYTYPE_INTEGER_LITERAL,
				STACKENTRYTYPE_REAL_LITERAL,
				STACKENTRYTYPE_BOOLEAN_LITERAL,
				STACKENTRYTYPE_STRING_LITERAL,
				STACKENTRYTYPE_IDENTIFIER,
				STACKENTRYTYPE_OPERATION,
				STACKENTRYTYPE_SCOPE
			} Type;

			union
			{
				Integer32 IntegerValue;
				bool BooleanValue;
				Real RealValue;
				VM::Operation* OperationPointer;
				VM::ScopeDescription* ScopePointer;
			};
			std::wstring StringValue;

			VM::EpochVariableTypeID DetermineEffectiveType(const VM::ScopeDescription& scope) const;
			bool IsArray() const;
		};

	private:

		//
		// Tracker for code/lexical scope blocks.
		//
		struct BlockEntry
		{
			enum BlockType
			{
				BLOCKENTRYTYPE_GLOBAL,
				BLOCKENTRYTYPE_FREE,
				BLOCKENTRYTYPE_FUNCTION,
				BLOCKENTRYTYPE_FUNCTION_NOCREATE,
				BLOCKENTRYTYPE_DOLOOP,
				BLOCKENTRYTYPE_IF,
				BLOCKENTRYTYPE_ELSE,
				BLOCKENTRYTYPE_ELSEIF,
				BLOCKENTRYTYPE_ELSEIFWRAPPER,
				BLOCKENTRYTYPE_WHILELOOP,
				BLOCKENTRYTYPE_PARALLELFOR,
				BLOCKENTRYTYPE_EXTENSIONCONTROL,
				BLOCKENTRYTYPE_TASK,
				BLOCKENTRYTYPE_THREAD,
				BLOCKENTRYTYPE_MSGDISPATCH,
				BLOCKENTRYTYPE_RESPONSEMAP
			} Type;

			VM::Block* TheBlock;
		};

		//
		// Tracker for higher order functions
		//
		struct FunctionSigStackEntry
		{
			std::wstring Name;
			VM::FunctionSignature Signature;
		};

	// Internal helper routines
	private:
		VM::OperationPtr CreateOperation(const std::wstring& operationname);

		template <class ParamClass>
		void ValidateOperationParameter(unsigned paramindex, VM::EpochVariableTypeID type, const ParamClass& params);

		void RegisterFunctionReturn(VM::EpochVariableTypeID type, const std::wstring& name);
		void MergeFunctionReturns(const FunctionRetMap& functionreturnvalues, VM::Block& block);

		void RegisterExternal(VM::EpochVariableTypeID returntype);

		template<typename LogicalOpType, typename BitwiseOpType>
		VM::OperationPtr ParseLogicalOpArrayOnly();

		template<typename ReturnPointerType, typename LiteralOperatorType, typename LiteralConstType>
		void ParsePotentialArray(bool isarray, ReturnPointerType& retopref);

		template<typename ReturnPointerType, typename LiteralOperatorType, typename LiteralConstType>
		ReturnPointerType* ParseLogicalOp(bool firstisarray, bool secondisarray);

		template<typename ReturnPointerType, typename LiteralOperatorType, typename LiteralConstType>
		ReturnPointerType* ParseBitwiseOp(bool firstisarray, bool secondisarray);

		FileLocationInfo GetFileLocationInfo() const;

		size_t ValidateStructInit(const std::vector<std::wstring>& members, const std::wstring& structtypename, std::vector<VM::Operation*>& ops, size_t maxop, bool& initbyfunctioncall);
		size_t ValidateTupleInit(const std::vector<std::wstring>& members, const std::wstring& tupletypename, std::vector<VM::Operation*>& ops, size_t maxop);
		void ReverseOps(VM::Block* block, size_t numops);
		void ReverseOpsAsGroups(VM::Block* block, size_t numops);

		void AddOperationToCurrentBlock(VM::OperationPtr op);

		void RegisterInfixFunction(const std::wstring& functionname);

	// Internal helpers for builtin functions
	private:
		// Arithmetic
		VM::OperationPtr CreateOperation_Add();
		VM::OperationPtr CreateOperation_Subtract();
		VM::OperationPtr CreateOperation_Multiply();
		VM::OperationPtr CreateOperation_Divide();

		// Bitwise and Logical Operators
		VM::OperationPtr CreateOperation_Or();
		VM::OperationPtr CreateOperation_And();
		VM::OperationPtr CreateOperation_Xor();
		VM::OperationPtr CreateOperation_Not();

		// Comparisons
		VM::OperationPtr CreateOperation_Equal();
		VM::OperationPtr CreateOperation_NotEqual();
		VM::OperationPtr CreateOperation_Less();
		VM::OperationPtr CreateOperation_Greater();
		VM::OperationPtr CreateOperation_LessEqual();
		VM::OperationPtr CreateOperation_GreaterEqual();

		// Concurrency
		VM::OperationPtr CreateOperation_Message();
		VM::OperationPtr CreateOperation_AcceptMessage();
		VM::OperationPtr CreateOperation_Future();

		// Containers
		VM::OperationPtr CreateOperation_ConsArray();
		VM::OperationPtr CreateOperation_ReadArray();
		VM::OperationPtr CreateOperation_WriteArray();

		// Debugging
		VM::OperationPtr CreateOperation_DebugWrite();
		VM::OperationPtr CreateOperation_DebugRead();
		VM::OperationPtr CreateOperation_DebugCrashVM();

		// Flow Control
		VM::OperationPtr CreateOperation_Break();
		VM::OperationPtr CreateOperation_Return();

		// Map and Reduce Functions
		VM::OperationPtr CreateOperation_Map();
		VM::OperationPtr CreateOperation_Reduce();

		// Strings
		VM::OperationPtr CreateOperation_Concat();
		VM::OperationPtr CreateOperation_Length();
		
		// Structures
		VM::OperationPtr CreateOperation_ReadStructure();
		VM::OperationPtr CreateOperation_AssignStructure();
		VM::OperationPtr CreateOperation_Member();

		// Tuples
		VM::OperationPtr CreateOperation_ReadTuple();
		VM::OperationPtr CreateOperation_AssignTuple();

		// Types
		VM::OperationPtr CreateOperation_Cast();

		// Variables
		VM::OperationPtr CreateOperation_Assign();
		VM::OperationPtr CreateOperation_SizeOf();
		

	// Internal state tracking
	private:
		VM::Program* ParsedProgram;

		std::deque<StackEntry> TheStack;
		std::deque<BlockEntry> Blocks;
		std::stack<BlockEntry::BlockType> ExpectedBlockTypes;
		std::stack<VM::EpochVariableTypeID> VariableTypeStack;
		std::stack<std::wstring> VariableNameStack;
		std::stack<IDType> VariableHintStack;
		std::stack<unsigned> PassedParameterCount;
		std::stack<bool> ParamsByRef;
		std::stack<FunctionSigStackEntry> FunctionSignatureStack;
		std::stack<VM::FunctionSignature> HigherOrderFunctionHintStack;
		std::deque<VM::ScopeDescription*> DisplacedScopes;
		std::deque<VM::ResponseMap*> ResponseMapStack;
		std::stack<std::wstring> ExtensionBlockKeywords;
		std::stack<std::wstring> UnknownReturnTypes;
		std::stack<IDType> UnknownReturnTypeHints;

		std::stack<std::list<std::wstring> > InfixOperatorList;
		std::deque<unsigned> InfixOperandCount;

		VM::ScopeDescription* CurrentScope;

		std::wstring ExternalDLLName;
		std::wstring FunctionName;
		unsigned ParamCount;

		std::stack<VM::EpochVariableTypeID> FunctionReturnTypes;
		std::stack<std::wstring> FunctionReturnNames;
		VM::ScopeDescription* FunctionReturns;

		std::map<std::wstring, FunctionRetMap> FunctionReturnValueTracker;
		std::map<std::wstring, VM::Block*> FunctionReturnInitializationBlocks;

		VM::TupleType* CreatedTupleType;
		VM::StructureType* CreatedStructureType;

		std::wstring UpcomingNestedMemberType;

		Byte* CodeBuffer;

		ParsePosIter ParsePosition;

		bool ReadingFunctionSignature;

		VM::Block* GlobalBlock;

		unsigned MemberLevelLValue;
		unsigned MemberLevelRValue;
		unsigned LastMemberLevelRValue;

		bool IsDefiningConstant;

		VM::ScopeDescription* MessageDispatchScope;

		std::vector<std::wstring> SavedStringSlots;

		bool InjectNotOperator;
		bool InjectNegateOperator;

		std::set<std::string> UserInfixOperators;

		std::map<std::string, VM::EpochVariableTypeID> TypeAliases;

		std::wstring OpAssignmentOperator;

		std::list<VM::Operation*> DeferredOperations;
		std::list<VM::Operation*> CachedOperations;

		std::stack<std::wstring> SavedTaskNames;

		std::map<std::wstring, VM::EpochVariableTypeID> ArrayTypes;
		VM::EpochVariableTypeID TempArrayType;

		std::list<std::wstring> MemberAccesses;

		struct TypeAnnotationOp
		{
			VM::Operation* TheOperation;
			size_t Offset;

			TypeAnnotationOp(VM::Operation* op, size_t offset)
				: TheOperation(op), Offset(offset)
			{ }
		};
		std::list<TypeAnnotationOp> TypeAnnotationOps;

		bool FunctionIsInfix;

		bool MessageParamIsArray;

		std::wstring ControlVarName;
		VM::EpochVariableTypeID ControlVarType;

	// Public tracking
	public:
		DebugTable DebugInfo;
	};

}


#include "TemplatedFunctions.inl"


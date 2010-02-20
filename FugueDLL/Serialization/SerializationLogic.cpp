//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Actual implementation of serialization logic
//
// The serialization functions for each operation class are invoked
// by the SerializationTraverser object as it traverses through the
// compiled code tree. We use a templated function to automatically
// generate function overloads that dispatch the serialization call
// to the correct logic in this file. From here we invoke functions
// in the traverser object to actually generate the text stream.
//

#include "pch.h"


//
// Quite a few of the operations we serialize do not actually need
// the full class definition to be present for this logic to work.
// Therefore we use forward declarations of these classes in order
// to minimize header dependencies.
//
namespace VM
{
	namespace Operations
	{
		class Break;
		class DebugCrashVM;
		class DebugReadStaticString;
		class DebugWriteStringExpression;
		class DoWhileLoop;
		class ElseIf;
		class ElseIfWrapper;
		class ExecuteBlock;
		class ExitIfChain;
		class ForkTask;
		class If;
		class MapOperation;
		class NoOp;
		class ReduceOperation;
		class Return;
		class WhileLoop;
		class WhileLoopConditional;
	}
}


// We need headers for all operations which are non-trivial to serialize
#include "Virtual Machine/Operations/Concurrency/FutureOps.h"
#include "Virtual Machine/Operations/Concurrency/Tasks.h"
#include "Virtual Machine/Operations/Concurrency/Messaging.h"
#include "Virtual Machine/Operations/Containers/ContainerOps.h"
#include "Virtual Machine/Operations/Flow/FlowControl.h"
#include "Virtual Machine/Operations/Flow/Invoke.h"
#include "Virtual Machine/Operations/Operators/Arithmetic.h"
#include "Virtual Machine/Operations/Operators/Bitwise.h"
#include "Virtual Machine/Operations/Operators/Comparison.h"
#include "Virtual Machine/Operations/Operators/CompoundOperator.h"
#include "Virtual Machine/Operations/Operators/Logical.h"
#include "Virtual Machine/Operations/Variables/StringOps.h"
#include "Virtual Machine/Operations/Variables/StructureOps.h"
#include "Virtual Machine/Operations/Variables/TupleOps.h"
#include "Virtual Machine/Operations/Variables/VariableOps.h"
#include "Virtual Machine/Operations/StackOps.h"
#include "Virtual Machine/Operations/Typedefs.h"

#include "Virtual Machine/Types Management/TypeInfo.h"
#include "Virtual Machine/Types Management/RuntimeCasts.h"

#include "Marshalling/ExternalDLL.h"

#include "Language Extensions/Handoff.h"

#include "Serialization/SerializationTraverser.h"
#include "Serialization/SerializationTokens.h"


//
// We use this set of macros to cut down on clutter and potential copy/paste
// errors within the serialization code itself. Note that there are still a
// few serialization functions defined by hand at the end of this list; in
// general these functions are used only when the payload data of a particular
// instruction is too complex and/or unique to fit under one of the existing
// operation categories.
//

#define HELPER_GETTOKEN(operationname, token) \
	template <> const std::wstring& Serialization::GetToken<operationname>() { return token; } \


#define SERIALIZE_TOKENONLY(operationname, token) \
	HELPER_GETTOKEN(operationname, token) \
	template <> void Serialization::SerializeNode<operationname>(const operationname& op, SerializationTraverser& traverser) \
	{ traverser.WriteOp(&op, GetToken<operationname>(), true); }

#define SERIALIZE_TOKENWITHTYPE(operationname, token) \
	HELPER_GETTOKEN(operationname, token) \
	template <> void Serialization::SerializeNode<operationname>(const operationname& op, SerializationTraverser& traverser) \
	{ traverser.WriteOp(&op, GetToken<operationname>(), op.GetType()); }

#define SERIALIZE_TOKEN_NOLINEBREAK(operationname, token) \
	HELPER_GETTOKEN(operationname, token) \
	template <> void Serialization::SerializeNode<operationname>(const operationname& op, SerializationTraverser& traverser) \
	{ traverser.WriteOp(&op, GetToken<operationname>(), false); }

#define SERIALIZE_TOKEN_NOADDRESS(operationname, token) \
	HELPER_GETTOKEN(operationname, token) \
	template <> void Serialization::SerializeNode<operationname>(const operationname& op, SerializationTraverser& traverser) \
	{ traverser.WriteOp(GetToken<operationname>()); }

#define SERIALIZE_WITHPAYLOAD(operationname, token) \
	HELPER_GETTOKEN(operationname, token) \
	template <> void Serialization::SerializeNode<operationname>(const operationname& op, SerializationTraverser& traverser) \
	{ traverser.WriteOpWithPayload(&op, GetToken<operationname>()); }

#define SERIALIZE_TYPECAST(operationname, token) \
	HELPER_GETTOKEN(operationname, token) \
	template <> void Serialization::SerializeNode<operationname>(const operationname& op, SerializationTraverser& traverser) \
	{ traverser.WriteCastOp(&op, GetToken<operationname>(), op.GetOriginalType(), op.GetDestinationType()); }

#define SERIALIZE_TYPECAST_TOSTRING(operationname, token) \
	HELPER_GETTOKEN(operationname, token) \
	template <> void Serialization::SerializeNode<operationname>(const operationname& op, SerializationTraverser& traverser) \
	{ traverser.WriteCastOp(&op, GetToken<operationname>(), op.GetOriginalType()); }

#define SERIALIZE_ARITHMETIC(operationname, token) \
	HELPER_GETTOKEN(operationname, token) \
	template <> void Serialization::SerializeNode<operationname>(const operationname& op, SerializationTraverser& traverser) \
	{ traverser.WriteArithmeticOp(&op, GetToken<operationname>(), op.IsFirstArray(), op.IsSecondArray(), op.GetNumParameters()); }

#define SERIALIZE_COMPOUND(operationname, token) \
	HELPER_GETTOKEN(operationname, token) \
	template <> void Serialization::SerializeNode<operationname>(const operationname& op, SerializationTraverser& traverser) \
	{ traverser.WriteCompoundOp(&op, GetToken<operationname>(), op.GetSubOperations().size()); }

#define SERIALIZE_COMPOUNDWITHTYPE(operationname, token) \
	HELPER_GETTOKEN(operationname, token) \
	template <> void Serialization::SerializeNode<operationname>(const operationname& op, SerializationTraverser& traverser) \
	{ traverser.WriteCompoundOp(&op, GetToken<operationname>(), op.GetType(), op.GetSubOperations().size()); }

#define SERIALIZE_MEMBERACCESS(operationname, token) \
	HELPER_GETTOKEN(operationname, token) \
	template <> void Serialization::SerializeNode<operationname>(const operationname& op, SerializationTraverser& traverser) \
	{ traverser.WriteOp(&op, GetToken<operationname>(), op.GetAssociatedIdentifier(), op.GetMemberName()); }

#define SERIALIZE_MEMBERACCESSINDIRECT(operationname, token) \
	HELPER_GETTOKEN(operationname, token) \
	template <> void Serialization::SerializeNode<operationname>(const operationname& op, SerializationTraverser& traverser) \
	{ traverser.WriteOp(&op, GetToken<operationname>(), op.GetMemberName()); }



// Serialization for operations that consist of just an instruction
SERIALIZE_TOKENONLY(VM::Operations::Break, Serialization::Break)
SERIALIZE_TOKENONLY(VM::Operations::CreateThreadPool, Serialization::ThreadPool)
SERIALIZE_TOKENONLY(VM::Operations::DebugCrashVM, Serialization::DebugCrashVM)
SERIALIZE_TOKENONLY(VM::Operations::DebugReadStaticString, Serialization::DebugRead)
SERIALIZE_TOKENONLY(VM::Operations::DebugWriteStringExpression, Serialization::DebugWrite)
SERIALIZE_TOKENONLY(VM::Operations::DoWhileLoop, Serialization::DoWhile)
SERIALIZE_TOKENONLY(VM::Operations::ExitIfChain, Serialization::ExitIfChain)
SERIALIZE_TOKENONLY(VM::Operations::ForkTask, Serialization::ForkTask)
SERIALIZE_TOKENONLY(VM::Operations::ForkThread, Serialization::ForkThread)
SERIALIZE_TOKENONLY(VM::Operations::GetMessageSender, Serialization::GetMessageSender)
SERIALIZE_TOKENONLY(VM::Operations::GetTaskCaller, Serialization::GetTaskCaller)
SERIALIZE_TOKENONLY(VM::Operations::If, Serialization::If)
SERIALIZE_TOKENONLY(VM::Operations::LogicalNot, Serialization::LogicalNot)
SERIALIZE_TOKENONLY(VM::Operations::LogicalXor, Serialization::LogicalXor)
SERIALIZE_TOKENONLY(VM::Operations::MapOperation, Serialization::Map)
SERIALIZE_TOKENONLY(VM::Operations::Negate, Serialization::Negate)
SERIALIZE_TOKENONLY(VM::Operations::NoOp, Serialization::NoOp)
SERIALIZE_TOKENONLY(VM::Operations::ReduceOperation, Serialization::Reduce)
SERIALIZE_TOKENONLY(VM::Operations::Return, Serialization::Return)
SERIALIZE_TOKENONLY(VM::Operations::WhileLoop, Serialization::While)
SERIALIZE_TOKENONLY(VM::Operations::WhileLoopConditional, Serialization::WhileCondition)


// Special templated operations
SERIALIZE_TYPECAST(VM::Operations::TypeCastStringToInteger, Serialization::TypeCast)
SERIALIZE_TYPECAST(VM::Operations::TypeCastRealToInteger, Serialization::TypeCast)
SERIALIZE_TYPECAST(VM::Operations::TypeCastInteger16ToInteger, Serialization::TypeCast)
SERIALIZE_TYPECAST(VM::Operations::TypeCastBooleanToInteger, Serialization::TypeCast)

SERIALIZE_TYPECAST(VM::Operations::TypeCastStringToInteger16, Serialization::TypeCast)
SERIALIZE_TYPECAST(VM::Operations::TypeCastRealToInteger16, Serialization::TypeCast)
SERIALIZE_TYPECAST(VM::Operations::TypeCastIntegerToInteger16, Serialization::TypeCast)
SERIALIZE_TYPECAST(VM::Operations::TypeCastBooleanToInteger16, Serialization::TypeCast)

SERIALIZE_TYPECAST(VM::Operations::TypeCastStringToReal, Serialization::TypeCast)
SERIALIZE_TYPECAST(VM::Operations::TypeCastInteger16ToReal, Serialization::TypeCast)
SERIALIZE_TYPECAST(VM::Operations::TypeCastIntegerToReal, Serialization::TypeCast)
SERIALIZE_TYPECAST(VM::Operations::TypeCastBooleanToReal, Serialization::TypeCast)

SERIALIZE_TYPECAST_TOSTRING(VM::Operations::TypeCastRealToString, Serialization::TypeCastToString)
SERIALIZE_TYPECAST_TOSTRING(VM::Operations::TypeCastInteger16ToString, Serialization::TypeCastToString)
SERIALIZE_TYPECAST_TOSTRING(VM::Operations::TypeCastIntegerToString, Serialization::TypeCastToString)
SERIALIZE_TYPECAST_TOSTRING(VM::Operations::TypeCastBooleanToString, Serialization::TypeCastToString)
SERIALIZE_TYPECAST_TOSTRING(VM::Operations::TypeCastBufferToString, Serialization::TypeCastToString)



// Operations that do not need a trailing newline
SERIALIZE_TOKEN_NOLINEBREAK(VM::Operations::PushOperation, Serialization::PushOperation)


// Operations that do not need to record their address
SERIALIZE_TOKEN_NOADDRESS(VM::Operations::ElseIf, Serialization::ElseIf)
SERIALIZE_TOKEN_NOADDRESS(VM::Operations::ElseIfWrapper, Serialization::ElseIfWrapper)


// Operations that must record associated type data
SERIALIZE_TOKENWITHTYPE(VM::Operations::BitwiseNot, Serialization::BitwiseNot)
SERIALIZE_TOKENWITHTYPE(VM::Operations::BitwiseXor, Serialization::BitwiseXor)


// Operations with payloads that must be serialized
SERIALIZE_WITHPAYLOAD(VM::Operations::AcceptMessageFromResponseMap, Serialization::AcceptMessageFromMap)
SERIALIZE_WITHPAYLOAD(VM::Operations::AssignValue, Serialization::AssignValue)
SERIALIZE_WITHPAYLOAD(VM::Operations::BindFunctionReference, Serialization::BindFunctionReference)
SERIALIZE_WITHPAYLOAD(VM::Operations::BindReference, Serialization::BindReference)
SERIALIZE_WITHPAYLOAD(VM::Operations::BooleanConstant, Serialization::BooleanConstant)
SERIALIZE_WITHPAYLOAD(VM::Operations::GetVariableValue, Serialization::GetValue)
SERIALIZE_WITHPAYLOAD(VM::Operations::InitializeValue, Serialization::InitializeValue)
SERIALIZE_WITHPAYLOAD(VM::Operations::IntegerConstant, Serialization::IntegerConstant)
SERIALIZE_WITHPAYLOAD(VM::Operations::Integer16Constant, Serialization::Integer16Constant)
SERIALIZE_WITHPAYLOAD(VM::Operations::Invoke, Serialization::Invoke)
SERIALIZE_WITHPAYLOAD(VM::Operations::InvokeIndirect, Serialization::InvokeIndirect)
SERIALIZE_WITHPAYLOAD(VM::Operations::IsEqual, Serialization::IsEqual)
SERIALIZE_WITHPAYLOAD(VM::Operations::IsGreater, Serialization::IsGreater)
SERIALIZE_WITHPAYLOAD(VM::Operations::IsGreaterOrEqual, Serialization::IsGreaterEqual)
SERIALIZE_WITHPAYLOAD(VM::Operations::IsLesser, Serialization::IsLesser)
SERIALIZE_WITHPAYLOAD(VM::Operations::IsLesserOrEqual, Serialization::IsLesserEqual)
SERIALIZE_WITHPAYLOAD(VM::Operations::IsNotEqual, Serialization::IsNotEqual)
SERIALIZE_WITHPAYLOAD(VM::Operations::Length, Serialization::Length)
SERIALIZE_WITHPAYLOAD(VM::Operations::PushBooleanLiteral, Serialization::PushBooleanLiteral)
SERIALIZE_WITHPAYLOAD(VM::Operations::PushIntegerLiteral, Serialization::PushIntegerLiteral)
SERIALIZE_WITHPAYLOAD(VM::Operations::PushInteger16Literal, Serialization::PushInteger16Literal)
SERIALIZE_WITHPAYLOAD(VM::Operations::PushRealLiteral, Serialization::PushRealLiteral)
SERIALIZE_WITHPAYLOAD(VM::Operations::PushStringLiteral, Serialization::PushStringLiteral)
SERIALIZE_WITHPAYLOAD(VM::Operations::RealConstant, Serialization::RealConstant)
SERIALIZE_WITHPAYLOAD(VM::Operations::SizeOf, Serialization::SizeOf)


// Operations with compound payloads
SERIALIZE_ARITHMETIC(VM::Operations::Concatenate, Serialization::Concat)
SERIALIZE_ARITHMETIC(VM::Operations::DivideInteger16s, Serialization::DivideInteger16s)
SERIALIZE_ARITHMETIC(VM::Operations::DivideIntegers, Serialization::DivideIntegers)
SERIALIZE_ARITHMETIC(VM::Operations::DivideReals, Serialization::DivideReals)
SERIALIZE_ARITHMETIC(VM::Operations::MultiplyInteger16s, Serialization::MultiplyInteger16s)
SERIALIZE_ARITHMETIC(VM::Operations::MultiplyIntegers, Serialization::MultiplyIntegers)
SERIALIZE_ARITHMETIC(VM::Operations::MultiplyReals, Serialization::MultiplyReals)
SERIALIZE_ARITHMETIC(VM::Operations::SubtractInteger16s, Serialization::SubtractInteger16s)
SERIALIZE_ARITHMETIC(VM::Operations::SubtractIntegers, Serialization::SubtractIntegers)
SERIALIZE_ARITHMETIC(VM::Operations::SubtractReals, Serialization::SubtractReals)
SERIALIZE_ARITHMETIC(VM::Operations::SumInteger16s, Serialization::AddInteger16s)
SERIALIZE_ARITHMETIC(VM::Operations::SumIntegers, Serialization::AddIntegers)
SERIALIZE_ARITHMETIC(VM::Operations::SumReals, Serialization::AddReals)

SERIALIZE_COMPOUNDWITHTYPE(VM::Operations::BitwiseAnd, Serialization::BitwiseAnd)
SERIALIZE_COMPOUNDWITHTYPE(VM::Operations::BitwiseOr, Serialization::BitwiseOr)
SERIALIZE_COMPOUND(VM::Operations::LogicalAnd, Serialization::LogicalAnd)
SERIALIZE_COMPOUND(VM::Operations::LogicalOr, Serialization::LogicalOr)

SERIALIZE_MEMBERACCESS(VM::Operations::ReadStructure, Serialization::ReadStructure)
SERIALIZE_MEMBERACCESS(VM::Operations::AssignStructure, Serialization::AssignStructure)
SERIALIZE_MEMBERACCESS(VM::Operations::ReadTuple, Serialization::ReadTuple)
SERIALIZE_MEMBERACCESS(VM::Operations::AssignTuple, Serialization::AssignTuple)

SERIALIZE_MEMBERACCESSINDIRECT(VM::Operations::AssignStructureIndirect, Serialization::AssignStructureIndirect)
SERIALIZE_MEMBERACCESSINDIRECT(VM::Operations::ReadStructureIndirect, Serialization::ReadStructureIndirect)


// Additional special serialization handling
template <> const std::wstring& Serialization::GetToken<Marshalling::CallDLL>() { return Serialization::CallDLL; }
template <> void Serialization::SerializeNode<Marshalling::CallDLL>(const Marshalling::CallDLL& op, SerializationTraverser& traverser)
{ traverser.WriteOp(&op, GetToken<Marshalling::CallDLL>(), op.GetDLLName(), op.GetFunctionName(), op.GetReturnType()); }

template <> const std::wstring& Serialization::GetToken<VM::Function>() { throw Exception("Function wrapper object is not serialized directly"); }
template <> void Serialization::SerializeNode<VM::Function>(const VM::Function& func, SerializationTraverser& traverser) { throw Exception("Function wrapper object is not serialized directly"); }

template <> const std::wstring& Serialization::GetToken<VM::Operations::ForkFuture>() { return Serialization::ForkFuture; }
template <> void Serialization::SerializeNode<VM::Operations::ForkFuture>(const VM::Operations::ForkFuture& op, SerializationTraverser& traverser)
{ traverser.WriteForkFuture(&op, GetToken<VM::Operations::ForkFuture>(), op.GetVarName(), op.GetType(), op.UsesThreadPool()); }

template <> const std::wstring& Serialization::GetToken<VM::Operations::SendTaskMessage>() { return Serialization::SendTaskMessage; }
template <> void Serialization::SerializeNode<VM::Operations::SendTaskMessage>(const VM::Operations::SendTaskMessage& op, SerializationTraverser& traverser)
{ traverser.WriteSendMessage(&op, GetToken<VM::Operations::SendTaskMessage>(), op.DoesUseTaskID(), op.GetMessageName(), op.GetPayloadTypes()); }

template <> const std::wstring& Serialization::GetToken<VM::Operations::AcceptMessage>() { return Serialization::AcceptMessage; }
template <> void Serialization::SerializeNode<VM::Operations::AcceptMessage>(const VM::Operations::AcceptMessage& op, SerializationTraverser& traverser)
{ traverser.WriteAcceptMessage(&op, GetToken<VM::Operations::AcceptMessage>(), op.GetMessageName(), op.GetPayloadTypes()); }

template <> const std::wstring& Serialization::GetToken<VM::Operations::ConsArray>() { return Serialization::ConsArray; }
template <> void Serialization::SerializeNode<VM::Operations::ConsArray>(const VM::Operations::ConsArray& op, SerializationTraverser& traverser)
{ traverser.WriteConsArray(&op, GetToken<VM::Operations::ConsArray>(), op.GetElementType(), op.GetNumEntries()); }

template <> const std::wstring& Serialization::GetToken<VM::Operations::BindStructMemberReference>() { return Serialization::BindStructMemberReference; }
template <> void Serialization::SerializeNode<VM::Operations::BindStructMemberReference>(const VM::Operations::BindStructMemberReference& op, SerializationTraverser& traverser)
{ traverser.WriteChainedOp(&op, GetToken<VM::Operations::BindStructMemberReference>(), op.IsChained(), (op.IsChained() ? Serialization::EmptyString : op.GetAssociatedIdentifier()), op.GetMemberName()); }

template <> const std::wstring& Serialization::GetToken<VM::Operations::ExecuteBlock>() { return Serialization::EmptyString; }
template <> void Serialization::SerializeNode<VM::Operations::ExecuteBlock>(const VM::Operations::ExecuteBlock& op, SerializationTraverser& traverser)
{ }

template <> const std::wstring& Serialization::GetToken<Extensions::HandoffOperation>() { return Serialization::Handoff; }
template <> void Serialization::SerializeNode<Extensions::HandoffOperation>(const Extensions::HandoffOperation& op, SerializationTraverser& traverser)
{ traverser.WriteHandoffOp(&op, GetToken<Extensions::HandoffOperation>(), op.GetExtensionName()); }

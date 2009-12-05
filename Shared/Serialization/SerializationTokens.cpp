//
// The Epoch Language Project
// Shared Library Code
//
// Constants for serializing code to Epoch Assembly format
//

#include "pch.h"
#include "Serialization/SerializationTokens.h"


//-------------------------------------------------------------------------------
// Constants
//-------------------------------------------------------------------------------

std::wstring Serialization::EmptyString(L"");

std::wstring Serialization::NoOp(L"NOOP");

std::wstring Serialization::Invoke(L"INVOKE");
std::wstring Serialization::InvokeIndirect(L"INVOKEINDIRECT");

std::wstring Serialization::Scope(L"SCOPE");
std::wstring Serialization::EndScope(L"ENDSCOPE");
std::wstring Serialization::Variables(L"VARS");
std::wstring Serialization::Functions(L"FUNCTIONS");
std::wstring Serialization::Ghosts(L"GHOSTS");
std::wstring Serialization::GhostRecord(L"GHOSTREC");
std::wstring Serialization::ParentScope(L"PARENT");
std::wstring Serialization::TupleTypes(L"TUPLETYPES");
std::wstring Serialization::TupleTypeHints(L"TUPLEHINTS");
std::wstring Serialization::TupleTypeMap(L"TUPLETYPEDATA");
std::wstring Serialization::StructureTypes(L"STRUCTURETYPES");
std::wstring Serialization::StructureTypeHints(L"STRUCTUREHINTS");
std::wstring Serialization::StructureTypeMap(L"STRUCTURETYPEDATA");
std::wstring Serialization::FunctionSignatureList(L"FUNCSIGNATURES");
std::wstring Serialization::FunctionSignatureBegin(L"BEGINSIGNATURE");
std::wstring Serialization::FunctionSignatureEnd(L"ENDSIGNATURE");
std::wstring Serialization::Reference(L"REF");
std::wstring Serialization::ResponseMaps(L"RESPONSEMAPS");

std::wstring Serialization::PushIntegerLiteral(L"PUSH_INT");
std::wstring Serialization::PushInteger16Literal(L"PUSH_INT16");
std::wstring Serialization::PushRealLiteral(L"PUSH_REAL");
std::wstring Serialization::PushStringLiteral(L"PUSH_STR");
std::wstring Serialization::PushBooleanLiteral(L"PUSH_BOOL");
std::wstring Serialization::PushOperation(L"PUSH");
std::wstring Serialization::BindReference(L"BINDREF");
std::wstring Serialization::BindFunctionReference(L"BINDFUNC");

std::wstring Serialization::If(L"IF");
std::wstring Serialization::ElseIfWrapper(L"ELSEIFWRAP");
std::wstring Serialization::ElseIf(L"ELSEIF");
std::wstring Serialization::ExitIfChain(L"EXIT_IF");
std::wstring Serialization::DoWhile(L"DOWHILE");
std::wstring Serialization::While(L"WHILE");
std::wstring Serialization::WhileCondition(L"CONDITIONAL_BREAK");
std::wstring Serialization::Break(L"BREAK");
std::wstring Serialization::Return(L"RET");

std::wstring Serialization::AssignValue(L"WRITE");
std::wstring Serialization::InitializeValue(L"INIT");
std::wstring Serialization::GetValue(L"READ");
std::wstring Serialization::SizeOf(L"SIZEOF");
std::wstring Serialization::Length(L"LENGTH");
std::wstring Serialization::IntegerConstant(L"INT");
std::wstring Serialization::Integer16Constant(L"INT16");
std::wstring Serialization::RealConstant(L"REAL");
std::wstring Serialization::BooleanConstant(L"BOOL");

std::wstring Serialization::ReadTuple(L"READTUPLE");
std::wstring Serialization::AssignTuple(L"WRITETUPLE");

std::wstring Serialization::ReadStructure(L"READSTRUCT");
std::wstring Serialization::ReadStructureIndirect(L"READSTRUCTINDIRECT");
std::wstring Serialization::AssignStructure(L"WRITESTRUCT");
std::wstring Serialization::AssignStructureIndirect(L"WRITESTRUCTINDIRECT");
std::wstring Serialization::BindStructMemberReference(L"BINDSTRUCT");

std::wstring Serialization::ConsList(L"CONSLIST");
std::wstring Serialization::Map(L"MAP");
std::wstring Serialization::Reduce(L"REDUCE");

std::wstring Serialization::AcceptMessage(L"ACCEPTMSG");
std::wstring Serialization::AcceptMessageFromMap(L"ACCEPTMSGMAP");
std::wstring Serialization::SendTaskMessage(L"SENDMSG");
std::wstring Serialization::GetTaskCaller(L"GETCALLER");
std::wstring Serialization::GetMessageSender(L"GETSENDER");

std::wstring Serialization::AddIntegers(L"ADD_INT");
std::wstring Serialization::AddInteger16s(L"ADD_INT16");
std::wstring Serialization::AddReals(L"ADD_REAL");
std::wstring Serialization::SubtractIntegers(L"SUB_INT");
std::wstring Serialization::SubtractInteger16s(L"SUB_INT16");
std::wstring Serialization::SubtractReals(L"SUB_REAL");
std::wstring Serialization::MultiplyIntegers(L"MULT_INT");
std::wstring Serialization::MultiplyInteger16s(L"MULT_INT16");
std::wstring Serialization::MultiplyReals(L"MULT_REAL");
std::wstring Serialization::DivideIntegers(L"DIV_INT");
std::wstring Serialization::DivideInteger16s(L"DIV_INT16");
std::wstring Serialization::DivideReals(L"DIV_REAL");
std::wstring Serialization::Negate(L"NEG");

std::wstring Serialization::IsEqual(L"EQ");
std::wstring Serialization::IsNotEqual(L"NEQ");
std::wstring Serialization::IsGreater(L"GT");
std::wstring Serialization::IsGreaterEqual(L"GTE");
std::wstring Serialization::IsLesser(L"LT");
std::wstring Serialization::IsLesserEqual(L"LTE");

std::wstring Serialization::BitwiseOr(L"BOR");
std::wstring Serialization::BitwiseAnd(L"BAND");
std::wstring Serialization::BitwiseXor(L"BXOR");
std::wstring Serialization::BitwiseNot(L"BNOT");

std::wstring Serialization::LogicalOr(L"LOR");
std::wstring Serialization::LogicalAnd(L"LAND");
std::wstring Serialization::LogicalXor(L"LXOR");
std::wstring Serialization::LogicalNot(L"LNOT");

std::wstring Serialization::Concat(L"CONCAT");

std::wstring Serialization::TypeCast(L"CAST");
std::wstring Serialization::TypeCastToString(L"CASTSTR");

std::wstring Serialization::ForkTask(L"FORK");
std::wstring Serialization::ForkFuture(L"FUTURE");

std::wstring Serialization::DebugWrite(L"DEBUG_WRITE");
std::wstring Serialization::DebugRead(L"DEBUG_READ");
std::wstring Serialization::DebugCrashVM(L"DEBUG_CRASH_VM");

std::wstring Serialization::CallDLL(L"EXTERNAL");

std::wstring Serialization::Handoff(L"HANDOFF");

std::wstring Serialization::Null(L"NULL");
std::wstring Serialization::BeginBlock(L"BLOCK");
std::wstring Serialization::EndBlock(L"ENDBLOCK");
std::wstring Serialization::True(L"TRUE");
std::wstring Serialization::False(L"FALSE");
std::wstring Serialization::StaticStrings(L"STATIC_STR_POOL");
std::wstring Serialization::StringVars(L"VAR_STR_POOL");
std::wstring Serialization::Members(L"MEMBERS");
std::wstring Serialization::IDCounter(L"IDCOUNTER");
std::wstring Serialization::TupleStaticData(L"TUPLEINFO");
std::wstring Serialization::StructureStaticData(L"STRUCTUREINFO");
std::wstring Serialization::GlobalBlock(L"GLOBAL");
std::wstring Serialization::Constants(L"CONSTANTS");
std::wstring Serialization::Futures(L"FUTURES");
std::wstring Serialization::ListTypes(L"LISTTYPES");
std::wstring Serialization::ListSizes(L"LISTSIZES");


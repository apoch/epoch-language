//
// The Epoch Language Project
// FUGUE Bytecode assembler/disassembler
//
// Constant definitions for mapping Epoch Assembler into bytecode
//

#include "pch.h"
#include "Bytecode.h"

const char* Bytecode::HeaderCookie = "EPOCH";

const unsigned char Bytecode::NullFlag					= 0x00;
const unsigned char Bytecode::ParentScope				= 0x01;
const unsigned char Bytecode::ThreadPool				= 0x02;
const unsigned char Bytecode::Variables					= 0x03;
const unsigned char Bytecode::Ghosts					= 0x04;
const unsigned char Bytecode::Functions					= 0x05;
const unsigned char Bytecode::CallDLL					= 0x06;
const unsigned char Bytecode::Reference					= 0x07;
const unsigned char Bytecode::StructureHints			= 0x08;
const unsigned char Bytecode::BeginBlock				= 0x09;
const unsigned char Bytecode::EndBlock					= 0x0a;
const unsigned char Bytecode::Invoke					= 0x0b;
const unsigned char Bytecode::PushOperation				= 0x0c;
const unsigned char Bytecode::PushIntegerLiteral		= 0x0d;
const unsigned char Bytecode::PushStringLiteral			= 0x0e;
const unsigned char Bytecode::AssignValue				= 0x0f;
const unsigned char Bytecode::GetValue					= 0x10;
const unsigned char Bytecode::BindFunctionReference		= 0x11;
const unsigned char Bytecode::SizeOf					= 0x12;
const unsigned char Bytecode::IsNotEqual				= 0x13;
const unsigned char Bytecode::If						= 0x14;
const unsigned char Bytecode::While						= 0x15;
const unsigned char Bytecode::BindReference				= 0x16;
const unsigned char Bytecode::WhileCondition			= 0x17;
const unsigned char Bytecode::NoOp						= 0x18;
const unsigned char Bytecode::Scope						= 0x19;
const unsigned char Bytecode::GhostRecord				= 0x1a;
const unsigned char Bytecode::IsEqual					= 0x1b;
const unsigned char Bytecode::ElseIfWrapper				= 0x1c;
const unsigned char Bytecode::ElseIf					= 0x1d;
const unsigned char Bytecode::ExitIfChain				= 0x1e;
const unsigned char Bytecode::Members					= 0x1f;
const unsigned char Bytecode::StaticStrings				= 0x20;
const unsigned char Bytecode::StringVars				= 0x21;
const unsigned char Bytecode::TupleStaticData			= 0x22;
const unsigned char Bytecode::IDCounter					= 0x23;
const unsigned char Bytecode::StructureStaticData		= 0x24;
const unsigned char Bytecode::TupleTypes				= 0x25;
const unsigned char Bytecode::TupleHints				= 0x26;
const unsigned char Bytecode::StructureTypes			= 0x27;
const unsigned char Bytecode::StructureTypeMap			= 0x28;
const unsigned char Bytecode::TupleTypeMap				= 0x29;
const unsigned char Bytecode::EndScope					= 0x2a;
const unsigned char Bytecode::TypeCast					= 0x2b;
const unsigned char Bytecode::DebugWrite				= 0x2c;
const unsigned char Bytecode::PushRealLiteral			= 0x2d;
const unsigned char Bytecode::DoWhile					= 0x2e;
const unsigned char Bytecode::DivideReals				= 0x2f;
const unsigned char Bytecode::AddReals					= 0x30;
const unsigned char Bytecode::SubReals					= 0x31;
const unsigned char Bytecode::IsLesser					= 0x32;
const unsigned char Bytecode::PushBooleanLiteral		= 0x33;
const unsigned char Bytecode::AddIntegers				= 0x34;
const unsigned char Bytecode::SubtractIntegers			= 0x35;
const unsigned char Bytecode::IsGreater					= 0x36;
const unsigned char Bytecode::DebugRead					= 0x37;
const unsigned char Bytecode::TypeCastToString			= 0x38;
const unsigned char Bytecode::ReadTuple					= 0x39;
const unsigned char Bytecode::WriteTuple				= 0x3a;
const unsigned char Bytecode::ReadStructure				= 0x3b;
const unsigned char Bytecode::WriteStructure			= 0x3c;
const unsigned char Bytecode::GlobalBlock				= 0x3d;
const unsigned char Bytecode::Init						= 0x3e;
const unsigned char Bytecode::PushInteger16Literal		= 0x3f;
const unsigned char Bytecode::DivideIntegers			= 0x40;
const unsigned char Bytecode::Concat					= 0x41;
const unsigned char Bytecode::IsGreaterEqual			= 0x42;
const unsigned char Bytecode::IsLesserEqual				= 0x43;
const unsigned char Bytecode::DivideInteger16s			= 0x44;
const unsigned char Bytecode::BitwiseOr					= 0x45;
const unsigned char Bytecode::BitwiseAnd				= 0x46;
const unsigned char Bytecode::BitwiseXor				= 0x47;
const unsigned char Bytecode::BitwiseNot				= 0x48;
const unsigned char Bytecode::LogicalOr					= 0x49;
const unsigned char Bytecode::LogicalAnd				= 0x4a;
const unsigned char Bytecode::LogicalXor				= 0x4b;
const unsigned char Bytecode::LogicalNot				= 0x4c;
const unsigned char Bytecode::InvokeIndirect			= 0x4d;
const unsigned char Bytecode::Break						= 0x4e;
const unsigned char Bytecode::Return					= 0x4f;
const unsigned char Bytecode::BooleanLiteral			= 0x50;
const unsigned char Bytecode::Futures					= 0x51;
const unsigned char Bytecode::ReadStructureIndirect		= 0x52;
const unsigned char Bytecode::BindStruct				= 0x53;
const unsigned char Bytecode::WriteStructureIndirect	= 0x54;
const unsigned char Bytecode::Constants					= 0x55;
const unsigned char Bytecode::FunctionSignatureList		= 0x56;
const unsigned char Bytecode::FunctionSignatureBegin	= 0x57;
const unsigned char Bytecode::FunctionSignatureEnd		= 0x58;
const unsigned char Bytecode::ForkTask					= 0x59;
const unsigned char Bytecode::ResponseMaps				= 0x5a;
const unsigned char Bytecode::AcceptMessage				= 0x5b;
const unsigned char Bytecode::MultiplyIntegers			= 0x5c;
const unsigned char Bytecode::GetMessageSender			= 0x5d;
const unsigned char Bytecode::GetTaskCaller				= 0x5e;
const unsigned char Bytecode::SendTaskMessage			= 0x6f;
const unsigned char Bytecode::AcceptMessageFromMap		= 0x60;
const unsigned char Bytecode::DebugCrashVM				= 0x61;
const unsigned char Bytecode::Future					= 0x62;
const unsigned char Bytecode::Map						= 0x63;
const unsigned char Bytecode::Reduce					= 0x64;
const unsigned char Bytecode::IntegerLiteral			= 0x65;
const unsigned char Bytecode::ForkThread				= 0x66;
const unsigned char Bytecode::Handoff					= 0x67;


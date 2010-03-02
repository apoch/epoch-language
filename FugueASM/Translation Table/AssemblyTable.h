//
// The Epoch Language Project
// FUGUE Bytecode assembler/disassembler
//
// Table used for assembling/disassembling instructions
//
// This table cannot be compiled directly; it is meant to be included
// by Assembler.cpp and Disassembler.cpp. Both modules set up some
// macros beforehand which control how the entries in this table are
// handled.
//

#pragma once


// Dependencies
#include "Bytecode/Bytecode.h"
#include "Serialization/SerializationTokens.h"
#include "Utility/Types/EpochTypeIDs.h"



//-------------------------------------------------------------------------------
// Commonly used instruction sequences
//-------------------------------------------------------------------------------

#define PARAM_UINT(paramname)																				\
	SPACE																									\
	COPY_UINT(paramname)																					\
	NEWLINE																									\

#define PARAM_HEX(paramname)																				\
	SPACE																									\
	COPY_HEX(paramname)																						\
	NEWLINE																									\

#define PARAM_STR(paramname)																				\
	SPACE																									\
	COPY_STR(paramname)																						\
	NEWLINE																									\

#define PARAM_BOOL(paramname)																				\
	SPACE																									\
	COPY_BOOL(paramname)																					\
	NEWLINE																									\

#define PARAM_REAL(paramname)																				\
	SPACE																									\
	COPY_REAL(paramname)																					\
	NEWLINE																									\


//-------------------------------------------------------------------------------
// Assembly table macro - generates the actual parser/serializer code
//-------------------------------------------------------------------------------


#define ASSEMBLYTABLE																						\
																											\
DEFINE_INSTRUCTION(Bytecode::NoOp, Serialization::Null)														\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::ParentScope, Serialization::ParentScope)										\
	PARAM_HEX(scopeid)																						\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::Variables, Serialization::Variables)											\
	PARAM_UINT(count)																						\
	LOOP(count)																								\
		IF_ASSEMBLING																						\
			READ_STRING(isref)																				\
			IF_STR_MATCHES(isref, Bytecode::Reference, Serialization::Reference)							\
				WRITE(Bytecode::Reference, Serialization::Reference)										\
				COPY_STR(varname)																			\
			ELSE																							\
				WRITE(Bytecode::NullFlag, Serialization::EmptyString)										\
				WRITE_STRING(isref)																			\
			END_IF																							\
		ELSE																								\
			READ_INSTRUCTION(isref)																			\
			IF_INSTRUCTION_MATCHES(isref, Bytecode::Reference, Serialization::Reference)					\
				WRITE_STRING(Serialization::Reference)														\
				SPACE																						\
			END_IF																							\
			COPY_STR(varname)																				\
			SPACE																							\
		END_IF																								\
		COPY_UINT(type)																						\
		NEWLINE																								\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::Ghosts, Serialization::Ghosts)													\
	PARAM_UINT(count)																						\
	LOOP(count)																								\
		EXPECT(Bytecode::GhostRecord, Serialization::GhostRecord)											\
		PARAM_UINT(reccount)																				\
		LOOP(reccount)																						\
			COPY_STR(varname)																				\
			PARAM_HEX(ownerscope)																			\
		ENDLOOP																								\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::Functions, Serialization::Functions)											\
	PARAM_UINT(count)																						\
	LOOP(count)																								\
		COPY_STR(funcname)																					\
		PARAM_HEX(addr1)																					\
		READ_HEX(addr2)																						\
		IF_UINT_MATCHES(addr1, addr2)																		\
			WRITE_HEX(addr2)																				\
			SPACE																							\
			EXPECT(Bytecode::CallDLL, Serialization::CallDLL)												\
			SPACE																							\
			COPY_STR(dllname)																				\
			SPACE																							\
			COPY_STR(funcname)																				\
			PARAM_UINT(functype)																			\
			PARAM_UINT(functypehint)																		\
			IF_ASSEMBLING																					\
				READ_HEX(scopeid)																			\
				EXPECT(Bytecode::Scope, Serialization::Scope)												\
				WRITE_HEX(scopeid)																			\
			ELSE																							\
				READ_INSTRUCTION(ignored)																	\
				COPY_HEX(scopeid)																			\
				SPACE																						\
				WRITE_STRING(Serialization::Scope)															\
				NEWLINE																						\
			END_IF																							\
			RECURSE																							\
		ELSE																								\
			WRITE_HEX(addr2)																				\
			SPACE																							\
			EXPECT(Bytecode::Scope, Serialization::Scope)													\
			NEWLINE																							\
			IF_ASSEMBLING																					\
				WRITE_HEX(addr2)																			\
				RECURSE																						\
				READ_HEX(scopeid)																			\
				EXPECT(Bytecode::Scope, Serialization::Scope)												\
				WRITE_HEX(scopeid)																			\
				RECURSE																						\
				EXPECT(Bytecode::BeginBlock, Serialization::BeginBlock)										\
				READ_HEX(innerscopeid)																		\
				EXPECT(Bytecode::Scope, Serialization::Scope)												\
				WRITE_HEX(innerscopeid)																		\
				RECURSE																						\
				RECURSE																						\
			ELSE																							\
				READ_HEX(scopeid)																			\
				RECURSE																						\
				READ_INSTRUCTION(ignored)																	\
				COPY_HEX(secondscopeid)																		\
				SPACE																						\
				WRITE_STRING(Serialization::Scope)															\
				NEWLINE																						\
				RECURSE																						\
				EXPECT(Bytecode::BeginBlock, Serialization::BeginBlock)										\
				NEWLINE																						\
				READ_INSTRUCTION(instr)																		\
				COPY_HEX(scopeid2)																			\
				SPACE																						\
				WRITE_STRING(Serialization::Scope)															\
				NEWLINE																						\
				RECURSE																						\
				RECURSE																						\
			END_IF																							\
		END_IF																								\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::CallDLL, Serialization::CallDLL)												\
	EXCEPTION("Wrong context for this instruction")															\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::Reference, Serialization::Reference)											\
	EXCEPTION("Wrong context for this instruction")															\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::StructureHints, Serialization::StructureTypeHints)								\
	PARAM_UINT(count)																						\
	LOOP(count)																								\
		COPY_STR(varname)																					\
		SPACE																								\
		PARAM_UINT(vartype)																					\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::BeginBlock, Serialization::BeginBlock)											\
	NEWLINE																									\
	IF_ASSEMBLING																							\
		READ_HEX(scopeid)																					\
		EXPECT(Bytecode::Scope, Serialization::Scope)														\
		WRITE_HEX(scopeid)																					\
	ELSE																									\
		READ_INSTRUCTION(ignored)																			\
		COPY_HEX(scopeid)																					\
		SPACE																								\
		WRITE_STRING(Serialization::Scope)																	\
		NEWLINE																								\
	END_IF																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::EndBlock, Serialization::EndBlock)												\
	NEWLINE																									\
	RETURN																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::Invoke, Serialization::Invoke)										\
	PARAM_HEX(funcid)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::PushOperation, Serialization::PushOperation)							\
	SPACE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::PushIntegerLiteral, Serialization::PushIntegerLiteral) 				\
	PARAM_UINT(literalvalue)																				\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::PushStringLiteral, Serialization::PushStringLiteral)					\
	SPACE																									\
	COPY_UINT(length)																						\
	SKIPTOSTRING(length)																					\
	SPACE																									\
	COPY_RAW(length)																						\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::AssignValue, Serialization::AssignValue)								\
	PARAM_STR(varname)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::GetValue, Serialization::GetValue)									\
	PARAM_STR(varname)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::BindFunctionReference, Serialization::BindFunctionReference)			\
	PARAM_STR(funcname)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::SizeOf, Serialization::SizeOf)										\
	PARAM_STR(varname)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::IsNotEqual, Serialization::IsNotEqual)								\
	PARAM_UINT(type)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::If, Serialization::If)												\
	NEWLINE																									\
	EXPECT(Bytecode::BeginBlock, Serialization::BeginBlock)													\
	IF_ASSEMBLING																							\
		READ_HEX(innerscopeid)																				\
		EXPECT(Bytecode::Scope, Serialization::Scope)														\
		WRITE_NUMBER(innerscopeid)																			\
	ELSE																									\
		NEWLINE																								\
		READ_INSTRUCTION(ignored)																			\
		COPY_HEX(scopeid)																					\
		SPACE																								\
		WRITE_STRING(Serialization::Scope)																	\
		NEWLINE																								\
	END_IF																									\
	RECURSE																									\
	RECURSE																									\
	READ_INSTRUCTION(instr)																					\
	IF_INSTRUCTION_MATCHES(instr, Bytecode::ElseIfWrapper, Serialization::ElseIfWrapper)					\
		WRITE(Bytecode::ElseIfWrapper, Serialization::ElseIfWrapper)										\
		NEWLINE																								\
		EXPECT(Bytecode::BeginBlock, Serialization::BeginBlock)												\
		IF_ASSEMBLING																						\
			READ_HEX(nestedscopeid)																			\
			EXPECT(Bytecode::Scope, Serialization::Scope)													\
			WRITE_NUMBER(nestedscopeid)																		\
		ELSE																								\
			NEWLINE																							\
			READ_INSTRUCTION(ignored)																		\
			COPY_HEX(nestedscopeid)																			\
			SPACE																							\
			WRITE_STRING(Serialization::Scope)																\
			NEWLINE																							\
		END_IF																								\
		RECURSE																								\
		RECURSE																								\
	ELSE																									\
		IF_INSTRUCTION_MATCHES(instr, Bytecode::BeginBlock, Serialization::BeginBlock)						\
			IF_ASSEMBLING																					\
				READ_HEX(nestedscopeid)																		\
				EXPECT(Bytecode::Scope, Serialization::Scope)												\
				WRITE_NUMBER(nestedscopeid)																	\
			ELSE																							\
				READ_INSTRUCTION(ignored)																	\
				COPY_HEX(nestedscopeid)																		\
				SPACE																						\
				WRITE_STRING(Serialization::Scope)															\
				NEWLINE																						\
			END_IF																							\
			RECURSE																							\
			RECURSE																							\
		ELSE																								\
			WRITE(Bytecode::NoOp, Serialization::Null)														\
			NEWLINE																							\
		END_IF																								\
	END_IF																									\
	READ_INSTRUCTION(instr2)																				\
	IF_INSTRUCTION_MATCHES(instr2, Bytecode::BeginBlock, Serialization::BeginBlock)							\
		IF_ASSEMBLING																						\
			READ_HEX(innerscopeid)																			\
			WRITE(Bytecode::BeginBlock, Serialization::BeginBlock)											\
			EXPECT(Bytecode::Scope, Serialization::Scope)													\
			WRITE_NUMBER(innerscopeid)																		\
		ELSE																								\
			WRITE(Bytecode::BeginBlock, Serialization::BeginBlock)											\
			NEWLINE																							\
			READ_INSTRUCTION(ignored)																		\
			COPY_HEX(innerscopeid)																			\
			SPACE																							\
			WRITE_STRING(Serialization::Scope)																\
			NEWLINE																							\
		END_IF																								\
		RECURSE																								\
		RECURSE																								\
	ELSE																									\
		WRITE(Bytecode::NoOp, Serialization::Null)															\
		NEWLINE																								\
	END_IF																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::While, Serialization::While)											\
	NEWLINE																									\
	EXPECT(Bytecode::BeginBlock, Serialization::BeginBlock)													\
	IF_ASSEMBLING																							\
		READ_HEX(scopeid)																					\
		EXPECT(Bytecode::Scope, Serialization::Scope)														\
		WRITE_NUMBER(scopeid)																				\
	ELSE																									\
		READ_INSTRUCTION(ignored)																			\
		NEWLINE																								\
		COPY_HEX(scopeid)																					\
		SPACE																								\
		WRITE_STRING(Serialization::Scope)																	\
		NEWLINE																								\
	END_IF																									\
	RECURSE																									\
	RECURSE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::BindReference, Serialization::BindReference)							\
	PARAM_STR(varname)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::WhileCondition, Serialization::WhileCondition)						\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::Scope, Serialization::Scope)													\
	EXCEPTION("Scope instruction not expected; check previous instructions")								\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::GhostRecord, Serialization::GhostRecord)										\
	PARAM_UINT(count)																						\
	LOOP(count)																								\
		COPY_STR(varname)																					\
		PARAM_HEX(ownerscope)																				\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::IsEqual, Serialization::IsEqual)										\
	PARAM_UINT(type)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::ElseIfWrapper, Serialization::ElseIfWrapper)							\
	NEWLINE																									\
	RECURSE																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::ElseIf, Serialization::ElseIf)													\
	NEWLINE																									\
	EXPECT(Bytecode::BeginBlock, Serialization::BeginBlock)													\
	NEWLINE																									\
	IF_ASSEMBLING																							\
		READ_HEX(scopeid)																					\
		EXPECT(Bytecode::Scope, Serialization::Scope)														\
		WRITE_NUMBER(scopeid)																				\
	ELSE																									\
		READ_INSTRUCTION(ignored)																			\
		COPY_HEX(scopeid)																					\
		SPACE																								\
		WRITE_STRING(Serialization::Scope)																	\
		NEWLINE																								\
	END_IF																									\
	RECURSE																									\
	RECURSE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::ExitIfChain, Serialization::ExitIfChain)								\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::Members, Serialization::Members)												\
	PARAM_UINT(count)																						\
	LOOP(count)																								\
		COPY_STR(varname)																					\
		SPACE																								\
		COPY_UINT(vartype)																					\
		PARAM_UINT(offset)																					\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::StaticStrings, Serialization::StaticStrings)									\
	PARAM_UINT(count)																						\
	LOOP(count)																								\
		COPY_UINT(length)																					\
		SKIPTOSTRING(length)																				\
		SPACE																								\
		COPY_RAW(length)																					\
		NEWLINE																								\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::StringVars, Serialization::StringVars)											\
	PARAM_UINT(count)																						\
	LOOP(count)																								\
		COPY_UINT(length)																					\
		SKIPTOSTRING(length)																				\
		SPACE																								\
		COPY_RAW(length)																					\
		NEWLINE																								\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::TupleStaticData, Serialization::TupleStaticData)								\
	NEWLINE																									\
	EXPECT(Bytecode::IDCounter, Serialization::IDCounter)													\
	PARAM_UINT(IDCounter)																					\
	COPY_UINT(count)																						\
	NEWLINE																									\
	LOOP(count)																								\
		COPY_UINT(id)																						\
		PARAM_HEX(ownerscope)																				\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::IDCounter, Serialization::IDCounter)											\
	EXCEPTION("Wrong context for this instruction")															\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::StructureStaticData, Serialization::StructureStaticData)						\
	NEWLINE																									\
	EXPECT(Bytecode::IDCounter, Serialization::IDCounter)													\
	PARAM_UINT(IDCounter)																					\
	COPY_UINT(count)																						\
	NEWLINE																									\
	LOOP(count)																								\
		COPY_UINT(id)																						\
		PARAM_HEX(ownerscope)																				\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::TupleTypes, Serialization::TupleTypes)											\
	PARAM_UINT(count)																						\
	LOOP(count)																								\
		COPY_STR(varname)																					\
		PARAM_UINT(vartype)																					\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::TupleHints, Serialization::TupleTypeHints)										\
	PARAM_UINT(count)																						\
	LOOP(count)																								\
		COPY_STR(varname)																					\
		PARAM_UINT(hint)																					\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::StructureTypes, Serialization::StructureTypes)									\
	PARAM_UINT(count)																						\
	LOOP(count)																								\
		COPY_STR(varname)																					\
		PARAM_UINT(vartype)																					\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::StructureTypeMap, Serialization::StructureTypeMap)								\
	PARAM_UINT(count)																						\
	LOOP(count)																								\
		COPY_UINT(id)																						\
		SPACE																								\
		EXPECT(Bytecode::Members, Serialization::Members)													\
		PARAM_UINT(membercount)																				\
		LOOP(membercount)																					\
			COPY_STR(membername)																			\
			SPACE																							\
			COPY_UINT(membertype)																			\
			SPACE																							\
			COPY_UINT(memberoffset)																			\
			IF_UINT_MATCHES(membertype, VM::EpochVariableType_Structure)									\
				NEWLINE																						\
				COPY_UINT(hint)																				\
			END_IF																							\
			IF_UINT_MATCHES(membertype, VM::EpochVariableType_Tuple)										\
				NEWLINE																						\
				COPY_UINT(hint)																				\
			END_IF																							\
			NEWLINE																							\
		ENDLOOP																								\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::TupleTypeMap, Serialization::TupleTypeMap)										\
	PARAM_UINT(count)																						\
	LOOP(count)																								\
		COPY_UINT(id)																						\
		SPACE																								\
		EXPECT(Bytecode::Members, Serialization::Members)													\
		PARAM_UINT(membercount)																				\
		LOOP(membercount)																					\
			COPY_STR(membername)																			\
			SPACE																							\
			COPY_UINT(membertype)																			\
			PARAM_UINT(memberoffset)																		\
		ENDLOOP																								\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::EndScope, Serialization::EndScope)												\
	NEWLINE																									\
	RETURN																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::DebugWrite, Serialization::DebugWrite)								\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::PushRealLiteral, Serialization::PushRealLiteral)						\
	PARAM_REAL(value)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::DoWhile, Serialization::DoWhile)										\
	NEWLINE																									\
	EXPECT(Bytecode::BeginBlock, Serialization::BeginBlock)													\
	IF_ASSEMBLING																							\
		READ_HEX(scopeid)																					\
		EXPECT(Bytecode::Scope, Serialization::Scope)														\
		WRITE_NUMBER(scopeid)																				\
	ELSE																									\
		READ_INSTRUCTION(ignored)																			\
		NEWLINE																								\
		COPY_HEX(scopeid)																					\
		SPACE																								\
		WRITE_STRING(Serialization::Scope)																	\
		NEWLINE																								\
	END_IF																									\
	RECURSE																									\
	RECURSE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::DivideReals, Serialization::DivideReals)								\
	SPACE																									\
	COPY_BOOL(firstisarray)																					\
	SPACE																									\
	COPY_BOOL(secondisarray)																				\
	SPACE																									\
	COPY_UINT(numparams)																					\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::AddReals, Serialization::AddReals)									\
	SPACE																									\
	COPY_BOOL(firstisarray)																					\
	SPACE																									\
	COPY_BOOL(secondisarray)																				\
	SPACE																									\
	COPY_UINT(numparams)																					\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::SubReals, Serialization::SubtractReals)								\
	SPACE																									\
	COPY_BOOL(firstisarray)																					\
	SPACE																									\
	COPY_BOOL(secondisarray)																				\
	SPACE																									\
	COPY_UINT(numparams)																					\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::IsLesser, Serialization::IsLesser)									\
	PARAM_UINT(type)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::PushBooleanLiteral, Serialization::PushBooleanLiteral)				\
	PARAM_BOOL(flag)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::AddIntegers, Serialization::AddIntegers)								\
	SPACE																									\
	COPY_BOOL(firstisarray)																					\
	SPACE																									\
	COPY_BOOL(secondisarray)																				\
	SPACE																									\
	COPY_UINT(numparams)																					\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::SubtractIntegers, Serialization::SubtractIntegers)					\
	SPACE																									\
	COPY_BOOL(firstisarray)																					\
	SPACE																									\
	COPY_BOOL(secondisarray)																				\
	SPACE																									\
	COPY_UINT(numparams)																					\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::IsGreater, Serialization::IsGreater)									\
	PARAM_UINT(type)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::DebugRead, Serialization::DebugRead)									\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::ReadTuple, Serialization::ReadTuple)									\
	SPACE																									\
	COPY_STR(varname)																						\
	PARAM_STR(membername)																					\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::WriteTuple, Serialization::AssignTuple)								\
	SPACE																									\
	COPY_STR(varname)																						\
	PARAM_STR(membername)																					\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::ReadStructure, Serialization::ReadStructure)							\
	SPACE																									\
	COPY_STR(varname)																						\
	PARAM_STR(membername)																					\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::WriteStructure, Serialization::AssignStructure)						\
	SPACE																									\
	COPY_STR(varname)																						\
	PARAM_STR(membername)																					\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::GlobalBlock, Serialization::GlobalBlock)										\
	NEWLINE																									\
	READ_INSTRUCTION(instruction)																			\
	IF_INSTRUCTION_MATCHES(instruction, Bytecode::BeginBlock, Serialization::BeginBlock)					\
		WRITE(Bytecode::BeginBlock, Serialization::BeginBlock)												\
		NEWLINE																								\
		RECURSE																								\
	ELSE																									\
		WRITE(Bytecode::NoOp, Serialization::Null)															\
		NEWLINE																								\
	END_IF																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::Init, Serialization::InitializeValue)								\
	PARAM_STR(varname)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::PushInteger16Literal, Serialization::PushInteger16Literal)			\
	PARAM_UINT(value)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::Concat, Serialization::Concat)										\
	SPACE																									\
	COPY_BOOL(firstisarray)																					\
	SPACE																									\
	COPY_BOOL(secondisarray)																				\
	SPACE																									\
	COPY_UINT(numparams)																					\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::IsGreaterEqual, Serialization::IsGreaterEqual)						\
	PARAM_UINT(type)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::IsLesserEqual, Serialization::IsLesserEqual)							\
	PARAM_UINT(type)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::BitwiseOr, Serialization::BitwiseOr)									\
	NEWLINE																									\
	COPY_UINT(elementtype)																					\
	SPACE																									\
	READ_NUMBER(testcount)																					\
	WRITE_NUMBER(testcount)																					\
	NEWLINE																									\
	LOOP(testcount)																							\
		COPY_INSTRUCTION																					\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::BitwiseAnd, Serialization::BitwiseAnd)								\
	NEWLINE																									\
	COPY_UINT(elementtype)																					\
	SPACE																									\
	READ_NUMBER(testcount)																					\
	WRITE_NUMBER(testcount)																					\
	NEWLINE																									\
	LOOP(testcount)																							\
		COPY_INSTRUCTION																					\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::BitwiseXor, Serialization::BitwiseXor)								\
	PARAM_UINT(type)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::BitwiseNot, Serialization::BitwiseNot)								\
	PARAM_UINT(type)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::LogicalOr, Serialization::LogicalOr)									\
	NEWLINE																									\
	READ_NUMBER(testcount)																					\
	WRITE_NUMBER(testcount)																					\
	NEWLINE																									\
	LOOP(testcount)																							\
		COPY_INSTRUCTION																					\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::LogicalAnd, Serialization::LogicalAnd)								\
	NEWLINE																									\
	READ_NUMBER(testcount)																					\
	WRITE_NUMBER(testcount)																					\
	NEWLINE																									\
	LOOP(testcount)																							\
		COPY_INSTRUCTION																					\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::LogicalXor, Serialization::LogicalXor)								\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::LogicalNot, Serialization::LogicalNot)								\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::InvokeIndirect, Serialization::InvokeIndirect)						\
	PARAM_STR(funcname)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::Break, Serialization::Break)											\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::Return, Serialization::Return)										\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::BooleanLiteral, Serialization::BooleanConstant)						\
	PARAM_BOOL(value)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::ReadStructureIndirect, Serialization::ReadStructureIndirect)			\
	PARAM_STR(member)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::BindStruct, Serialization::BindStructMemberReference)				\
	SPACE																									\
	COPY_BOOL(chained)																						\
	SPACE																									\
	IF_BOOL(chained)																						\
		COPY_STR(member)																					\
	ELSE																									\
		COPY_STR(varname)																					\
		SPACE																								\
		COPY_STR(member)																					\
	END_IF																									\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::WriteStructureIndirect, Serialization::AssignStructureIndirect)		\
	PARAM_STR(varname)																						\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::Constants, Serialization::Constants)											\
	PARAM_UINT(count)																						\
	LOOP(count)																								\
		COPY_STR(varname)																					\
		NEWLINE																								\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::ResponseMaps, Serialization::ResponseMaps)										\
	PARAM_UINT(count)																						\
	LOOP(count)																								\
		COPY_STR(mapname)																					\
		NEWLINE																								\
		COPY_UINT(entrycount)																				\
		NEWLINE																								\
		LOOP(entrycount)																					\
			COPY_STR(entryname)																				\
			NEWLINE																							\
			COPY_UINT(payloadcount)																			\
			NEWLINE																							\
			LOOP(payloadcount)																				\
				COPY_UINT(payloadtype)																		\
				NEWLINE																						\
			ENDLOOP																							\
			EXPECT(Bytecode::BeginBlock, Serialization::BeginBlock)											\
			NEWLINE																							\
			IF_ASSEMBLING																					\
				READ_HEX(scopeid)																			\
				EXPECT(Bytecode::Scope, Serialization::Scope)												\
				WRITE_HEX(scopeid)																			\
			ELSE																							\
				READ_INSTRUCTION(ignored)																	\
				COPY_HEX(scopeid)																			\
				SPACE																						\
				WRITE_STRING(Serialization::Scope)															\
				NEWLINE																						\
			END_IF																							\
			RECURSE																							\
			RECURSE																							\
			IF_ASSEMBLING																					\
				READ_HEX(scopeid)																			\
				EXPECT(Bytecode::Scope, Serialization::Scope)												\
				WRITE_HEX(scopeid)																			\
			ELSE																							\
				READ_INSTRUCTION(ignored)																	\
				COPY_HEX(scopeid)																			\
				SPACE																						\
				WRITE_STRING(Serialization::Scope)															\
				NEWLINE																						\
			END_IF																							\
			RECURSE																							\
		ENDLOOP																								\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::MultiplyIntegers, Serialization::MultiplyIntegers)					\
	SPACE																									\
	COPY_BOOL(firstisarray)																					\
	SPACE																									\
	COPY_BOOL(secondisarray)																				\
	SPACE																									\
	COPY_UINT(numparams)																					\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::AcceptMessage, Serialization::AcceptMessage)							\
	PARAM_STR(messagename)																					\
	COPY_UINT(payloadcount)																					\
	NEWLINE																									\
	LOOP(payloadcount)																						\
		COPY_UINT(payloadtype)																				\
		NEWLINE																								\
	ENDLOOP																									\
	EXPECT(Bytecode::BeginBlock, Serialization::BeginBlock)													\
	IF_ASSEMBLING																							\
		READ_HEX(innerscopeid)																				\
		EXPECT(Bytecode::Scope, Serialization::Scope)														\
		WRITE_NUMBER(innerscopeid)																			\
	ELSE																									\
		NEWLINE																								\
		READ_INSTRUCTION(ignored)																			\
		COPY_HEX(scopeid)																					\
		SPACE																								\
		WRITE_STRING(Serialization::Scope)																	\
		NEWLINE																								\
	END_IF																									\
	RECURSE																									\
	RECURSE																									\
	IF_ASSEMBLING																							\
		READ_HEX(innerscopeid)																				\
		EXPECT(Bytecode::Scope, Serialization::Scope)														\
		WRITE_NUMBER(innerscopeid)																			\
	ELSE																									\
		READ_INSTRUCTION(ignored)																			\
		COPY_HEX(scopeid)																					\
		SPACE																								\
		WRITE_STRING(Serialization::Scope)																	\
		NEWLINE																								\
	END_IF																									\
	RECURSE																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::FunctionSignatureList, Serialization::FunctionSignatureList)					\
	PARAM_UINT(signaturecount)																				\
	LOOP(signaturecount)																					\
		COPY_STR(funcname)																					\
		SPACE																								\
		RECURSE																								\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::FunctionSignatureBegin, Serialization::FunctionSignatureBegin)					\
	PARAM_UINT(numparams)																					\
	LOOP(numparams)																							\
		COPY_UINT(paramtype)																				\
		SPACE																								\
	ENDLOOP																									\
	NEWLINE																									\
	COPY_UINT(numreturns)																					\
	NEWLINE																									\
	LOOP(numreturns)																						\
		COPY_UINT(returntype)																				\
		SPACE																								\
	ENDLOOP																									\
	NEWLINE																									\
	COPY_UINT(numhints)																						\
	NEWLINE																									\
	LOOP(numhints)																							\
		COPY_UINT(hint)																						\
		SPACE																								\
	ENDLOOP																									\
	NEWLINE																									\
	COPY_UINT(numflags)																						\
	NEWLINE																									\
	LOOP(numflags)																							\
		COPY_UINT(flag)																						\
		SPACE																								\
	ENDLOOP																									\
	NEWLINE																									\
	COPY_UINT(numsignatures)																				\
	NEWLINE																									\
	LOOP(numsignatures)																						\
		RECURSE																								\
	ENDLOOP																									\
	NEWLINE																									\
	COPY_UINT(numreturnhints)																				\
	NEWLINE																									\
	LOOP(numreturnhints)																					\
		COPY_UINT(hint)																						\
		SPACE																								\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::FunctionSignatureEnd, Serialization::FunctionSignatureEnd)						\
	NEWLINE																									\
	RETURN																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::ForkTask, Serialization::ForkTask)									\
	EXPECT(Bytecode::BeginBlock, Serialization::BeginBlock)													\
	IF_ASSEMBLING																							\
		READ_HEX(scopeid)																					\
		EXPECT(Bytecode::Scope, Serialization::Scope)														\
		WRITE_NUMBER(scopeid)																				\
	ELSE																									\
		READ_INSTRUCTION(ignored)																			\
		NEWLINE																								\
		COPY_HEX(scopeid)																					\
		SPACE																								\
		WRITE_STRING(Serialization::Scope)																	\
		NEWLINE																								\
	END_IF																									\
	RECURSE																									\
	RECURSE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::GetMessageSender, Serialization::GetMessageSender)					\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::GetTaskCaller, Serialization::GetTaskCaller)							\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::AcceptMessageFromMap, Serialization::AcceptMessageFromMap)			\
	PARAM_STR(handlermapid)																					\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::SendTaskMessage, Serialization::SendTaskMessage)						\
	SPACE																									\
	COPY_BOOL(explicittaskname)																				\
	SPACE																									\
	COPY_STR(messagename)																					\
	SPACE																									\
	COPY_UINT(signaturecount)																				\
	NEWLINE																									\
	LOOP(signaturecount)																					\
		COPY_UINT(type)																						\
		NEWLINE																								\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::TypeCast, Serialization::TypeCast)									\
	SPACE																									\
	COPY_UINT(origintype)																					\
	SPACE																									\
	COPY_UINT(desttype)																						\
    NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::TypeCastToString, Serialization::TypeCastToString)					\
	SPACE																									\
	COPY_UINT(origintype)																					\
    NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::DivideIntegers, Serialization::DivideIntegers)						\
	SPACE																									\
	COPY_BOOL(firstisarray)																					\
	SPACE																									\
	COPY_BOOL(secondisarray)																				\
	SPACE																									\
	COPY_UINT(numparams)																					\
    NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::DivideInteger16s, Serialization::DivideInteger16s)					\
	SPACE																									\
	COPY_BOOL(firstisarray)																					\
	SPACE																									\
	COPY_BOOL(secondisarray)																				\
	SPACE																									\
	COPY_UINT(numparams)																					\
    NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_INSTRUCTION(Bytecode::Futures, Serialization::Futures)												\
	PARAM_UINT(count)																						\
	LOOP(count)																								\
		COPY_STR(futurename)																				\
		SPACE																								\
		COPY_HEX(futureopaddr)																				\
		SPACE																								\
		COPY_INSTRUCTION																					\
	ENDLOOP																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::Future, Serialization::ForkFuture)									\
	SPACE																									\
	COPY_STR(futurename)																					\
	SPACE																									\
	COPY_UINT(futuretype)																					\
	SPACE																									\
	COPY_BOOL(usesthreadpool)																				\
    NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::Map, Serialization::Map)												\
	SPACE																									\
	COPY_INSTRUCTION																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::Reduce, Serialization::Reduce)										\
	SPACE																									\
	COPY_INSTRUCTION																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::IntegerLiteral, Serialization::IntegerConstant)						\
	PARAM_UINT(value)																						\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::ThreadPool, Serialization::ThreadPool)								\
	NEWLINE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::ForkThread, Serialization::ForkThread)								\
	EXPECT(Bytecode::BeginBlock, Serialization::BeginBlock)													\
	IF_ASSEMBLING																							\
		READ_HEX(scopeid)																					\
		EXPECT(Bytecode::Scope, Serialization::Scope)														\
		WRITE_NUMBER(scopeid)																				\
	ELSE																									\
		READ_INSTRUCTION(ignored)																			\
		NEWLINE																								\
		COPY_HEX(scopeid)																					\
		SPACE																								\
		WRITE_STRING(Serialization::Scope)																	\
		NEWLINE																								\
	END_IF																									\
	RECURSE																									\
	RECURSE																									\
END_INSTRUCTION																								\
																											\
DEFINE_ADDRESSED_INSTRUCTION(Bytecode::Handoff, Serialization::Handoff)										\
	PARAM_STR(libraryname)																					\
	EXPECT(Bytecode::BeginBlock, Serialization::BeginBlock)													\
	IF_ASSEMBLING																							\
		READ_HEX(scopeid)																					\
		EXPECT(Bytecode::Scope, Serialization::Scope)														\
		WRITE_NUMBER(scopeid)																				\
	ELSE																									\
		READ_INSTRUCTION(ignored)																			\
		NEWLINE																								\
		COPY_HEX(scopeid)																					\
		SPACE																								\
		WRITE_STRING(Serialization::Scope)																	\
		NEWLINE																								\
	END_IF																									\
	RECURSE																									\
	RECURSE																									\
END_INSTRUCTION																								\




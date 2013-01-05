//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Just-in-time native code generation for Epoch
//

#include "pch.h"

#include "Bytecode/Instructions.h"

#include "Libraries/Library.h"

#include "User Interface/Output.h"

#include "Virtual Machine/VirtualMachine.h"

#include "Metadata/ScopeDescription.h"
#include "Metadata/TypeInfo.h"

#include "Utility/Strings.h"

#include <llvm/Intrinsics.h>
#include <llvm/Support/CrashRecoveryContext.h>
#include <llvm/Transforms/Vectorize.h>

#pragma warning(push)
#pragma warning(disable: 4146)
#include <llvm/ADT/Statistic.h>
#pragma warning(pop)

#include <sstream>
#include <map>
#include <stack>
#include <iostream>


//
// Internal implementation helpers
//
namespace JIT
{
	namespace impl
	{

		enum VMInteropFunc
		{
			VMBreak,
			VMHalt,
			VMAllocStruct,
			VMCopyStruct,
			VMGetBuffer,
			VMReturnFromFunction,
		};

		struct LLVMData
		{
			LLVMData();

			llvm::LLVMContext& Context;

			llvm::Module* CurrentModule;

			llvm::Type* VMTypeIDType;
			llvm::Type* VMContextPtrType;
			llvm::Type* VMBufferHandleType;
			llvm::Type* VMStructureHandleType;
			llvm::Type* VMNothingType;
			
			std::map<VMInteropFunc, llvm::Function*> VMFunctions;

			std::map<std::string, llvm::Function*> GeneratedFunctions;
			std::map<std::string, llvm::Function*> GeneratedNativeTypeMatchers;
			std::map<StringHandle, llvm::Function*> GeneratedBridgeFunctions;

			std::map<Metadata::EpochTypeID, llvm::StructType*> SumTypeCache;

		// Non-copyable
		private:
			LLVMData(const LLVMData&);
			LLVMData& operator = (const LLVMData&);
		};


		//
		// Construct and initialize an LLVM data wrapper object
		//
		LLVMData::LLVMData() :
			Context(llvm::getGlobalContext())
		{
			using namespace llvm;

			CurrentModule = new Module("EpochJIT", Context);

			VMTypeIDType = Type::getInt32Ty(Context);
			VMContextPtrType = Type::getInt1PtrTy(Context);
			VMBufferHandleType = Type::getInt32Ty(Context);
			VMStructureHandleType = Type::getInt8PtrTy(Context);
			VMNothingType = Type::getInt32Ty(Context);

			// Set up various VM interop functions
			{
				FunctionType* ftype = FunctionType::get(Type::getVoidTy(getGlobalContext()), false);
				VMFunctions[VMBreak] = Function::Create(ftype, Function::ExternalLinkage, "VMBreak", CurrentModule);
			}

			{
				FunctionType* ftype = FunctionType::get(Type::getVoidTy(Context), false);
				VMFunctions[VMHalt] = Function::Create(ftype, Function::ExternalLinkage, "VMHalt", CurrentModule);
			}

			{
				std::vector<Type*> args;
				args.push_back(VMContextPtrType);
				args.push_back(VMTypeIDType);
				FunctionType* ftype = FunctionType::get(VMStructureHandleType, args, false);

				VMFunctions[VMAllocStruct] = Function::Create(ftype, Function::ExternalLinkage, "VMAllocStruct", CurrentModule);
			}

			{
				std::vector<Type*> args;
				args.push_back(VMContextPtrType);
				args.push_back(VMStructureHandleType);
				FunctionType* ftype = FunctionType::get(VMStructureHandleType, args, false);

				VMFunctions[VMCopyStruct] = Function::Create(ftype, Function::ExternalLinkage, "VMCopyStruct", CurrentModule);
			}

			{
				std::vector<Type*> args;
				args.push_back(VMContextPtrType);
				args.push_back(VMBufferHandleType);
				FunctionType* ftype = FunctionType::get(Type::getInt1PtrTy(Context), args, false);

				VMFunctions[VMGetBuffer] = Function::Create(ftype, Function::ExternalLinkage, "VMGetBuffer", CurrentModule);
			}

			{
				std::vector<Type*> args;
				args.push_back(VMContextPtrType);
				FunctionType* ftype = FunctionType::get(Type::getVoidTy(Context), args, false);

				VMFunctions[VMReturnFromFunction] = Function::Create(ftype, Function::ExternalLinkage, "VMRet", CurrentModule);
			}
		}

	}
}


//
// Additional internal helpers
//
namespace
{

	template <typename T>
	T Fetch(const Bytecode::Instruction* bytecode, size_t& InstructionOffset)
	{
		const T* data = reinterpret_cast<const T*>(&bytecode[InstructionOffset]);
		InstructionOffset += sizeof(T);
		return static_cast<T>(*data);
	}

}


// Namespace shortcuts
using namespace JIT;
using namespace JIT::impl;


//
// Map Epoch types to LLVM types
//
llvm::Type* NativeCodeGenerator::GetLLVMType(Metadata::EpochTypeID type, bool flatten)
{
	Metadata::EpochTypeFamily family = Metadata::GetTypeFamily(type);
	switch(type)
	{
	case Metadata::EpochType_Integer:
	case Metadata::EpochType_String:
	case Metadata::EpochType_Function:
	case Metadata::EpochType_Buffer:
		return llvm::Type::getInt32Ty(Data->Context);

	case Metadata::EpochType_Real:
		return llvm::Type::getFloatTy(Data->Context);

	case Metadata::EpochType_Boolean:
		return llvm::Type::getInt1Ty(Data->Context);

	case Metadata::EpochType_Integer16:
		return llvm::Type::getInt16Ty(Data->Context);

	case Metadata::EpochType_Identifier:
		// STUPID LAME HACK - we only use Identifier params for constructors, so assume this is an aggregate constructor param
		return Data->VMStructureHandleType;

	default:
		if(family == Metadata::EpochTypeFamily_SumType)
			return GetLLVMSumType(type, flatten);

		if(family == Metadata::EpochTypeFamily_Structure || family == Metadata::EpochTypeFamily_TemplateInstance)
			return Data->VMStructureHandleType;

		throw NotImplementedException("Unsupported type for native code generation");
	}
}

//
// Create tagged structures to hold Epoch unions in LLVM structs
//
// Since LLVM does not support unions, we need to fake this by creating
// a field of the largest necesary bit width and doing a lot of pointer
// casting when reading/writing the union payload.
//
llvm::Type* NativeCodeGenerator::GetLLVMSumType(Metadata::EpochTypeID type, bool flatten)
{
	llvm::StructType* taggedtype = Data->SumTypeCache[type];
	if(!taggedtype && !flatten)
	{
		const VariantDefinition& def = OwnerVM.VariantDefinitions.find(type)->second;
		const std::set<Metadata::EpochTypeID>& types = def.GetBaseTypes();

		llvm::Type* rettype = NULL;
		size_t maxsize = 0;

		for(std::set<Metadata::EpochTypeID>::const_iterator iter = types.begin(); iter != types.end(); ++iter)
		{
			size_t size = Metadata::GetStorageSize(*iter);
			if(size > maxsize)
			{
				maxsize = size;
				rettype = GetLLVMType(*iter, true);
			}
		}

		if(flatten)
			return rettype;

		std::ostringstream name;
		name << "SumTypeTag_" << type;

		std::vector<llvm::Type*> elemtypes;
		elemtypes.push_back(Data->VMTypeIDType);
		elemtypes.push_back(rettype);
		taggedtype = llvm::StructType::create(elemtypes, name.str());
		Data->SumTypeCache[type] = taggedtype;
	}

	return taggedtype;
}

//
// Synthesize the LLVM function type signature for a given Epoch function
//
llvm::FunctionType* NativeCodeGenerator::GetLLVMFunctionType(StringHandle epochfunc)
{
	using namespace llvm;
	
	Type* rettype = Type::getVoidTy(Data->Context);

	std::vector<Type*> args;
	args.push_back(Data->VMContextPtrType);

	const ScopeDescription& scope = OwnerVM.GetScopeDescription(epochfunc);
	for(size_t i = 0; i < scope.GetVariableCount(); ++i)
	{
		if(scope.GetVariableOrigin(i) == VARIABLE_ORIGIN_PARAMETER)
		{
			Metadata::EpochTypeID vartype = scope.GetVariableTypeByIndex(i);

			if(vartype == Metadata::EpochType_Nothing)
			{
				args.push_back(Data->VMNothingType);
				continue;
			}

			Type* type = GetLLVMType(vartype);
			if(scope.IsReference(i))
				args.push_back(type->getPointerTo());
			else
				args.push_back(type);
		}
		else if(scope.GetVariableOrigin(i) == VARIABLE_ORIGIN_RETURN)
			rettype = GetLLVMType(scope.GetVariableTypeByIndex(i));
	}
	
	return FunctionType::get(rettype, args, false);
}




// TODO - everything below here needs organization and cleanup

void NativeCodeGenerator::AddNativeTypeMatcher(size_t beginoffset, size_t endoffset)
{
	using namespace llvm;

	Function* matcherfunction = Builder.GetInsertBlock()->getParent();

	std::vector<Value*> reftypes;
	std::vector<Value*> reftargets;

	std::vector<std::vector<Value*> > parampayloadptrs;
	std::vector<std::vector<Value*> > providedtypeholders;

	unsigned typematchindex = 0;
	StringHandle entityname = 0;

	// Count type matchers in this entity
	for(size_t offset = beginoffset; offset <= endoffset; )
	{
		Bytecode::Instruction instruction = Bytecode[offset++];
		switch(instruction)
		{
		case Bytecode::Instructions::BeginEntity:
			Fetch<Integer32>(Bytecode, offset);
			Fetch<StringHandle>(Bytecode, offset);
			break;

		case Bytecode::Instructions::EndEntity:
			break;

		case Bytecode::Instructions::Halt:
			break;

		case Bytecode::Instructions::TypeMatch:
			{
				parampayloadptrs.push_back(std::vector<Value*>());
				providedtypeholders.push_back(std::vector<Value*>());

				Fetch<StringHandle>(Bytecode, offset);
				Fetch<size_t>(Bytecode, offset);
				size_t paramcount = Fetch<size_t>(Bytecode, offset);

				for(size_t i = 0; i < paramcount; ++i)
				{
					Fetch<bool>(Bytecode, offset);
					Fetch<Metadata::EpochTypeID>(Bytecode, offset);

					parampayloadptrs.back().push_back(Builder.CreateAlloca(Type::getInt8PtrTy(Data->Context)));
					providedtypeholders.back().push_back(Builder.CreateAlloca(Type::getInt32Ty(Data->Context)));
				}
			}
			break;

		default:
			throw FatalException("Invalid opcode in native type matcher");
		}
	}


	for(size_t offset = beginoffset; offset <= endoffset; )
	{
		Bytecode::Instruction instruction = Bytecode[offset++];
		switch(instruction)
		{
		case Bytecode::Instructions::BeginEntity:
			Fetch<Integer32>(Bytecode, offset);
			entityname = Fetch<StringHandle>(Bytecode, offset);
			break;

		case Bytecode::Instructions::EndEntity:
			break;

		case Bytecode::Instructions::Halt:
			Builder.CreateCall(Data->VMFunctions[VMHalt]);
			break;

		case Bytecode::Instructions::TypeMatch:
			{
				StringHandle funcname = Fetch<StringHandle>(Bytecode, offset);
				size_t matchoffset = Fetch<size_t>(Bytecode, offset);
				size_t paramcount = Fetch<size_t>(Bytecode, offset);

				std::vector<Value*> actualparams;
				BasicBlock* nexttypematcher = BasicBlock::Create(Data->Context, "nexttypematcher", matcherfunction);

				Function::arg_iterator argiter = matcherfunction->arg_begin();

				if(reftypes.empty())
				{
					for(size_t i = 0; i < paramcount; ++i)
					{
						++argiter;
						Value* reftype = argiter;
						++argiter;
						Value* reftarget = argiter;

						reftypes.push_back(reftype);
						reftargets.push_back(reftarget);
					}
				}

				for(size_t i = 0; i < paramcount; ++i)
				{
					bool expectref = Fetch<bool>(Bytecode, offset);
					Metadata::EpochTypeID expecttype = Fetch<Metadata::EpochTypeID>(Bytecode, offset);

					BasicBlock* checkmatchblock = BasicBlock::Create(Data->Context, "checkmatch", matcherfunction);

					Builder.CreateStore(reftypes[i], providedtypeholders[typematchindex][i]);

					BasicBlock* setnothingrefflagblock = BasicBlock::Create(Data->Context, "setnothingrefflag", matcherfunction);
					BasicBlock* skipblock = BasicBlock::Create(Data->Context, "skip", matcherfunction);

					Value* providednothingflag = Builder.CreateICmpEQ(reftypes[i], ConstantInt::get(Type::getInt32Ty(Data->Context), Metadata::EpochType_Nothing));
					Builder.CreateCondBr(providednothingflag, setnothingrefflagblock, skipblock);

					Builder.SetInsertPoint(setnothingrefflagblock);
					Builder.CreateBr(skipblock);

					Builder.SetInsertPoint(skipblock);
					Builder.CreateStore(reftargets[i], parampayloadptrs[typematchindex][i]);


					BasicBlock* handlesumtypeblock = BasicBlock::Create(Data->Context, "handlesumtype", matcherfunction);

					Value* providedtypefamily = Builder.CreateAnd(Builder.CreateLoad(providedtypeholders[typematchindex][i]), 0xff000000);
					Value* issumtype = Builder.CreateICmpEQ(providedtypefamily, ConstantInt::get(Type::getInt32Ty(Data->Context), Metadata::EpochTypeFamily_SumType));
					Builder.CreateCondBr(issumtype, handlesumtypeblock, checkmatchblock);

					Builder.SetInsertPoint(handlesumtypeblock);

					Value* rt = Builder.CreateLoad(Builder.CreatePointerCast(reftargets[i], Type::getInt32PtrTy(Data->Context)));
					Builder.CreateStore(rt, providedtypeholders[typematchindex][i]);

					if(Metadata::GetTypeFamily(expecttype) == Metadata::EpochTypeFamily_SumType)
					{
						Builder.CreateStore(reftargets[i], parampayloadptrs[typematchindex][i]);
					}
					else
					{
						Value* gep = Builder.CreateGEP(reftargets[i], ConstantInt::get(Type::getInt32Ty(Data->Context), sizeof(Metadata::EpochTypeID)));
						Builder.CreateStore(gep, parampayloadptrs[typematchindex][i]);
					}
					Builder.CreateBr(checkmatchblock);

					Builder.SetInsertPoint(checkmatchblock);

					Value* nomatch = Builder.CreateICmpNE(Builder.CreateLoad(providedtypeholders[typematchindex][i]), ConstantInt::get(Type::getInt32Ty(Data->Context), expecttype));
					Value* notexpectsumtype = ConstantInt::get(Type::getInt1Ty(Data->Context), Metadata::GetTypeFamily(expecttype) != Metadata::EpochTypeFamily_SumType);

					BasicBlock* nextparamblock = BasicBlock::Create(Data->Context, "nextparam", matcherfunction);

					Value* bailflag = Builder.CreateAnd(nomatch, notexpectsumtype);
					Builder.CreateCondBr(bailflag, nexttypematcher, nextparamblock);

					Builder.SetInsertPoint(nextparamblock);

					if(expecttype != Metadata::EpochType_Nothing)
					{
						if(Metadata::GetTypeFamily(expecttype) == Metadata::EpochTypeFamily_Structure || Metadata::GetTypeFamily(expecttype) == Metadata::EpochTypeFamily_TemplateInstance)
						{
							Value* v = Builder.CreatePointerCast(parampayloadptrs[typematchindex][i], GetLLVMType(expecttype)->getPointerTo()->getPointerTo());
							v = Builder.CreateLoad(v);

							actualparams.push_back(v);
						}
						else if(!expectref)
						{
							Value* v = Builder.CreatePointerCast(parampayloadptrs[typematchindex][i], GetLLVMType(expecttype)->getPointerTo()->getPointerTo());
							v = Builder.CreateLoad(v);
							v = Builder.CreateLoad(v);

							actualparams.push_back(v);
						}
						else if(Metadata::GetTypeFamily(expecttype) == Metadata::EpochTypeFamily_SumType)
						{
							Value* v = Builder.CreatePointerCast(parampayloadptrs[typematchindex][i], GetLLVMType(expecttype)->getPointerTo()->getPointerTo());
							v = Builder.CreateLoad(v);

							actualparams.push_back(v);
						}
						else
						{
							Value* v = Builder.CreatePointerCast(parampayloadptrs[typematchindex][i], GetLLVMType(expecttype)->getPointerTo()->getPointerTo());
							v = Builder.CreateLoad(v);

							actualparams.push_back(v);
						}
					}
					else
					{
						actualparams.push_back(ConstantInt::get(Type::getInt32Ty(Data->Context), 0xbaadf00d));
					}
				}

				std::ostringstream matchname;
				matchname << "JITFuncInner_" << matchoffset;

				Function* targetinnerfunc = NULL;
				FunctionType* targetinnerfunctype = GetLLVMFunctionType(funcname);
				if(!Data->GeneratedFunctions[matchname.str()])
				{
					targetinnerfunc = Function::Create(targetinnerfunctype, Function::InternalLinkage, matchname.str().c_str(), Data->CurrentModule);
					Data->GeneratedFunctions[matchname.str()] = targetinnerfunc;
				}
				else
					targetinnerfunc = Data->GeneratedFunctions[matchname.str()];

				std::vector<Value*> resolvedargs;
				for(std::vector<Value*>::const_iterator argiter = actualparams.begin(); argiter != actualparams.end(); ++argiter)
					resolvedargs.push_back(*argiter);
				resolvedargs.push_back(matcherfunction->arg_begin());
				std::reverse(resolvedargs.begin(), resolvedargs.end());

				if(targetinnerfunc->getReturnType() != Type::getVoidTy(Data->Context))
				{
					CallInst* typematchret = Builder.CreateCall(targetinnerfunc, resolvedargs);
					Builder.CreateRet(typematchret);
				}
				else
				{
					Builder.CreateCall(targetinnerfunc, resolvedargs);
					Builder.CreateRetVoid();
				}

				Builder.SetInsertPoint(nexttypematcher);
			}

			++typematchindex;
			break;

		default:
			throw FatalException("Invalid instruction in type matcher");
		}
	}

	Builder.CreateCall(Data->VMFunctions[VMHalt]);
	Builder.CreateUnreachable();
}


//
// Add a standard Epoch function implementation to the JIT module
//
void NativeCodeGenerator::AddFunction(size_t beginoffset, size_t endoffset, StringHandle alias)
{
	using namespace llvm;

	PointerType* stackptrtype = Type::getInt8PtrTy(Data->Context);
	PointerType* pstackptrtype = stackptrtype->getPointerTo();

	std::ostringstream name;
	name << "JITFunc_" << beginoffset;

	Function* jitbridgefunction = Data->GeneratedFunctions[name.str()];
	if(!jitbridgefunction)
	{
		std::vector<Type*> args;
		args.push_back(pstackptrtype);
		args.push_back(Data->VMContextPtrType);
		FunctionType* ftype = FunctionType::get(Type::getVoidTy(Data->Context), args, false);

		jitbridgefunction = Function::Create(ftype, Function::ExternalLinkage, name.str().c_str(), Data->CurrentModule);
		Data->GeneratedFunctions[name.str()] = jitbridgefunction;
	}

	Function* sqrtintrinsic = Data->CurrentModule->getFunction("llvm.sqrt.f32");
	if(!sqrtintrinsic)
	{
		std::vector<Type*> argtypes;
		argtypes.push_back(Type::getFloatTy(Data->Context));
		FunctionType* functype = FunctionType::get(Type::getFloatTy(Data->Context), argtypes, false);
		sqrtintrinsic = Function::Create(functype, Function::ExternalLinkage, "llvm.sqrt.f32", Data->CurrentModule);
	}


	BasicBlock* block = BasicBlock::Create(Data->Context, "entry", jitbridgefunction);
	Builder.SetInsertPoint(block);

	Value* pstackptr = Builder.CreateAlloca(pstackptrtype);
	Builder.CreateStore(jitbridgefunction->arg_begin(), pstackptr);
	Value* vmcontextptr = ++(jitbridgefunction->arg_begin());

	size_t localoffset = 0;
	
	BasicBlock* outerfunctionexit = BasicBlock::Create(Data->Context, "exit", jitbridgefunction);

	JIT::JITContext jitcontext;
	jitcontext.Builder = &Builder;
	jitcontext.VMContextPtr = NULL;
	jitcontext.Context = &Data->Context;
	jitcontext.MyModule = Data->CurrentModule;
	jitcontext.InnerFunction = NULL;
	jitcontext.VMGetBuffer = Data->VMFunctions[VMGetBuffer];
	jitcontext.SqrtIntrinsic = sqrtintrinsic;
	BasicBlock* innerfunctionexit = NULL;
	BasicBlock* nativematchblock = NULL;
	
	Value* retval = NULL;
	Value* innerretval = NULL;
	Value* typematchret = NULL;
	unsigned numparameters = 0;
	unsigned numparambytes = 0;
	unsigned numreturns = 0;
	unsigned typematchindex = 0;

	std::stack<Metadata::EpochTypeID> annotations;
	std::map<size_t, size_t> offsettoindexmap;
	std::map<size_t, size_t> paramoffsettoindexmap;
	std::map<Value*, Value*> structurerefcache;

	const ScopeDescription* curscope = NULL;

	Metadata::EpochTypeID hackstructtype = 0;

	Value* typematchconsumedbytes = NULL;

	for(size_t offset = beginoffset; offset <= endoffset; )
	{
		Bytecode::Instruction instruction = Bytecode[offset++];
		switch(instruction)
		{
		case Bytecode::Instructions::BeginEntity:
			{
				Bytecode::EntityTag entitytype = Fetch<Integer32>(Bytecode, offset);
				jitcontext.EntityTypes.push(entitytype);

				StringHandle entityname = Fetch<StringHandle>(Bytecode, offset);

				if(entitytype == Bytecode::EntityTags::Function)
				{
					Type* rettype = Type::getVoidTy(Data->Context);

					Value* stackptr = Builder.CreateLoad(Builder.CreateLoad(pstackptr));
					Type* type = NULL;

					std::set<size_t> locals;
					std::set<size_t> parameters;

					size_t localoffsetbytes = 0;

					size_t retindex = static_cast<size_t>(-1);

					const ScopeDescription& scope = OwnerVM.GetScopeDescription(entityname);
					curscope = &scope;
					for(size_t i = scope.GetVariableCount(); i-- > 0; )
					{
						Metadata::EpochTypeID vartype = scope.GetVariableTypeByIndex(i);

						if(vartype == Metadata::EpochType_Nothing)
						{
							if(scope.GetVariableOrigin(i) == VARIABLE_ORIGIN_PARAMETER)
							{
								Value* zero = Builder.CreateAlloca(Type::getInt32Ty(Data->Context));
								Builder.CreateStore(ConstantInt::get(Type::getInt32Ty(Data->Context), 0xfeedface), zero);
								parameters.insert(i);
								++numparameters;
								jitcontext.VariableMap[i] = zero;
							}
							continue;
						}

						type = GetLLVMType(vartype);

						switch(scope.GetVariableOrigin(i))
						{
						case VARIABLE_ORIGIN_RETURN:
							++numreturns;
							retindex = i;
							rettype = type;
							jitcontext.VariableMap[i] = Builder.CreateAlloca(type);
							// Deliberate fallthrough

						case VARIABLE_ORIGIN_LOCAL:
							locals.insert(i);
							break;

						case VARIABLE_ORIGIN_PARAMETER:
							{
								paramoffsettoindexmap[numparambytes] = i;
								parameters.insert(i);
								Constant* offset = ConstantInt::get(Type::getInt32Ty(Data->Context), numparambytes);
								++numparameters;
								++localoffset;

								if(scope.IsReference(i))
								{
									Type* ptype = PointerType::get(PointerType::get(type, 0), 0);
									Value* newstackptr = Builder.CreateGEP(stackptr, offset);
									Value* ref = Builder.CreatePointerCast(newstackptr, ptype);
									jitcontext.VariableMap[i] = ref;
									numparambytes += sizeof(void*) + sizeof(Metadata::EpochTypeID);
								}
								else
								{
									Value* local = Builder.CreateAlloca(type);
									jitcontext.VariableMap[i] = local;
									Value* newstackptr = Builder.CreateGEP(stackptr, offset);
									Value* stackval = Builder.CreateLoad(Builder.CreatePointerCast(newstackptr, type->getPointerTo()));
									Builder.CreateStore(stackval, local);
									numparambytes += Metadata::GetStorageSize(vartype);
								}
							}
							break;
						}

						jitcontext.NameToIndexMap[scope.GetVariableNameHandle(i)] = i;
					}

					std::ostringstream name;
					name << "JITFuncInner_" << beginoffset;

					FunctionType* innerfunctype = GetLLVMFunctionType(entityname);
					if(!Data->GeneratedFunctions[name.str()])
					{
						jitcontext.InnerFunction = Function::Create(innerfunctype, Function::InternalLinkage, name.str().c_str(), Data->CurrentModule);
						Data->GeneratedFunctions[name.str()] = jitcontext.InnerFunction;
					}
					else
						jitcontext.InnerFunction = Data->GeneratedFunctions[name.str()];

					Builder.SetInsertPoint(block);

					std::vector<Value*> innerparams;
					innerparams.push_back(vmcontextptr);
					for(std::set<size_t>::const_iterator paramiter = parameters.begin(); paramiter != parameters.end(); ++paramiter)
					{
						Value* p = Builder.CreateLoad(jitcontext.VariableMap[*paramiter]);
						innerparams.push_back(p);
					}

					if(numreturns)
					{
						Value* r = Builder.CreateCall(jitcontext.InnerFunction, innerparams);
						Builder.CreateStore(r, jitcontext.VariableMap[retindex]);
					}
					else
						Builder.CreateCall(jitcontext.InnerFunction, innerparams);

					BasicBlock* innerentryblock = BasicBlock::Create(Data->Context, "innerentry", jitcontext.InnerFunction);
					Builder.SetInsertPoint(innerentryblock);

					innerfunctionexit = BasicBlock::Create(Data->Context, "innerexit", jitcontext.InnerFunction);

					if(numreturns)
						innerretval = Builder.CreateAlloca(rettype);

					size_t i = 0;
					retval = numreturns ? jitcontext.VariableMap[retindex] : NULL;
					Function::ArgumentListType& args = jitcontext.InnerFunction->getArgumentList();
					Function::ArgumentListType::iterator argiter = args.begin();
					++argiter;
					for(; argiter != args.end(); ++argiter)
						jitcontext.VariableMap[i++] = ((Argument*)argiter);

					if(numreturns)
						jitcontext.VariableMap[retindex] = innerretval;

					localoffsetbytes = 0;
					for(std::set<size_t>::const_iterator localiter = locals.begin(); localiter != locals.end(); ++localiter)
					{
						Metadata::EpochTypeID localtype = scope.GetVariableTypeByIndex(*localiter);
						Type* type = GetLLVMType(localtype);

						if(*localiter != retindex)
							jitcontext.VariableMap[*localiter] = Builder.CreateAlloca(type, NULL, narrow(OwnerVM.GetPooledString(scope.GetVariableNameHandle(*localiter))));

						offsettoindexmap[localoffsetbytes] = *localiter;

						if(Metadata::GetTypeFamily(localtype) == Metadata::EpochTypeFamily_SumType)
							localoffsetbytes += OwnerVM.VariantDefinitions.find(localtype)->second.GetMaxSize();
						else
							localoffsetbytes += Metadata::GetStorageSize(localtype);
					}

					jitcontext.VMContextPtr = jitcontext.InnerFunction->arg_begin();
				}
				else if(entitytype == Bytecode::EntityTags::TypeResolver)
				{
					std::ostringstream matchername;
					matchername << "JITMatcher_" << beginoffset;

					Function* nativetypematcher = Data->GeneratedNativeTypeMatchers[matchername.str()];
					if(!nativetypematcher)
					{
						std::vector<Type*> matchargtypes;
						matchargtypes.push_back(vmcontextptr->getType());
						for(size_t i = 0; i < OwnerVM.TypeMatcherParamCount.find(entityname)->second; ++i)
						{
							matchargtypes.push_back(Data->VMTypeIDType);					// type annotation
							matchargtypes.push_back(Type::getInt8PtrTy(Data->Context));		// pointer to payload
						}

						Type* retty = Type::getVoidTy(Data->Context);
						if(OwnerVM.TypeMatcherRetType.find(entityname)->second != Metadata::EpochType_Error)
							retty = GetLLVMType(OwnerVM.TypeMatcherRetType.find(entityname)->second);
						FunctionType* matchfunctype = FunctionType::get(retty, matchargtypes, false);

						nativetypematcher = Function::Create(matchfunctype, Function::ExternalLinkage, matchername.str().c_str(), Data->CurrentModule);
						Data->GeneratedNativeTypeMatchers[matchername.str()] = nativetypematcher;
					}

					nativematchblock = BasicBlock::Create(Data->Context, "nativematchentry", nativetypematcher);
				}
				else
				{
					std::map<StringHandle, JIT::JITHelper>::const_iterator helperiter = OwnerVM.JITHelpers.EntityHelpers.find(entitytype);
					if(helperiter == OwnerVM.JITHelpers.EntityHelpers.end())
						throw FatalException("Cannot JIT this type of entity");
					
					helperiter->second(jitcontext, true);
				}
			}
			break;

		case Bytecode::Instructions::EndEntity:
			{
				Bytecode::EntityTag tag = jitcontext.EntityTypes.top();
				jitcontext.EntityTypes.pop();
				if(tag != Bytecode::EntityTags::Function && tag != Bytecode::EntityTags::TypeResolver)
				{
					OwnerVM.JITHelpers.EntityHelpers.find(tag)->second(jitcontext, false);
				}
			}
			break;

		case Bytecode::Instructions::BeginChain:
			jitcontext.EntityChecks.push(BasicBlock::Create(Data->Context, "", jitcontext.InnerFunction));
			jitcontext.EntityChains.push(BasicBlock::Create(Data->Context, "", jitcontext.InnerFunction));
			jitcontext.EntityChainExits.push(BasicBlock::Create(Data->Context, "", jitcontext.InnerFunction));
			Builder.CreateBr(jitcontext.EntityChecks.top());
			Builder.SetInsertPoint(jitcontext.EntityChecks.top());
			break;

		case Bytecode::Instructions::EndChain:
			if(jitcontext.EntityChains.top()->empty())
			{
				Builder.SetInsertPoint(jitcontext.EntityChains.top());
				Builder.CreateBr(jitcontext.EntityChainExits.top());
			}

			jitcontext.EntityChecks.pop();
			jitcontext.EntityChains.pop();
			Builder.SetInsertPoint(jitcontext.EntityChainExits.top());
			jitcontext.EntityChainExits.pop();
			break;

		case Bytecode::Instructions::Assign:
			{
				Value* reftarget = jitcontext.ValuesOnStack.top();
				jitcontext.ValuesOnStack.pop();
				Value* v = jitcontext.ValuesOnStack.top();
				jitcontext.ValuesOnStack.pop();
				Builder.CreateStore(v, reftarget);
			}
			break;

		case Bytecode::Instructions::AssignSumType:
			{
				Value* reftarget = jitcontext.ValuesOnStack.top();
				jitcontext.ValuesOnStack.pop();
				Value* actualtype = jitcontext.ValuesOnStack.top();
				jitcontext.ValuesOnStack.pop();

				Value* typeholder = Builder.CreatePointerCast(reftarget, Type::getInt32PtrTy(Data->Context));
				if(actualtype->getType()->getNumContainedTypes() > 0)
				{
					LoadInst* load = dyn_cast<LoadInst>(actualtype);

					std::vector<Value*> gepindices;
					gepindices.push_back(ConstantInt::get(Type::getInt32Ty(Data->Context), 0));
					gepindices.push_back(ConstantInt::get(Type::getInt32Ty(Data->Context), 0));
					Builder.CreateStore(Builder.CreateLoad(Builder.CreateGEP(load->getOperand(0), gepindices)), typeholder);

					std::vector<Value*> payloadgepindices;
					payloadgepindices.push_back(ConstantInt::get(Type::getInt32Ty(Data->Context), 0));
					payloadgepindices.push_back(ConstantInt::get(Type::getInt32Ty(Data->Context), 1));
					Value* payloadgep = Builder.CreateGEP(load->getOperand(0), payloadgepindices);
					Value* payload = Builder.CreateLoad(payloadgep);

					Value* target = Builder.CreateGEP(Builder.CreatePointerCast(reftarget, Type::getInt8PtrTy(Data->Context)), ConstantInt::get(Type::getInt32Ty(Data->Context), sizeof(Metadata::EpochTypeID)));
					Value* casttarget = Builder.CreatePointerCast(target, payload->getType()->getPointerTo());
					Builder.CreateStore(payload, casttarget);
				}
				else
				{
					Builder.CreateStore(actualtype, typeholder);

					Value* target = Builder.CreateGEP(Builder.CreatePointerCast(reftarget, Type::getInt8PtrTy(Data->Context)), ConstantInt::get(Type::getInt32Ty(Data->Context), sizeof(Metadata::EpochTypeID)));
					Value* casttarget = Builder.CreatePointerCast(target, jitcontext.ValuesOnStack.top()->getType()->getPointerTo());
					Builder.CreateStore(jitcontext.ValuesOnStack.top(), casttarget);

					jitcontext.ValuesOnStack.pop();
				}
			}
			break;

		case Bytecode::Instructions::Read:
			{
				StringHandle varname = Fetch<StringHandle>(Bytecode, offset);
				size_t index = jitcontext.NameToIndexMap[varname];

				Value* v = NULL;

				if(index >= numparameters)
					v = Builder.CreateLoad(jitcontext.VariableMap[index]);
				else
					v = jitcontext.VariableMap[index];

				if(curscope->IsReferenceByID(varname))
					v = Builder.CreateLoad(v);

				jitcontext.ValuesOnStack.push(v);
			}
			break;

		case Bytecode::Instructions::ReadStack:
			{
				size_t frames = Fetch<size_t>(Bytecode, offset);
				size_t stackoffset = Fetch<size_t>(Bytecode, offset);
				Fetch<size_t>(Bytecode, offset);		// stack size is irrelevant

				if(frames != 0)
					throw NotImplementedException("Scope is not flat!");

				size_t index = offsettoindexmap[stackoffset];
				Value* val = Builder.CreateLoad(jitcontext.VariableMap[index]);
				jitcontext.ValuesOnStack.push(val);
			}
			break;

		case Bytecode::Instructions::ReadParam:
			{
				size_t frames = Fetch<size_t>(Bytecode, offset);
				size_t stackoffset = Fetch<size_t>(Bytecode, offset);
				Fetch<size_t>(Bytecode, offset);		// stack size is irrelevant

				if(frames != 0)
					throw NotImplementedException("Scope is not flat!");
				
				size_t idx = paramoffsettoindexmap[stackoffset];
				Value* val = jitcontext.VariableMap[idx];
				if(curscope->IsReference(idx))
					val = Builder.CreateLoad(val);
				jitcontext.ValuesOnStack.push(val);
			}
			break;

		case Bytecode::Instructions::BindRef:
			{
				size_t frames = Fetch<size_t>(Bytecode, offset);
				size_t index = Fetch<size_t>(Bytecode, offset);

				if(frames > 0)
					throw NotImplementedException("Scope is not flat!");

				Metadata::EpochTypeID vartype = curscope->GetVariableTypeByIndex(index);
				Value* ptr = jitcontext.VariableMap[index];
				jitcontext.ValuesOnStack.push(ptr);
				annotations.push(vartype);
			}
			break;

		case Bytecode::Instructions::BindMemberRef:
			{
				Metadata::EpochTypeID membertype = Fetch<Metadata::EpochTypeID>(Bytecode, offset);
				size_t memberoffset = Fetch<size_t>(Bytecode, offset);

				Value* voidstructptr = jitcontext.ValuesOnStack.top();
				if(voidstructptr->getType() == Type::getInt8PtrTy(Data->Context)->getPointerTo())
					voidstructptr = Builder.CreateLoad(voidstructptr);
				Value* bytestructptr = Builder.CreatePointerCast(voidstructptr, Type::getInt8PtrTy(Data->Context));
				Value* voidmemberptr = Builder.CreateGEP(bytestructptr, ConstantInt::get(Type::getInt32Ty(Data->Context), memberoffset));

				Value* memberptr = NULL;

				switch(membertype)
				{
				case Metadata::EpochType_Real:
					{
						Type* pfinaltype = Type::getFloatPtrTy(Data->Context);
						memberptr = Builder.CreatePointerCast(voidmemberptr, pfinaltype);
					}
					break;

				default:
					if(Metadata::GetTypeFamily(membertype) == Metadata::EpochTypeFamily_Structure || Metadata::GetTypeFamily(membertype) == Metadata::EpochTypeFamily_TemplateInstance)
					{
						Type* pfinaltype = Type::getInt8PtrTy(Data->Context)->getPointerTo();
						memberptr = Builder.CreatePointerCast(voidmemberptr, pfinaltype);
					}
					else if(Metadata::GetTypeFamily(membertype) == Metadata::EpochTypeFamily_SumType)
					{
						Type* pfinaltype = GetLLVMSumType(membertype, false)->getPointerTo();
						memberptr = Builder.CreatePointerCast(voidmemberptr, pfinaltype);
					}
					else
						throw NotImplementedException("I am lazy.");
				}

				jitcontext.ValuesOnStack.pop();
				jitcontext.ValuesOnStack.push(memberptr);
				annotations.push(membertype);
			}
			break;

		case Bytecode::Instructions::ReadRef:
			{
				Value* derefvalue = Builder.CreateLoad(jitcontext.ValuesOnStack.top());
				jitcontext.ValuesOnStack.pop();
				jitcontext.ValuesOnStack.push(derefvalue);
				annotations.pop();
			}
			break;

		case Bytecode::Instructions::ReadRefAnnotated:
			{
				std::vector<Value*> gepindices;
				gepindices.push_back(ConstantInt::get(Type::getInt32Ty(Data->Context), 0));
				gepindices.push_back(ConstantInt::get(Type::getInt32Ty(Data->Context), 0));
				Value* annotationgep = Builder.CreateGEP(jitcontext.ValuesOnStack.top(), gepindices);

				std::vector<Value*> derefindices;
				derefindices.push_back(ConstantInt::get(Type::getInt32Ty(Data->Context), 0));
				derefindices.push_back(ConstantInt::get(Type::getInt32Ty(Data->Context), 1));
				Value* derefgep = Builder.CreateLoad(Builder.CreateGEP(jitcontext.ValuesOnStack.top(), derefindices));

				Value* annotationvalue = Builder.CreateLoad(annotationgep);
				jitcontext.ValuesOnStack.pop();
				jitcontext.ValuesOnStack.push(derefgep);
				jitcontext.ValuesOnStack.push(annotationvalue);
				annotations.pop();
			}
			break;

		case Bytecode::Instructions::SetRetVal:
			{
				size_t index = Fetch<size_t>(Bytecode, offset);
				Builder.CreateStore(Builder.CreateLoad(jitcontext.VariableMap[index]), innerretval);
			}
			break;

		case Bytecode::Instructions::Return:
			Builder.CreateBr(innerfunctionexit);
			break;

		case Bytecode::Instructions::Invoke:
			{
				StringHandle target = Fetch<StringHandle>(Bytecode, offset);
				std::map<StringHandle, JIT::JITHelper>::const_iterator iter = OwnerVM.JITHelpers.InvokeHelpers.find(target);
				if(iter == OwnerVM.JITHelpers.InvokeHelpers.end())
					throw FatalException("Cannot invoke this function, no native code support!");

				iter->second(jitcontext, true);
			}
			break;

		case Bytecode::Instructions::InvokeOffset:
			{
				StringHandle functionname = Fetch<StringHandle>(Bytecode, offset);
				size_t internaloffset = Fetch<size_t>(Bytecode, offset);

				if(OwnerVM.TypeMatcherParamCount.find(functionname) == OwnerVM.TypeMatcherParamCount.end())
					throw FatalException("Cannot invoke VM code from native code");

				std::ostringstream matchername;
				matchername << "JITMatcher_" << internaloffset;

				std::vector<Type*> matchargtypes;
				matchargtypes.push_back(vmcontextptr->getType());
				for(size_t i = 0; i < OwnerVM.TypeMatcherParamCount.find(functionname)->second; ++i)
				{
					matchargtypes.push_back(Type::getInt32Ty(Data->Context));		// type annotation
					matchargtypes.push_back(Type::getInt8PtrTy(Data->Context));	// pointer to payload
				}


				Type* retty = Type::getVoidTy(Data->Context);
				if(OwnerVM.TypeMatcherRetType.find(functionname)->second != Metadata::EpochType_Error)
					retty = GetLLVMType(OwnerVM.TypeMatcherRetType.find(functionname)->second);
				FunctionType* matchfunctype = FunctionType::get(retty, matchargtypes, false);

				Function* nativetypematcher = Data->GeneratedNativeTypeMatchers[matchername.str()];
				if(!nativetypematcher)
				{
					nativetypematcher = Function::Create(matchfunctype, Function::ExternalLinkage, matchername.str().c_str(), Data->CurrentModule);
					Data->GeneratedNativeTypeMatchers[matchername.str()] = nativetypematcher;
				}

				std::vector<Value*> matchervarargs;
				size_t numparams = OwnerVM.TypeMatcherParamCount.find(functionname)->second;
				matchervarargs.push_back(jitcontext.InnerFunction->arg_begin());
				for(size_t i = 0; i < numparams; ++i)
				{
					Value* v1 = jitcontext.ValuesOnStack.top();
					jitcontext.ValuesOnStack.pop();

					Value* v2 = jitcontext.ValuesOnStack.top();
					jitcontext.ValuesOnStack.pop();

					if(v2->getType()->isPointerTy())
					{
						Metadata::EpochTypeID paramepochtype = annotations.top();
						Value* annotation = ConstantInt::get(Type::getInt32Ty(Data->Context), paramepochtype);
						annotations.pop();

						matchervarargs.push_back(annotation);
						matchervarargs.push_back(Builder.CreatePointerCast(v2, Type::getInt8PtrTy(Data->Context)));
					}
					else
					{
						LoadInst* load = dyn_cast<LoadInst>(v2);
						if(load)
						{
							matchervarargs.push_back(v1);
							matchervarargs.push_back(Builder.CreatePointerCast(load->getOperand(0), Type::getInt8PtrTy(Data->Context)));
						}
						else
						{
							Value* stacktemp = Builder.CreateAlloca(v2->getType());
							Builder.CreateStore(v2, stacktemp);

							matchervarargs.push_back(v1);
							matchervarargs.push_back(Builder.CreatePointerCast(stacktemp, Type::getInt8PtrTy(Data->Context)));
						}
					}
				}

				jitcontext.ValuesOnStack.push(Builder.CreateCall(nativetypematcher, matchervarargs));
			}
			break;

		case Bytecode::Instructions::InvokeNative:
			{
				StringHandle target = Fetch<StringHandle>(Bytecode, offset);
				Fetch<size_t>(Bytecode, offset);		// skip dummy offset
				std::map<StringHandle, JIT::JITHelper>::const_iterator iter = OwnerVM.JITHelpers.InvokeHelpers.find(target);
				if(iter != OwnerVM.JITHelpers.InvokeHelpers.end())
					iter->second(jitcontext, true);
				else
				{
					FunctionType* targetfunctype = GetLLVMFunctionType(target);

					std::ostringstream name;
					name << "JITFuncInner_" << OwnerVM.GetFunctionInstructionOffsetNoThrow(target);
					Function* targetfunc;
					if(Data->GeneratedFunctions[name.str()])
						targetfunc = Data->GeneratedFunctions[name.str()];
					else
					{
						targetfunc = Function::Create(targetfunctype, Function::InternalLinkage, name.str().c_str(), Data->CurrentModule);
						Data->GeneratedFunctions[name.str()] = targetfunc;
					}

					std::vector<Value*> targetargs;
					for(size_t i = 1; i < targetfunctype->getNumParams(); ++i)
					{
						Value* p = jitcontext.ValuesOnStack.top();
						jitcontext.ValuesOnStack.pop();

						targetargs.push_back(p);
					}
					targetargs.push_back(jitcontext.InnerFunction->arg_begin());
					std::reverse(targetargs.begin(), targetargs.end());

					jitcontext.ValuesOnStack.push(Builder.CreateCall(targetfunc, targetargs));
				}
			}
			break;

		case Bytecode::Instructions::Push:
			{
				Metadata::EpochTypeID type = Fetch<Metadata::EpochTypeID>(Bytecode, offset);
				Constant* valueval;

				switch(type)
				{
				case Metadata::EpochType_Integer:
					{
						Integer32 value = Fetch<Integer32>(Bytecode, offset);
						valueval = ConstantInt::get(Type::getInt32Ty(Data->Context), value);
					}
					break;

				case Metadata::EpochType_Identifier:
				case Metadata::EpochType_String:
					{
						StringHandle value = Fetch<StringHandle>(Bytecode, offset);
						valueval = ConstantInt::get(Type::getInt32Ty(Data->Context), value);
					}
					break;

				case Metadata::EpochType_Real:
					{
						Real32 value = Fetch<Real32>(Bytecode, offset);
						valueval = ConstantFP::get(Type::getFloatTy(Data->Context), value);
					}
					break;

				case Metadata::EpochType_Boolean:
					{
						bool value = Fetch<bool>(Bytecode, offset);
						valueval = ConstantInt::get(Type::getInt1Ty(Data->Context), value);
					}
					break;

				case Metadata::EpochType_Integer16:
				case Metadata::EpochType_Buffer:
				default:
					throw FatalException("Unsupported type for JIT compilation");
				}

				jitcontext.ValuesOnStack.push(valueval);
			}
			break;

		case Bytecode::Instructions::Pop:
			jitcontext.ValuesOnStack.pop();
			break;

		case Bytecode::Instructions::TypeMatch:
			{
				std::ostringstream nextname;
				nextname << "nexttypematcher" << ++typematchindex;
				BasicBlock* nexttypematcher = BasicBlock::Create(Data->Context, nextname.str(), jitbridgefunction);

				Value* providedtype = Builder.CreateAlloca(Type::getInt32Ty(Data->Context), NULL, "providedtype");
				Value* initialstackptr = Builder.CreateLoad(Builder.CreateLoad(pstackptr));
				Value* stackoffsettracker = Builder.CreateAlloca(Type::getInt32Ty(Data->Context), NULL, "stackoffset");

				typematchconsumedbytes = Builder.CreateAlloca(Type::getInt32Ty(Data->Context)), NULL, "consumedbytes";
				Builder.CreateStore(ConstantInt::get(Type::getInt32Ty(Data->Context), 0), stackoffsettracker);

				StringHandle funcname = Fetch<StringHandle>(Bytecode, offset);
				size_t matchoffset = Fetch<size_t>(Bytecode, offset);
				size_t paramcount = Fetch<size_t>(Bytecode, offset);

				std::vector<Value*> actualparams;

				for(size_t i = 0; i < paramcount; ++i)
				{
					bool expectref = Fetch<bool>(Bytecode, offset);
					Metadata::EpochTypeID expecttype = Fetch<Metadata::EpochTypeID>(Bytecode, offset);

					BasicBlock* checkmatchblock = BasicBlock::Create(Data->Context, "checkmatch", jitbridgefunction);

					Value* parampayloadptr = Builder.CreateAlloca(Type::getInt8PtrTy(Data->Context), NULL, "parampayloadptr");

					if(!expectref)
					{
						Value* lateststackptr = Builder.CreateGEP(initialstackptr, Builder.CreateLoad(stackoffsettracker));
						Value* gettype = Builder.CreateLoad(Builder.CreatePointerCast(lateststackptr, Type::getInt32PtrTy(Data->Context)));
						Builder.CreateStore(gettype, providedtype);

						// TODO - support undecomposed sum types
						/*
						while(GetTypeFamily(providedtype) == EpochTypeFamily_SumType)
						{
							stackptr += sizeof(EpochTypeID);
							providedtype = *reinterpret_cast<const EpochTypeID*>(stackptr);
						}
						*/

						Value* providedrefflag = Builder.CreateICmpEQ(gettype, ConstantInt::get(Type::getInt32Ty(Data->Context), Metadata::EpochType_RefFlag));

						BasicBlock* providedrefblock = BasicBlock::Create(Data->Context, "providedref", jitbridgefunction);
						BasicBlock* providedvalueblock = BasicBlock::Create(Data->Context, "providedvalue", jitbridgefunction);
						Builder.CreateCondBr(providedrefflag, providedrefblock, providedvalueblock);

						Builder.SetInsertPoint(providedrefblock);
						Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(Data->Context), sizeof(Metadata::EpochTypeID))), stackoffsettracker);

						Value* providednothingflag = Builder.CreateICmpEQ(gettype, ConstantInt::get(Type::getInt32Ty(Data->Context), Metadata::EpochType_Nothing));
						Value* expectnothing = (expecttype == Metadata::EpochType_Nothing ? ConstantInt::get(Type::getInt1Ty(Data->Context), 1) : ConstantInt::get(Type::getInt1Ty(Data->Context), 0));
						Value* provideandexpectnothing = Builder.CreateAnd(providednothingflag, expectnothing);

						BasicBlock* handlesomethingblock = BasicBlock::Create(Data->Context, "handlesomething", jitbridgefunction);
						BasicBlock* handlenothingblock = BasicBlock::Create(Data->Context, "handlenothing", jitbridgefunction);

						Builder.CreateCondBr(provideandexpectnothing, handlenothingblock, handlesomethingblock);
						Builder.SetInsertPoint(handlenothingblock);
						Builder.CreateStore(ConstantInt::get(Type::getInt32Ty(Data->Context), Metadata::EpochType_Nothing), providedtype);
						Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(Data->Context), sizeof(void*))), stackoffsettracker);
						Builder.CreateBr(checkmatchblock);

						Builder.SetInsertPoint(handlesomethingblock);
						BasicBlock* handlerefblock = BasicBlock::Create(Data->Context, "handleref", jitbridgefunction);
						Builder.CreateCondBr(providednothingflag, nexttypematcher, handlerefblock);
						Builder.SetInsertPoint(handlerefblock);

						Value* reftarget = Builder.CreateGEP(initialstackptr, Builder.CreateLoad(stackoffsettracker));
						Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(Data->Context), sizeof(void*))), stackoffsettracker);

						Value* reftype = Builder.CreateLoad(Builder.CreatePointerCast(Builder.CreateGEP(initialstackptr, Builder.CreateLoad(stackoffsettracker)), Type::getInt32PtrTy(Data->Context)));
						Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(Data->Context), sizeof(Metadata::EpochTypeID))), stackoffsettracker);

						Value* refisnothingflag = Builder.CreateICmpEQ(reftype, ConstantInt::get(Type::getInt32Ty(Data->Context), Metadata::EpochType_Nothing));
						Value* refandexpectnothing = Builder.CreateAnd(expectnothing, refisnothingflag);

						BasicBlock* handlenothingrefblock = BasicBlock::Create(Data->Context, "handlenothingref", jitbridgefunction);
						BasicBlock* checksumtypeblock = BasicBlock::Create(Data->Context, "checksumtype", jitbridgefunction);
						Builder.CreateCondBr(refandexpectnothing, handlenothingrefblock, checksumtypeblock);

						Builder.SetInsertPoint(handlenothingrefblock);
						Builder.CreateStore(ConstantInt::get(Type::getInt32Ty(Data->Context), Metadata::EpochType_Nothing), providedtype);
						Builder.CreateBr(checkmatchblock);

						BasicBlock* handlesumtypeblock = BasicBlock::Create(Data->Context, "handlesumtype", jitbridgefunction);

						Builder.SetInsertPoint(checksumtypeblock);
						Value* providedtypefamily = Builder.CreateAnd(gettype, 0xff000000);
						Value* issumtype = Builder.CreateICmpEQ(providedtypefamily, ConstantInt::get(Type::getInt32Ty(Data->Context), Metadata::EpochTypeFamily_SumType));
						Builder.CreateCondBr(issumtype, handlesumtypeblock, nexttypematcher);

						Builder.SetInsertPoint(handlesumtypeblock);
						Value* reftypeptr = Builder.CreateGEP(reftarget, ConstantInt::get(Type::getInt32Ty(Data->Context), static_cast<uint64_t>(-(signed)sizeof(Metadata::EpochTypeID))));
						reftype = Builder.CreateLoad(Builder.CreatePointerCast(reftypeptr, Type::getInt32PtrTy(Data->Context)));

						BasicBlock* handlereftonothingblock = BasicBlock::Create(Data->Context, "handlereftonothing", jitbridgefunction);
						refisnothingflag = Builder.CreateICmpEQ(reftype, ConstantInt::get(Type::getInt32Ty(Data->Context), Metadata::EpochType_Nothing));
						refandexpectnothing = Builder.CreateAnd(expectnothing, refisnothingflag);
						Builder.CreateCondBr(refandexpectnothing, handlereftonothingblock, nexttypematcher);

						Builder.SetInsertPoint(handlereftonothingblock);
						Builder.CreateStore(reftype, providedtype);
						Builder.CreateBr(checkmatchblock);

						Builder.SetInsertPoint(providedvalueblock);
						Builder.CreateStore(Builder.CreateLoad(Builder.CreatePointerCast(Builder.CreateGEP(initialstackptr, Builder.CreateLoad(stackoffsettracker)), Type::getInt32PtrTy(Data->Context))), providedtype);
						Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(Data->Context), sizeof(Metadata::EpochTypeID))), stackoffsettracker);
						Builder.CreateStore(Builder.CreateGEP(initialstackptr, Builder.CreateLoad(stackoffsettracker)), parampayloadptr);
						Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(Data->Context), Metadata::GetStorageSize(expecttype))), stackoffsettracker);
						Builder.CreateBr(checkmatchblock);
					}
					else
					{
						Value* magic = Builder.CreateLoad(Builder.CreatePointerCast(Builder.CreateGEP(initialstackptr, Builder.CreateLoad(stackoffsettracker)), Type::getInt32PtrTy(Data->Context)), true);
						Value* providedrefflag = Builder.CreateICmpEQ(magic, ConstantInt::get(Type::getInt32Ty(Data->Context), Metadata::EpochType_RefFlag));

						BasicBlock* providedrefblock = BasicBlock::Create(Data->Context, "providedexpectedref", jitbridgefunction);
						Builder.CreateCondBr(providedrefflag, providedrefblock, nexttypematcher);
						Builder.SetInsertPoint(providedrefblock);

						Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(Data->Context), sizeof(Metadata::EpochTypeID))), stackoffsettracker);

						Value* reftarget = Builder.CreateLoad(Builder.CreatePointerCast(Builder.CreateGEP(initialstackptr, Builder.CreateLoad(stackoffsettracker)), PointerType::get(Type::getInt8PtrTy(Data->Context), 0)));
						Builder.CreateStore(reftarget, parampayloadptr);
						Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(Data->Context), sizeof(void*))), stackoffsettracker);

						Value* reftype = Builder.CreateLoad(Builder.CreatePointerCast(Builder.CreateGEP(initialstackptr, Builder.CreateLoad(stackoffsettracker)), Type::getInt32PtrTy(Data->Context)));
						Builder.CreateStore(reftype, providedtype);

						BasicBlock* setnothingrefflagblock = BasicBlock::Create(Data->Context, "setnothingrefflag", jitbridgefunction);
						BasicBlock* skipblock = BasicBlock::Create(Data->Context, "skip", jitbridgefunction);

						Value* providednothingflag = Builder.CreateICmpEQ(reftype, ConstantInt::get(Type::getInt32Ty(Data->Context), Metadata::EpochType_Nothing));
						Builder.CreateCondBr(providednothingflag, setnothingrefflagblock, skipblock);

						Builder.SetInsertPoint(setnothingrefflagblock);
						Builder.CreateBr(skipblock);

						Builder.SetInsertPoint(skipblock);
						Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(Data->Context), sizeof(Metadata::EpochTypeID))), stackoffsettracker);


						BasicBlock* handlesumtypeblock = BasicBlock::Create(Data->Context, "handlesumtype", jitbridgefunction);

						Value* providedtypefamily = Builder.CreateAnd(Builder.CreateLoad(providedtype), 0xff000000);
						Value* issumtype = Builder.CreateICmpEQ(providedtypefamily, ConstantInt::get(Type::getInt32Ty(Data->Context), Metadata::EpochTypeFamily_SumType));
						Builder.CreateCondBr(issumtype, handlesumtypeblock, checkmatchblock);

						Builder.SetInsertPoint(handlesumtypeblock);
						Value* reftypeptr = Builder.CreateGEP(reftarget, ConstantInt::get(Type::getInt32Ty(Data->Context), static_cast<uint64_t>(-(signed)sizeof(Metadata::EpochTypeID))));
						reftype = Builder.CreateLoad(Builder.CreatePointerCast(reftypeptr, Type::getInt32PtrTy(Data->Context)));
						Builder.CreateStore(reftype, providedtype);
						Builder.CreateStore(reftarget, parampayloadptr);
						Builder.CreateBr(checkmatchblock);
					}

					Builder.SetInsertPoint(checkmatchblock);

					Value* nomatch = Builder.CreateICmpNE(Builder.CreateLoad(providedtype), ConstantInt::get(Type::getInt32Ty(Data->Context), expecttype));
					Value* notexpectsumtype = ConstantInt::get(Type::getInt1Ty(Data->Context), Metadata::GetTypeFamily(expecttype) != Metadata::EpochTypeFamily_SumType);

					BasicBlock* setflagsblock = BasicBlock::Create(Data->Context, "setflags", jitbridgefunction);
					BasicBlock* nextparamblock = BasicBlock::Create(Data->Context, "nextparam", jitbridgefunction);

					Value* bailflag = Builder.CreateAnd(nomatch, notexpectsumtype);
					Builder.CreateCondBr(bailflag, nexttypematcher, setflagsblock);

					Builder.SetInsertPoint(setflagsblock);
					BasicBlock* moreflagsblock = BasicBlock::Create(Data->Context, "moreflags", jitbridgefunction);
					Builder.CreateBr(moreflagsblock);
					
					Builder.SetInsertPoint(moreflagsblock);
					BasicBlock* setnothingflagblock = BasicBlock::Create(Data->Context, "setnothingflag", jitbridgefunction);
					Value* providednothingflag = Builder.CreateICmpEQ(Builder.CreateLoad(providedtype), ConstantInt::get(Type::getInt32Ty(Data->Context), Metadata::EpochType_Nothing));
					Builder.CreateCondBr(providednothingflag, setnothingflagblock, nextparamblock);
					Builder.SetInsertPoint(setnothingflagblock);
					Builder.CreateBr(nextparamblock);

					Builder.SetInsertPoint(nextparamblock);

					if(expecttype != Metadata::EpochType_Nothing)
					{
						if(expectref)
						{
							Value* actualparam = Builder.CreatePointerCast(Builder.CreateLoad(parampayloadptr), GetLLVMType(expecttype)->getPointerTo());
							actualparams.push_back(actualparam);
						}
						else
						{
							Value* actualparam = Builder.CreatePointerCast(Builder.CreateLoad(parampayloadptr), GetLLVMType(expecttype)->getPointerTo());
							actualparams.push_back(Builder.CreateLoad(actualparam));
						}
					}
					else
						actualparams.push_back(ConstantInt::get(Type::getInt32Ty(Data->Context), 0));
				}

				BasicBlock* invokeblock = BasicBlock::Create(Data->Context, "invokesuccess", jitbridgefunction);
				Builder.CreateBr(invokeblock);

				Builder.SetInsertPoint(invokeblock);

				std::ostringstream matchname;
				matchname << "JITFuncInner_" << matchoffset;

				Function* targetinnerfunc = NULL;
				FunctionType* targetinnerfunctype = GetLLVMFunctionType(funcname);
				if(!Data->GeneratedFunctions[matchname.str()])
				{
					targetinnerfunc = Function::Create(targetinnerfunctype, Function::InternalLinkage, matchname.str().c_str(), Data->CurrentModule);
					Data->GeneratedFunctions[matchname.str()] = targetinnerfunc;
				}
				else
					targetinnerfunc = Data->GeneratedFunctions[matchname.str()];

				std::vector<Value*> resolvedargs;
				resolvedargs.push_back(vmcontextptr);
				for(std::vector<Value*>::const_reverse_iterator argiter = actualparams.rbegin(); argiter != actualparams.rend(); ++argiter)
					resolvedargs.push_back(*argiter);

				typematchret = Builder.CreateCall(targetinnerfunc, resolvedargs);

				Builder.CreateStore(Builder.CreateLoad(stackoffsettracker), typematchconsumedbytes);

				// Fixup VM stack with return value
				Type* matchrettype = typematchret->getType();
				if(matchrettype != Type::getVoidTy(Data->Context))
				{
					Value* offset = Builder.CreateSub(Builder.CreateLoad(typematchconsumedbytes), ConstantInt::get(Type::getInt32Ty(Data->Context), 4));
					Value* stackptr = Builder.CreateLoad(Builder.CreateLoad(pstackptr));
					Value* retstackptr = Builder.CreateGEP(stackptr, offset);
					Builder.CreateStore(typematchret, Builder.CreatePointerCast(retstackptr, matchrettype->getPointerTo()));
					Builder.CreateStore(retstackptr, Builder.CreateLoad(pstackptr));
				}
				else
				{
					Value* offset = Builder.CreateLoad(typematchconsumedbytes);
					Value* stackptr = Builder.CreateLoad(Builder.CreateLoad(pstackptr));
					Value* retstackptr = Builder.CreateGEP(stackptr, offset);
					Builder.CreateStore(retstackptr, Builder.CreateLoad(pstackptr));
				}
				
				Builder.CreateRetVoid();
				Builder.SetInsertPoint(nexttypematcher);
			}
			break;

		case Bytecode::Instructions::Halt:
			Builder.CreateCall(Data->VMFunctions[VMHalt]);
			Builder.CreateUnreachable();
			break;

		case Bytecode::Instructions::AllocStructure:
			{
				Metadata::EpochTypeID type = Fetch<Metadata::EpochTypeID>(Bytecode, offset);
				Value* typeconst = ConstantInt::get(Type::getInt32Ty(Data->Context), type);
				Value* handle = Builder.CreateCall2(Data->VMFunctions[VMAllocStruct], jitcontext.InnerFunction->arg_begin(), typeconst);
				jitcontext.ValuesOnStack.push(handle);

				hackstructtype = type;
			}
			break;

		case Bytecode::Instructions::ConstructSumType:
			{
				Value* vartype = jitcontext.ValuesOnStack.top();
				jitcontext.ValuesOnStack.pop();

				Value* value = jitcontext.ValuesOnStack.top();
				jitcontext.ValuesOnStack.pop();

				Value* targetid = jitcontext.ValuesOnStack.top();
				jitcontext.ValuesOnStack.pop();

				llvm::ConstantInt* cint = llvm::dyn_cast<llvm::ConstantInt>(targetid);
				size_t vartarget = static_cast<size_t>(cint->getValue().getLimitedValue());

				Value* storagetarget = jitcontext.VariableMap[jitcontext.NameToIndexMap[vartarget]];

				// Set contents
				{
					std::vector<Value*> memberindices;
					memberindices.push_back(ConstantInt::get(Type::getInt32Ty(Data->Context), 0));
					memberindices.push_back(ConstantInt::get(Type::getInt32Ty(Data->Context), 1));
					Value* valueholder = Builder.CreateGEP(storagetarget, memberindices);
					Builder.CreateStore(value, valueholder);
				}

				// Set type annotation
				{
					std::vector<Value*> memberindices;
					memberindices.push_back(ConstantInt::get(Type::getInt32Ty(Data->Context), 0));
					memberindices.push_back(ConstantInt::get(Type::getInt32Ty(Data->Context), 0));
					Value* typeholder = Builder.CreateGEP(storagetarget, memberindices);
					Builder.CreateStore(vartype, typeholder);
				}
			}
			break;

		case Bytecode::Instructions::CopyToStructure:
			{
				StringHandle variablename = Fetch<StringHandle>(Bytecode, offset);
				StringHandle actualmember = Fetch<StringHandle>(Bytecode, offset);

				Value* v = jitcontext.VariableMap[jitcontext.NameToIndexMap[variablename]];
				Value* structptr = Builder.CreateLoad(v);

				const StructureDefinition& def = OwnerVM.GetStructureDefinition(hackstructtype);
				size_t memberindex = def.FindMember(actualmember);
				size_t memberoffset = def.GetMemberOffset(memberindex);
				Metadata::EpochTypeID membertype = def.GetMemberType(memberindex);

				Value* memberptr = Builder.CreateGEP(structptr, ConstantInt::get(Type::getInt32Ty(Data->Context), memberoffset));
				Value* castmemberptr = Builder.CreatePointerCast(memberptr, GetLLVMType(membertype)->getPointerTo());
				
				Builder.CreateStore(jitcontext.ValuesOnStack.top(), castmemberptr);

				jitcontext.ValuesOnStack.pop();
			}
			break;

			case Bytecode::Instructions::CopyStructure:
			{
				Value* structureptr = jitcontext.ValuesOnStack.top();
				jitcontext.ValuesOnStack.pop();
				Value* copyptr = Builder.CreateCall2(Data->VMFunctions[VMCopyStruct], jitcontext.InnerFunction->arg_begin(), structureptr);
				Value* castptr = Builder.CreatePointerCast(copyptr, Type::getInt8PtrTy(Data->Context));
				jitcontext.ValuesOnStack.push(castptr);
			}
			break;


		default:
			throw FatalException("Unsupported instruction for JIT compilation");
		}
	}
	
	if(jitcontext.InnerFunction)
	{
		Builder.SetInsertPoint(innerfunctionexit);
		if(innerretval)
		{
			Value* ret = Builder.CreateLoad(innerretval);
			Builder.CreateRet(ret);
		}
		else
			Builder.CreateRetVoid();
	}

	Builder.SetInsertPoint(block);
	if(!typematchindex)
		Builder.CreateBr(outerfunctionexit);

	Builder.SetInsertPoint(outerfunctionexit);

	LoadInst* stackptr = Builder.CreateLoad(Builder.CreateLoad(pstackptr));
	Constant* offset = ConstantInt::get(Type::getInt32Ty(Data->Context), static_cast<unsigned>(numparambytes - (numreturns * 4)));	// TODO - change to retbytes
	Value* stackptr2 = Builder.CreateGEP(stackptr, offset);
	if(retval)
	{
		Value* ret = Builder.CreateLoad(retval);
		Builder.CreateStore(ret, Builder.CreatePointerCast(stackptr2, PointerType::get(ret->getType(), 0)));
	}
	Builder.CreateStore(stackptr2, Builder.CreateLoad(pstackptr));

	Builder.CreateRetVoid();


	if(nativematchblock)
	{
		Builder.SetInsertPoint(nativematchblock);
		AddNativeTypeMatcher(beginoffset, endoffset);
	}

	std::string ErrStr;
	ExecutionEngine* ee = EngineBuilder(Data->CurrentModule).setErrorStr(&ErrStr).create();
	if(!ee)
		return;

	Data->GeneratedBridgeFunctions[alias] = jitbridgefunction;
}

void NativeCodeGenerator::Generate()
{
	using namespace llvm;

	//Data->CurrentModule->dump();
	verifyModule(*Data->CurrentModule, llvm::PrintMessageAction);

#ifdef _DEBUG
	//EnableStatistics();
#endif

	std::string ErrStr;
	ExecutionEngine* ee = EngineBuilder(Data->CurrentModule).setErrorStr(&ErrStr).create();
	if(!ee)
		return;
	
	FunctionPassManager OurFPM(Data->CurrentModule);

	OurFPM.add(new DataLayout(*ee->getDataLayout()));
	OurFPM.add(createTypeBasedAliasAnalysisPass());
	OurFPM.add(createBasicAliasAnalysisPass());
	OurFPM.add(createCFGSimplificationPass());
	OurFPM.add(createScalarReplAggregatesPass());
	OurFPM.add(createEarlyCSEPass());
	OurFPM.add(createLowerExpectIntrinsicPass());

	VectorizeConfig vcfg;
	vcfg.ReqChainDepth = 2;
	vcfg.MaxIter = 500;
	OurFPM.add(createBBVectorizePass(vcfg));

	OurFPM.doInitialization();

	for(std::map<StringHandle, Function*>::const_iterator iter = Data->GeneratedBridgeFunctions.begin(); iter != Data->GeneratedBridgeFunctions.end(); ++iter)
		OurFPM.run(*iter->second);

	PassManager OurMPM;
	OurFPM.add(new DataLayout(*ee->getDataLayout()));
	OurMPM.add(createTypeBasedAliasAnalysisPass());
	OurMPM.add(createBasicAliasAnalysisPass());
	OurMPM.add(createGlobalOptimizerPass());
	OurMPM.add(createPromoteMemoryToRegisterPass());
	OurMPM.add(createIPSCCPPass());
	OurMPM.add(createDeadArgEliminationPass());
	OurMPM.add(createInstructionCombiningPass());
	OurMPM.add(createCFGSimplificationPass());
	OurMPM.add(createPruneEHPass());
	OurMPM.add(createFunctionAttrsPass());
	OurMPM.add(createFunctionInliningPass());
	OurMPM.add(createArgumentPromotionPass());
	OurMPM.add(createScalarReplAggregatesPass(-1, false));
	OurMPM.add(createEarlyCSEPass());
	OurMPM.add(createSimplifyLibCallsPass());
	OurMPM.add(createJumpThreadingPass());
	OurMPM.add(createCorrelatedValuePropagationPass());
	OurMPM.add(createCFGSimplificationPass());
	OurMPM.add(createInstructionCombiningPass());
	OurMPM.add(createTailCallEliminationPass());
	OurMPM.add(createCFGSimplificationPass());
	OurMPM.add(createReassociatePass());
	OurMPM.add(createLoopRotatePass());
	OurMPM.add(createLICMPass());
	OurMPM.add(createLoopUnswitchPass(false));
	OurMPM.add(createInstructionCombiningPass());
	OurMPM.add(createIndVarSimplifyPass());
	OurMPM.add(createLoopIdiomPass());
	OurMPM.add(createLoopDeletionPass());
	OurMPM.add(createLoopUnrollPass());
	OurMPM.add(createGVNPass());
	OurMPM.add(createMemCpyOptPass());
	OurMPM.add(createSCCPPass());
	OurMPM.add(createInstructionCombiningPass());
	OurMPM.add(createJumpThreadingPass());
	OurMPM.add(createCorrelatedValuePropagationPass());
	OurMPM.add(createDeadStoreEliminationPass());
	OurMPM.add(createAggressiveDCEPass());
	OurMPM.add(createCFGSimplificationPass());
	OurMPM.add(createInstructionCombiningPass());
	OurMPM.add(createFunctionInliningPass());

	OurMPM.run(*Data->CurrentModule);

	//Data->CurrentModule->dump();

	for(std::map<StringHandle, Function*>::const_iterator iter = Data->GeneratedBridgeFunctions.begin(); iter != Data->GeneratedBridgeFunctions.end(); ++iter)
	{
		void* fptr = ee->getPointerToFunction(iter->second);
		OwnerVM.JITExecs[iter->first] = reinterpret_cast<EpochToJITWrapperFunc>(fptr);
	}

	PrintStatistics();

	CrashRecoveryContext::Disable();
}

//
// Construct and initialize a native code generation wrapper
//
NativeCodeGenerator::NativeCodeGenerator(VM::VirtualMachine& ownervm, const Bytecode::Instruction* bytecode)
	: OwnerVM(ownervm),
	  Bytecode(bytecode),
	  Data(new LLVMData),
	  Builder(Data->Context)
{
	using namespace llvm;

	InitializeNativeTarget();
}

//
// Destruct and clean up a native code generation wrapper
//
NativeCodeGenerator::~NativeCodeGenerator()
{
	delete Data;
}


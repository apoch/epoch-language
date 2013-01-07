//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Just-in-time native code generation for Epoch
//

// TODO - improve documentation of JITter

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


// We heavily use this namespace so may as well import it
using namespace llvm;



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

		enum IntrinsicFunc
		{
			IntrinsicSqrt,
		};


		struct LLVMData
		{
			LLVMData();

			LLVMContext& Context;

			Module* CurrentModule;

			Type* VMTypeIDType;
			Type* VMContextPtrType;
			Type* VMBufferHandleType;
			Type* VMStructureHandleType;
			Type* VMNothingType;
			
			std::map<VMInteropFunc, Function*> VMFunctions;
			std::map<IntrinsicFunc, Function*> Intrinsics;

			std::map<std::string, Function*> GeneratedFunctions;
			std::map<std::string, Function*> GeneratedNativeTypeMatchers;
			std::map<StringHandle, Function*> GeneratedBridgeFunctions;

			std::map<Metadata::EpochTypeID, StructType*> SumTypeCache;

		// Non-copyable
		private:
			LLVMData(const LLVMData&);
			LLVMData& operator = (const LLVMData&);
		};


		class FunctionJITHelper
		{
		// Construction
		public:
			explicit FunctionJITHelper(NativeCodeGenerator& generator);

		// Non-copyable
		private:
			FunctionJITHelper(const FunctionJITHelper&);
			FunctionJITHelper& operator = (const FunctionJITHelper&);

		// Function JIT interface
		public:
			void DoFunction(size_t beginoffset, size_t endoffset);

		// Instruction handlers
		private:
			void BeginEntity(size_t& offset);
			void EndEntity(size_t& offset);

			void BeginChain(size_t& offset);
			void EndChain(size_t& offset);

			void Read(size_t& offset);
			void ReadStackLocal(size_t& offset);
			void ReadParameter(size_t& offset);

			void BindReference(size_t& offset);
			void ReadRef(size_t& offset);
			void ReadRefAnnotated(size_t& offset);

			void Assign(size_t& offset);
			void AssignSumType(size_t& offset);
			void ConstructSumType(size_t& offset);

			void SetRetValue(size_t& offset);
			void Return(size_t& offset);
			void Halt(size_t& offset);

			void Push(size_t& offset);
			void Pop(size_t& offset);

			void AllocStructure(size_t& offset);
			void CopyToStructure(size_t& offset);
			void CopyStructure(size_t& offset);
			void BindMemberRef(size_t& offset);

			void Invoke(size_t& offset);
			void InvokeOffset(size_t& offset);
			void InvokeNative(size_t& offset);

			void TypeMatch(size_t& offset);

		// Accessible tracking
		public:
			Function* BridgeFunction;

		// Internal tracking
		private:
			NativeCodeGenerator& Generator;
			const Bytecode::Instruction* Bytecode;

			IRBuilder<>& Builder;
			LLVMContext& Context;

			JITContext LibJITContext;

			Type* StackPtrType;
			Type* PStackPtrType;

			BasicBlock* BridgeEntryBlock;
			BasicBlock* BridgeExitBlock;

			BasicBlock* InnerExitBlock;
			BasicBlock* NativeMatchBlock;

			Value* PStackPtr;
			Value* VMContextPtr;

			Value* OuterRetVal;
			Value* InnerRetVal;
			Value* TypeMatchRetVal;
			Value* TypeMatchConsumedBytes;

			size_t BeginOffset;
			size_t EndOffset;

			const ScopeDescription* CurrentScope;

			unsigned NumParameters;
			unsigned NumParameterBytes;
			unsigned NumReturns;
			unsigned TypeMatchIndex;

			Metadata::EpochTypeID HackStructType;

			std::stack<Metadata::EpochTypeID> TypeAnnotations;
			std::map<size_t, size_t> LocalOffsetToIndexMap;
			std::map<size_t, size_t> ParameterOffsetToIndexMap;

			typedef void (FunctionJITHelper::* InstructionJITHelper)(size_t& offset);
			std::map<Bytecode::Instruction, InstructionJITHelper> InstructionJITHelpers;
		};


		//
		// Construct and initialize an LLVM data wrapper object
		//
		LLVMData::LLVMData() :
			Context(getGlobalContext())
		{
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

			// Set up intrinsics
			{
				std::vector<Type*> args;
				args.push_back(Type::getFloatTy(Context));
				FunctionType* ftype = FunctionType::get(Type::getFloatTy(Context), args, false);
				Intrinsics[IntrinsicSqrt] = Function::Create(ftype, Function::ExternalLinkage, "llvm.sqrt.f32", CurrentModule);
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
// Construct and initialize a native code generation wrapper
//
NativeCodeGenerator::NativeCodeGenerator(VM::VirtualMachine& ownervm, const Bytecode::Instruction* bytecode)
	: OwnerVM(ownervm),
	  Bytecode(bytecode),
	  Data(new LLVMData),
	  Builder(Data->Context)
{
	InitializeNativeTarget();
}

//
// Destruct and clean up a native code generation wrapper
//
NativeCodeGenerator::~NativeCodeGenerator()
{
	delete Data;
}


//
// Retrieve (or create if necessary) a generated function
//
Function* NativeCodeGenerator::GetGeneratedFunction(StringHandle funcname, size_t beginoffset)
{
	std::ostringstream name;
	name << "JITFuncInner_" << beginoffset;

	Function* targetinnerfunc = NULL;
	FunctionType* targetinnerfunctype = GetLLVMFunctionType(funcname);
	if(!Data->GeneratedFunctions[name.str()])
	{
		targetinnerfunc = Function::Create(targetinnerfunctype, Function::InternalLinkage, name.str().c_str(), Data->CurrentModule);
		Data->GeneratedFunctions[name.str()] = targetinnerfunc;
	}
	else
		targetinnerfunc = Data->GeneratedFunctions[name.str()];

	return targetinnerfunc;
}

//
// Retrieve (or create if necessary) a generated type matcher
//
Function* NativeCodeGenerator::GetGeneratedTypeMatcher(StringHandle funcname, size_t beginoffset)
{
	std::ostringstream matchername;
	matchername << "JITMatcher_" << beginoffset;

	Function* nativetypematcher = Data->GeneratedNativeTypeMatchers[matchername.str()];
	if(!nativetypematcher)
	{
		std::vector<Type*> matchargtypes;
		matchargtypes.push_back(Data->VMContextPtrType);
		for(size_t i = 0; i < OwnerVM.TypeMatcherParamCount.find(funcname)->second; ++i)
		{
			matchargtypes.push_back(Data->VMTypeIDType);					// type annotation
			matchargtypes.push_back(Type::getInt8PtrTy(Data->Context));		// pointer to payload
		}

		Type* retty = Type::getVoidTy(Data->Context);
		if(OwnerVM.TypeMatcherRetType.find(funcname)->second != Metadata::EpochType_Error)
			retty = GetLLVMType(OwnerVM.TypeMatcherRetType.find(funcname)->second);
		FunctionType* matchfunctype = FunctionType::get(retty, matchargtypes, false);

		nativetypematcher = Function::Create(matchfunctype, Function::ExternalLinkage, matchername.str().c_str(), Data->CurrentModule);
		Data->GeneratedNativeTypeMatchers[matchername.str()] = nativetypematcher;
	}

	return nativetypematcher;
}

//
// Retrieve (or create, if necessary) a generated Epoch-to-native bridge function
//
Function* NativeCodeGenerator::GetGeneratedBridge(size_t beginoffset)
{
	std::ostringstream name;
	name << "JITFunc_" << beginoffset;

	Function* jitbridgefunction = Data->GeneratedFunctions[name.str()];
	if(!jitbridgefunction)
	{
		std::vector<Type*> args;
		args.push_back(Type::getInt8PtrTy(Data->Context)->getPointerTo());				// Epoch stack pointer pointer
		args.push_back(Data->VMContextPtrType);
		FunctionType* ftype = FunctionType::get(Type::getVoidTy(Data->Context), args, false);

		jitbridgefunction = Function::Create(ftype, Function::ExternalLinkage, name.str().c_str(), Data->CurrentModule);
		Data->GeneratedFunctions[name.str()] = jitbridgefunction;
	}

	return jitbridgefunction;
}


//
// Map Epoch types to LLVM types
//
Type* NativeCodeGenerator::GetLLVMType(Metadata::EpochTypeID type, bool flatten)
{
	Metadata::EpochTypeFamily family = Metadata::GetTypeFamily(type);
	switch(type)
	{
	case Metadata::EpochType_Integer:
	case Metadata::EpochType_String:
	case Metadata::EpochType_Function:
	case Metadata::EpochType_Buffer:
		return Type::getInt32Ty(Data->Context);

	case Metadata::EpochType_Real:
		return Type::getFloatTy(Data->Context);

	case Metadata::EpochType_Boolean:
		return Type::getInt1Ty(Data->Context);

	case Metadata::EpochType_Integer16:
		return Type::getInt16Ty(Data->Context);

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
Type* NativeCodeGenerator::GetLLVMSumType(Metadata::EpochTypeID type, bool flatten)
{
	StructType* taggedtype = Data->SumTypeCache[type];
	if(!taggedtype && !flatten)
	{
		const VariantDefinition& def = OwnerVM.VariantDefinitions.find(type)->second;
		const std::set<Metadata::EpochTypeID>& types = def.GetBaseTypes();

		Type* rettype = NULL;
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

		std::vector<Type*> elemtypes;
		elemtypes.push_back(Data->VMTypeIDType);
		elemtypes.push_back(rettype);
		taggedtype = StructType::create(elemtypes, name.str());
		Data->SumTypeCache[type] = taggedtype;
	}

	return taggedtype;
}

//
// Synthesize the LLVM function type signature for a given Epoch function
//
FunctionType* NativeCodeGenerator::GetLLVMFunctionType(StringHandle epochfunc)
{
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


//
// Add a standard Epoch function implementation to the JIT module
//
void NativeCodeGenerator::AddFunction(size_t beginoffset, size_t endoffset, StringHandle alias)
{
	FunctionJITHelper jithelper(*this);
	jithelper.DoFunction(beginoffset, endoffset);

	Data->GeneratedBridgeFunctions[alias] = jithelper.BridgeFunction;
}

//
// Optimize LLVM bitcode and generate final native machine code
//
void NativeCodeGenerator::Generate()
{
	// This dump can come in handy if verification fails or we otherwise
	// need to check up on the bitcode being generated by the JIT engine
	//Data->CurrentModule->dump();

	// Always verify - it is useful for catching JIT bugs
	verifyModule(*Data->CurrentModule, AbortProcessAction);

#ifdef _DEBUG
	//EnableStatistics();
#endif

	std::string ErrStr;
	ExecutionEngine* ee = EngineBuilder(Data->CurrentModule).setErrorStr(&ErrStr).create();
	if(!ee)
		return;
	
	FunctionPassManager fpm(Data->CurrentModule);

	fpm.add(new DataLayout(*ee->getDataLayout()));
	fpm.add(createTypeBasedAliasAnalysisPass());
	fpm.add(createBasicAliasAnalysisPass());
	fpm.add(createCFGSimplificationPass());
	fpm.add(createScalarReplAggregatesPass());
	fpm.add(createEarlyCSEPass());
	fpm.add(createLowerExpectIntrinsicPass());

	VectorizeConfig vcfg;
	vcfg.ReqChainDepth = 2;
	vcfg.MaxIter = 500;
	fpm.add(createBBVectorizePass(vcfg));

	fpm.doInitialization();

	for(std::map<StringHandle, Function*>::const_iterator iter = Data->GeneratedBridgeFunctions.begin(); iter != Data->GeneratedBridgeFunctions.end(); ++iter)
		fpm.run(*iter->second);

	PassManager mpm;
	mpm.add(new DataLayout(*ee->getDataLayout()));
	mpm.add(createTypeBasedAliasAnalysisPass());
	mpm.add(createBasicAliasAnalysisPass());
	mpm.add(createGlobalOptimizerPass());
	mpm.add(createPromoteMemoryToRegisterPass());
	mpm.add(createIPSCCPPass());
	mpm.add(createDeadArgEliminationPass());
	mpm.add(createInstructionCombiningPass());
	mpm.add(createCFGSimplificationPass());
	mpm.add(createPruneEHPass());
	mpm.add(createFunctionAttrsPass());
	mpm.add(createFunctionInliningPass());
	mpm.add(createArgumentPromotionPass());
	mpm.add(createScalarReplAggregatesPass(-1, false));
	mpm.add(createEarlyCSEPass());
	mpm.add(createSimplifyLibCallsPass());
	mpm.add(createJumpThreadingPass());
	mpm.add(createCorrelatedValuePropagationPass());
	mpm.add(createCFGSimplificationPass());
	mpm.add(createInstructionCombiningPass());
	mpm.add(createTailCallEliminationPass());
	mpm.add(createCFGSimplificationPass());
	mpm.add(createReassociatePass());
	mpm.add(createLoopRotatePass());
	mpm.add(createLICMPass());
	mpm.add(createLoopUnswitchPass(false));
	mpm.add(createInstructionCombiningPass());
	mpm.add(createIndVarSimplifyPass());
	mpm.add(createLoopIdiomPass());
	mpm.add(createLoopDeletionPass());
	mpm.add(createLoopUnrollPass());
	mpm.add(createGVNPass());
	mpm.add(createMemCpyOptPass());
	mpm.add(createSCCPPass());
	mpm.add(createInstructionCombiningPass());
	mpm.add(createJumpThreadingPass());
	mpm.add(createCorrelatedValuePropagationPass());
	mpm.add(createDeadStoreEliminationPass());
	mpm.add(createAggressiveDCEPass());
	mpm.add(createCFGSimplificationPass());
	mpm.add(createInstructionCombiningPass());
	mpm.add(createFunctionInliningPass());

	mpm.run(*Data->CurrentModule);

	// This dump is useful for observing the optimized LLVM bitcode
	//Data->CurrentModule->dump();


	// Perform actual native code generation and link the created
	// pages of machine code back to the VM for execution
	for(std::map<StringHandle, Function*>::const_iterator iter = Data->GeneratedBridgeFunctions.begin(); iter != Data->GeneratedBridgeFunctions.end(); ++iter)
	{
		void* fptr = ee->getPointerToFunction(iter->second);
		OwnerVM.JITExecs[iter->first] = reinterpret_cast<EpochToJITWrapperFunc>(fptr);
	}

	// This is a no-op unless we enabled stats above
	// The numbers are very handy for A/B testing optimization passes
	PrintStatistics();
}



//
// Construct and initialize a function JIT helper object
//
FunctionJITHelper::FunctionJITHelper(NativeCodeGenerator& generator)
	: Generator(generator),
	  Builder(generator.Builder),
	  Context(generator.Data->Context),
	  Bytecode(generator.Bytecode)
{
	StackPtrType = Type::getInt8PtrTy(Generator.Data->Context);
	PStackPtrType = StackPtrType->getPointerTo();

	InstructionJITHelpers[Bytecode::Instructions::BeginEntity] = &FunctionJITHelper::BeginEntity;
	InstructionJITHelpers[Bytecode::Instructions::EndEntity] = &FunctionJITHelper::EndEntity;

	InstructionJITHelpers[Bytecode::Instructions::BeginChain] = &FunctionJITHelper::BeginChain;
	InstructionJITHelpers[Bytecode::Instructions::EndChain] = &FunctionJITHelper::EndChain;

	InstructionJITHelpers[Bytecode::Instructions::Read] = &FunctionJITHelper::Read;
	InstructionJITHelpers[Bytecode::Instructions::ReadStack] = &FunctionJITHelper::ReadStackLocal;
	InstructionJITHelpers[Bytecode::Instructions::ReadParam] = &FunctionJITHelper::ReadParameter;

	InstructionJITHelpers[Bytecode::Instructions::BindRef] = &FunctionJITHelper::BindReference;
	InstructionJITHelpers[Bytecode::Instructions::ReadRef] = &FunctionJITHelper::ReadRef;
	InstructionJITHelpers[Bytecode::Instructions::ReadRefAnnotated] = &FunctionJITHelper::ReadRefAnnotated;

	InstructionJITHelpers[Bytecode::Instructions::Assign] = &FunctionJITHelper::Assign;
	InstructionJITHelpers[Bytecode::Instructions::AssignSumType] = &FunctionJITHelper::AssignSumType;
	InstructionJITHelpers[Bytecode::Instructions::ConstructSumType] = &FunctionJITHelper::ConstructSumType;

	InstructionJITHelpers[Bytecode::Instructions::SetRetVal] = &FunctionJITHelper::SetRetValue;
	InstructionJITHelpers[Bytecode::Instructions::Return] = &FunctionJITHelper::Return;
	InstructionJITHelpers[Bytecode::Instructions::Halt] = &FunctionJITHelper::Halt;

	InstructionJITHelpers[Bytecode::Instructions::Push] = &FunctionJITHelper::Push;
	InstructionJITHelpers[Bytecode::Instructions::Pop] = &FunctionJITHelper::Pop;

	InstructionJITHelpers[Bytecode::Instructions::AllocStructure] = &FunctionJITHelper::AllocStructure;
	InstructionJITHelpers[Bytecode::Instructions::CopyToStructure] = &FunctionJITHelper::CopyToStructure;
	InstructionJITHelpers[Bytecode::Instructions::CopyStructure] = &FunctionJITHelper::CopyStructure;
	InstructionJITHelpers[Bytecode::Instructions::BindMemberRef] = &FunctionJITHelper::BindMemberRef;

	InstructionJITHelpers[Bytecode::Instructions::Invoke] = &FunctionJITHelper::Invoke;
	InstructionJITHelpers[Bytecode::Instructions::InvokeOffset] = &FunctionJITHelper::InvokeOffset;
	InstructionJITHelpers[Bytecode::Instructions::InvokeNative] = &FunctionJITHelper::InvokeNative;

	InstructionJITHelpers[Bytecode::Instructions::TypeMatch] = &FunctionJITHelper::TypeMatch;
}


//
// JIT a function
//
void FunctionJITHelper::DoFunction(size_t beginoffset, size_t endoffset)
{
	// Set up context for external libraries that we need to interact with
	// TODO - clean up JITContext a bit
	LibJITContext.Builder = &Builder;
	LibJITContext.VMContextPtr = NULL;
	LibJITContext.Context = &Context;
	LibJITContext.MyModule = Generator.Data->CurrentModule;
	LibJITContext.InnerFunction = NULL;
	LibJITContext.VMGetBuffer = Generator.Data->VMFunctions[VMGetBuffer];
	LibJITContext.SqrtIntrinsic = Generator.Data->Intrinsics[IntrinsicSqrt];


	// Initialize tracking for JIT operations
	CurrentScope = NULL;

	NumParameters = 0;
	NumParameterBytes = 0;
	NumReturns = 0;
	TypeMatchIndex = 0;

	HackStructType = 0;

	BeginOffset = beginoffset;
	EndOffset = endoffset;


	// Set up the bridge function and its known required basic blocks
	BridgeFunction = Generator.GetGeneratedBridge(beginoffset);

	BridgeEntryBlock = BasicBlock::Create(Context, "entry", BridgeFunction);
	BridgeExitBlock = BasicBlock::Create(Context, "exit", BridgeFunction);

	// Initialize block pointers that are lazily populated
	InnerExitBlock = NULL;
	NativeMatchBlock = NULL;

	// Initialize values used during JIT procedures
	OuterRetVal = NULL;
	InnerRetVal = NULL;
	TypeMatchRetVal = NULL;


	// Prepare for bitcode generation
	Builder.SetInsertPoint(BridgeEntryBlock);


	// Marshal in parameters from the VM
	PStackPtr = Builder.CreateAlloca(PStackPtrType);
	Builder.CreateStore(BridgeFunction->arg_begin(), PStackPtr);
	VMContextPtr = ++(BridgeFunction->arg_begin());


	// Now process each instruction in the Epoch bytecode and produce the LLVM bitcode output
	for(size_t offset = beginoffset; offset <= endoffset; )
	{
		Bytecode::Instruction instruction = Bytecode[offset++];

		if(InstructionJITHelpers.find(instruction) == InstructionJITHelpers.end())
			throw FatalException("Invalid instruction for native code generation");

		InstructionJITHelper helper = InstructionJITHelpers[instruction];
		(this->*helper)(offset);
	}
	
	if(LibJITContext.InnerFunction)
	{
		Builder.SetInsertPoint(InnerExitBlock);
		if(InnerRetVal)
		{
			Value* ret = Builder.CreateLoad(InnerRetVal);
			Builder.CreateRet(ret);
		}
		else
			Builder.CreateRetVoid();
	}

	Builder.SetInsertPoint(BridgeEntryBlock);
	if(!TypeMatchIndex)
		Builder.CreateBr(BridgeExitBlock);

	Builder.SetInsertPoint(BridgeExitBlock);

	LoadInst* stackptr = Builder.CreateLoad(Builder.CreateLoad(PStackPtr));
	Constant* offset = ConstantInt::get(Type::getInt32Ty(Context), static_cast<unsigned>(NumParameterBytes - (NumReturns * 4)));	// TODO - change to retbytes
	Value* stackptr2 = Builder.CreateGEP(stackptr, offset);
	if(OuterRetVal)
	{
		Value* ret = Builder.CreateLoad(OuterRetVal);
		Builder.CreateStore(ret, Builder.CreatePointerCast(stackptr2, PointerType::get(ret->getType(), 0)));
	}
	Builder.CreateStore(stackptr2, Builder.CreateLoad(PStackPtr));

	Builder.CreateRetVoid();


	if(NativeMatchBlock)
	{
		Builder.SetInsertPoint(NativeMatchBlock);
		Generator.AddNativeTypeMatcher(beginoffset, endoffset);
	}
}

void FunctionJITHelper::BeginEntity(size_t& offset)
{
	Bytecode::EntityTag entitytype = Fetch<Integer32>(Bytecode, offset);
	LibJITContext.EntityTypes.push(entitytype);

	StringHandle entityname = Fetch<StringHandle>(Bytecode, offset);

	if(entitytype == Bytecode::EntityTags::Function)
	{
		Type* rettype = Type::getVoidTy(Context);

		Value* stackptr = Builder.CreateLoad(Builder.CreateLoad(PStackPtr));
		Type* type = NULL;

		std::set<size_t> locals;
		std::set<size_t> parameters;

		size_t localoffsetbytes = 0;

		size_t retindex = static_cast<size_t>(-1);

		const ScopeDescription& scope = Generator.OwnerVM.GetScopeDescription(entityname);
		CurrentScope = &scope;
		for(size_t i = scope.GetVariableCount(); i-- > 0; )
		{
			Metadata::EpochTypeID vartype = scope.GetVariableTypeByIndex(i);

			if(vartype == Metadata::EpochType_Nothing)
			{
				if(scope.GetVariableOrigin(i) == VARIABLE_ORIGIN_PARAMETER)
				{
					Value* zero = Builder.CreateAlloca(Type::getInt32Ty(Context));
					Builder.CreateStore(ConstantInt::get(Type::getInt32Ty(Context), 0xfeedface), zero);
					parameters.insert(i);
					++NumParameters;
					LibJITContext.VariableMap[i] = zero;
				}
				continue;
			}

			type = Generator.GetLLVMType(vartype);

			switch(scope.GetVariableOrigin(i))
			{
			case VARIABLE_ORIGIN_RETURN:
				++NumReturns;
				retindex = i;
				rettype = type;
				LibJITContext.VariableMap[i] = Builder.CreateAlloca(type);
				// Deliberate fallthrough

			case VARIABLE_ORIGIN_LOCAL:
				locals.insert(i);
				break;

			case VARIABLE_ORIGIN_PARAMETER:
				{
					ParameterOffsetToIndexMap[NumParameterBytes] = i;
					parameters.insert(i);
					Constant* offset = ConstantInt::get(Type::getInt32Ty(Context), NumParameterBytes);
					++NumParameters;

					if(scope.IsReference(i))
					{
						Type* ptype = PointerType::get(PointerType::get(type, 0), 0);
						Value* newstackptr = Builder.CreateGEP(stackptr, offset);
						Value* ref = Builder.CreatePointerCast(newstackptr, ptype);
						LibJITContext.VariableMap[i] = ref;
						NumParameterBytes += sizeof(void*) + sizeof(Metadata::EpochTypeID);
					}
					else
					{
						Value* local = Builder.CreateAlloca(type);
						LibJITContext.VariableMap[i] = local;
						Value* newstackptr = Builder.CreateGEP(stackptr, offset);
						Value* stackval = Builder.CreateLoad(Builder.CreatePointerCast(newstackptr, type->getPointerTo()));
						Builder.CreateStore(stackval, local);
						NumParameterBytes += Metadata::GetStorageSize(vartype);
					}
				}
				break;
			}

			LibJITContext.NameToIndexMap[scope.GetVariableNameHandle(i)] = i;
		}

		LibJITContext.InnerFunction = Generator.GetGeneratedFunction(entityname, BeginOffset);

		Builder.SetInsertPoint(BridgeEntryBlock);

		std::vector<Value*> innerparams;
		innerparams.push_back(VMContextPtr);
		for(std::set<size_t>::const_iterator paramiter = parameters.begin(); paramiter != parameters.end(); ++paramiter)
		{
			Value* p = Builder.CreateLoad(LibJITContext.VariableMap[*paramiter]);
			innerparams.push_back(p);
		}

		if(NumReturns)
		{
			Value* r = Builder.CreateCall(LibJITContext.InnerFunction, innerparams);
			Builder.CreateStore(r, LibJITContext.VariableMap[retindex]);
		}
		else
			Builder.CreateCall(LibJITContext.InnerFunction, innerparams);

		BasicBlock* innerentryblock = BasicBlock::Create(Context, "innerentry", LibJITContext.InnerFunction);
		Builder.SetInsertPoint(innerentryblock);

		InnerExitBlock = BasicBlock::Create(Context, "innerexit", LibJITContext.InnerFunction);

		if(NumReturns)
			InnerRetVal = Builder.CreateAlloca(rettype);

		size_t i = 0;
		OuterRetVal = NumReturns ? LibJITContext.VariableMap[retindex] : NULL;
		Function::ArgumentListType& args = LibJITContext.InnerFunction->getArgumentList();
		Function::ArgumentListType::iterator argiter = args.begin();
		++argiter;
		for(; argiter != args.end(); ++argiter)
			LibJITContext.VariableMap[i++] = ((Argument*)argiter);

		if(NumReturns)
			LibJITContext.VariableMap[retindex] = InnerRetVal;

		localoffsetbytes = 0;
		for(std::set<size_t>::const_iterator localiter = locals.begin(); localiter != locals.end(); ++localiter)
		{
			Metadata::EpochTypeID localtype = scope.GetVariableTypeByIndex(*localiter);
			Type* type = Generator.GetLLVMType(localtype);

			if(*localiter != retindex)
				LibJITContext.VariableMap[*localiter] = Builder.CreateAlloca(type, NULL, narrow(Generator.OwnerVM.GetPooledString(scope.GetVariableNameHandle(*localiter))));

			LocalOffsetToIndexMap[localoffsetbytes] = *localiter;

			if(Metadata::GetTypeFamily(localtype) == Metadata::EpochTypeFamily_SumType)
				localoffsetbytes += Generator.OwnerVM.VariantDefinitions.find(localtype)->second.GetMaxSize();
			else
				localoffsetbytes += Metadata::GetStorageSize(localtype);
		}

		LibJITContext.VMContextPtr = LibJITContext.InnerFunction->arg_begin();
	}
	else if(entitytype == Bytecode::EntityTags::TypeResolver)
	{
		Function* nativetypematcher = Generator.GetGeneratedTypeMatcher(entityname, BeginOffset);
		NativeMatchBlock = BasicBlock::Create(Context, "nativematchentry", nativetypematcher);
	}
	else
	{
		std::map<StringHandle, JIT::JITHelper>::const_iterator helperiter = Generator.OwnerVM.JITHelpers.EntityHelpers.find(entitytype);
		if(helperiter == Generator.OwnerVM.JITHelpers.EntityHelpers.end())
			throw FatalException("Cannot JIT this type of entity");
					
		helperiter->second(LibJITContext, true);
	}
}


void FunctionJITHelper::EndEntity(size_t&)
{
	Bytecode::EntityTag tag = LibJITContext.EntityTypes.top();
	LibJITContext.EntityTypes.pop();

	if(tag != Bytecode::EntityTags::Function && tag != Bytecode::EntityTags::TypeResolver)
		Generator.OwnerVM.JITHelpers.EntityHelpers.find(tag)->second(LibJITContext, false);
}


void FunctionJITHelper::BeginChain(size_t&)
{
	LibJITContext.EntityChecks.push(BasicBlock::Create(Context, "", LibJITContext.InnerFunction));
	LibJITContext.EntityChains.push(BasicBlock::Create(Context, "", LibJITContext.InnerFunction));
	LibJITContext.EntityChainExits.push(BasicBlock::Create(Context, "", LibJITContext.InnerFunction));
	Builder.CreateBr(LibJITContext.EntityChecks.top());
	Builder.SetInsertPoint(LibJITContext.EntityChecks.top());
}

void FunctionJITHelper::EndChain(size_t&)
{
	if(LibJITContext.EntityChains.top()->empty())
	{
		Builder.SetInsertPoint(LibJITContext.EntityChains.top());
		Builder.CreateBr(LibJITContext.EntityChainExits.top());
	}

	LibJITContext.EntityChecks.pop();
	LibJITContext.EntityChains.pop();
	Builder.SetInsertPoint(LibJITContext.EntityChainExits.top());
	LibJITContext.EntityChainExits.pop();
}


void FunctionJITHelper::Read(size_t& offset)
{
	StringHandle varname = Fetch<StringHandle>(Bytecode, offset);
	size_t index = LibJITContext.NameToIndexMap[varname];

	Value* v = NULL;

	if(index >= NumParameters)
		v = Builder.CreateLoad(LibJITContext.VariableMap[index]);
	else
		v = LibJITContext.VariableMap[index];

	if(CurrentScope->IsReferenceByID(varname))
		v = Builder.CreateLoad(v);

	LibJITContext.ValuesOnStack.push(v);
}

void FunctionJITHelper::ReadStackLocal(size_t& offset)
{
	size_t frames = Fetch<size_t>(Bytecode, offset);
	size_t stackoffset = Fetch<size_t>(Bytecode, offset);
	Fetch<size_t>(Bytecode, offset);		// stack size is irrelevant

	if(frames != 0)
		throw NotImplementedException("Scope is not flat!");

	size_t index = LocalOffsetToIndexMap[stackoffset];
	Value* val = Builder.CreateLoad(LibJITContext.VariableMap[index]);
	LibJITContext.ValuesOnStack.push(val);
}

void FunctionJITHelper::ReadParameter(size_t& offset)
{
	size_t frames = Fetch<size_t>(Bytecode, offset);
	size_t stackoffset = Fetch<size_t>(Bytecode, offset);
	Fetch<size_t>(Bytecode, offset);		// stack size is irrelevant

	if(frames != 0)
		throw NotImplementedException("Scope is not flat!");
				
	size_t idx = ParameterOffsetToIndexMap[stackoffset];
	Value* val = LibJITContext.VariableMap[idx];
	if(CurrentScope->IsReference(idx))
		val = Builder.CreateLoad(val);
	LibJITContext.ValuesOnStack.push(val);
}

void FunctionJITHelper::BindReference(size_t& offset)
{
	size_t frames = Fetch<size_t>(Bytecode, offset);
	size_t index = Fetch<size_t>(Bytecode, offset);

	if(frames > 0)
		throw NotImplementedException("Scope is not flat!");

	Metadata::EpochTypeID vartype = CurrentScope->GetVariableTypeByIndex(index);
	Value* ptr = LibJITContext.VariableMap[index];
	LibJITContext.ValuesOnStack.push(ptr);
	TypeAnnotations.push(vartype);
}

void FunctionJITHelper::ReadRef(size_t&)
{
	Value* derefvalue = Builder.CreateLoad(LibJITContext.ValuesOnStack.top());
	LibJITContext.ValuesOnStack.pop();
	LibJITContext.ValuesOnStack.push(derefvalue);
	TypeAnnotations.pop();
}

void FunctionJITHelper::ReadRefAnnotated(size_t&)
{
	std::vector<Value*> gepindices;
	gepindices.push_back(ConstantInt::get(Type::getInt32Ty(Context), 0));
	gepindices.push_back(ConstantInt::get(Type::getInt32Ty(Context), 0));
	Value* annotationgep = Builder.CreateGEP(LibJITContext.ValuesOnStack.top(), gepindices);

	std::vector<Value*> derefindices;
	derefindices.push_back(ConstantInt::get(Type::getInt32Ty(Context), 0));
	derefindices.push_back(ConstantInt::get(Type::getInt32Ty(Context), 1));
	Value* derefgep = Builder.CreateLoad(Builder.CreateGEP(LibJITContext.ValuesOnStack.top(), derefindices));

	Value* annotationvalue = Builder.CreateLoad(annotationgep);
	LibJITContext.ValuesOnStack.pop();
	LibJITContext.ValuesOnStack.push(derefgep);
	LibJITContext.ValuesOnStack.push(annotationvalue);
	TypeAnnotations.pop();
}

void FunctionJITHelper::Assign(size_t&)
{
	Value* reftarget = LibJITContext.ValuesOnStack.top();
	LibJITContext.ValuesOnStack.pop();
	Value* v = LibJITContext.ValuesOnStack.top();
	LibJITContext.ValuesOnStack.pop();
	Builder.CreateStore(v, reftarget);
}

void FunctionJITHelper::AssignSumType(size_t&)
{
	Value* reftarget = LibJITContext.ValuesOnStack.top();
	LibJITContext.ValuesOnStack.pop();
	Value* actualtype = LibJITContext.ValuesOnStack.top();
	LibJITContext.ValuesOnStack.pop();

	Value* typeholder = Builder.CreatePointerCast(reftarget, Type::getInt32PtrTy(Context));
	if(actualtype->getType()->getNumContainedTypes() > 0)
	{
		LoadInst* load = dyn_cast<LoadInst>(actualtype);

		std::vector<Value*> gepindices;
		gepindices.push_back(ConstantInt::get(Type::getInt32Ty(Context), 0));
		gepindices.push_back(ConstantInt::get(Type::getInt32Ty(Context), 0));
		Builder.CreateStore(Builder.CreateLoad(Builder.CreateGEP(load->getOperand(0), gepindices)), typeholder);

		std::vector<Value*> payloadgepindices;
		payloadgepindices.push_back(ConstantInt::get(Type::getInt32Ty(Context), 0));
		payloadgepindices.push_back(ConstantInt::get(Type::getInt32Ty(Context), 1));
		Value* payloadgep = Builder.CreateGEP(load->getOperand(0), payloadgepindices);
		Value* payload = Builder.CreateLoad(payloadgep);

		Value* target = Builder.CreateGEP(Builder.CreatePointerCast(reftarget, Type::getInt8PtrTy(Context)), ConstantInt::get(Type::getInt32Ty(Context), sizeof(Metadata::EpochTypeID)));
		Value* casttarget = Builder.CreatePointerCast(target, payload->getType()->getPointerTo());
		Builder.CreateStore(payload, casttarget);
	}
	else
	{
		Builder.CreateStore(actualtype, typeholder);

		Value* target = Builder.CreateGEP(Builder.CreatePointerCast(reftarget, Type::getInt8PtrTy(Context)), ConstantInt::get(Type::getInt32Ty(Context), sizeof(Metadata::EpochTypeID)));
		Value* casttarget = Builder.CreatePointerCast(target, LibJITContext.ValuesOnStack.top()->getType()->getPointerTo());
		Builder.CreateStore(LibJITContext.ValuesOnStack.top(), casttarget);

		LibJITContext.ValuesOnStack.pop();
	}
}

void FunctionJITHelper::ConstructSumType(size_t&)
{
	Value* vartype = LibJITContext.ValuesOnStack.top();
	LibJITContext.ValuesOnStack.pop();

	Value* value = LibJITContext.ValuesOnStack.top();
	LibJITContext.ValuesOnStack.pop();

	Value* targetid = LibJITContext.ValuesOnStack.top();
	LibJITContext.ValuesOnStack.pop();

	ConstantInt* cint = dyn_cast<ConstantInt>(targetid);
	size_t vartarget = static_cast<size_t>(cint->getValue().getLimitedValue());

	Value* storagetarget = LibJITContext.VariableMap[LibJITContext.NameToIndexMap[vartarget]];

	// Set contents
	{
		std::vector<Value*> memberindices;
		memberindices.push_back(ConstantInt::get(Type::getInt32Ty(Context), 0));
		memberindices.push_back(ConstantInt::get(Type::getInt32Ty(Context), 1));
		Value* valueholder = Builder.CreateGEP(storagetarget, memberindices);
		Builder.CreateStore(value, valueholder);
	}

	// Set type annotation
	{
		std::vector<Value*> memberindices;
		memberindices.push_back(ConstantInt::get(Type::getInt32Ty(Context), 0));
		memberindices.push_back(ConstantInt::get(Type::getInt32Ty(Context), 0));
		Value* typeholder = Builder.CreateGEP(storagetarget, memberindices);
		Builder.CreateStore(vartype, typeholder);
	}
}

void FunctionJITHelper::SetRetValue(size_t& offset)
{
	size_t index = Fetch<size_t>(Bytecode, offset);
	Builder.CreateStore(Builder.CreateLoad(LibJITContext.VariableMap[index]), InnerRetVal);
}

void FunctionJITHelper::Return(size_t&)
{
	Builder.CreateBr(InnerExitBlock);
}

void FunctionJITHelper::Halt(size_t&)
{
	Builder.CreateCall(Generator.Data->VMFunctions[VMHalt]);
	Builder.CreateUnreachable();
}

void FunctionJITHelper::Push(size_t& offset)
{
	Metadata::EpochTypeID type = Fetch<Metadata::EpochTypeID>(Bytecode, offset);
	Constant* valueval;

	switch(type)
	{
	case Metadata::EpochType_Integer:
		{
			Integer32 value = Fetch<Integer32>(Bytecode, offset);
			valueval = ConstantInt::get(Type::getInt32Ty(Context), value);
		}
		break;

	case Metadata::EpochType_Identifier:
	case Metadata::EpochType_String:
		{
			StringHandle value = Fetch<StringHandle>(Bytecode, offset);
			valueval = ConstantInt::get(Type::getInt32Ty(Context), value);
		}
		break;

	case Metadata::EpochType_Real:
		{
			Real32 value = Fetch<Real32>(Bytecode, offset);
			valueval = ConstantFP::get(Type::getFloatTy(Context), value);
		}
		break;

	case Metadata::EpochType_Boolean:
		{
			bool value = Fetch<bool>(Bytecode, offset);
			valueval = ConstantInt::get(Type::getInt1Ty(Context), value);
		}
		break;

	case Metadata::EpochType_Integer16:
	case Metadata::EpochType_Buffer:
	default:
		throw FatalException("Unsupported type for JIT compilation");
	}

	LibJITContext.ValuesOnStack.push(valueval);
}

void FunctionJITHelper::Pop(size_t&)
{
	LibJITContext.ValuesOnStack.pop();
}

void FunctionJITHelper::AllocStructure(size_t& offset)
{
	Metadata::EpochTypeID type = Fetch<Metadata::EpochTypeID>(Bytecode, offset);
	Value* typeconst = ConstantInt::get(Type::getInt32Ty(Context), type);
	Value* handle = Builder.CreateCall2(Generator.Data->VMFunctions[VMAllocStruct], LibJITContext.InnerFunction->arg_begin(), typeconst);
	LibJITContext.ValuesOnStack.push(handle);

	HackStructType = type;
}

void FunctionJITHelper::CopyToStructure(size_t& offset)
{
	StringHandle variablename = Fetch<StringHandle>(Bytecode, offset);
	StringHandle actualmember = Fetch<StringHandle>(Bytecode, offset);

	Value* v = LibJITContext.VariableMap[LibJITContext.NameToIndexMap[variablename]];
	Value* structptr = Builder.CreateLoad(v);

	const StructureDefinition& def = Generator.OwnerVM.GetStructureDefinition(HackStructType);
	size_t memberindex = def.FindMember(actualmember);
	size_t memberoffset = def.GetMemberOffset(memberindex);
	Metadata::EpochTypeID membertype = def.GetMemberType(memberindex);

	Value* memberptr = Builder.CreateGEP(structptr, ConstantInt::get(Type::getInt32Ty(Context), memberoffset));
	Value* castmemberptr = Builder.CreatePointerCast(memberptr, Generator.GetLLVMType(membertype)->getPointerTo());
				
	Builder.CreateStore(LibJITContext.ValuesOnStack.top(), castmemberptr);

	LibJITContext.ValuesOnStack.pop();
}

void FunctionJITHelper::CopyStructure(size_t&)
{
	Value* structureptr = LibJITContext.ValuesOnStack.top();
	LibJITContext.ValuesOnStack.pop();
	Value* copyptr = Builder.CreateCall2(Generator.Data->VMFunctions[VMCopyStruct], LibJITContext.InnerFunction->arg_begin(), structureptr);
	Value* castptr = Builder.CreatePointerCast(copyptr, Type::getInt8PtrTy(Context));
	LibJITContext.ValuesOnStack.push(castptr);
}

void FunctionJITHelper::BindMemberRef(size_t& offset)
{
	Metadata::EpochTypeID membertype = Fetch<Metadata::EpochTypeID>(Bytecode, offset);
	size_t memberoffset = Fetch<size_t>(Bytecode, offset);

	Value* voidstructptr = LibJITContext.ValuesOnStack.top();
	if(voidstructptr->getType() == Type::getInt8PtrTy(Context)->getPointerTo())
		voidstructptr = Builder.CreateLoad(voidstructptr);
	Value* bytestructptr = Builder.CreatePointerCast(voidstructptr, Type::getInt8PtrTy(Context));
	Value* voidmemberptr = Builder.CreateGEP(bytestructptr, ConstantInt::get(Type::getInt32Ty(Context), memberoffset));
	Value* memberptr = Builder.CreatePointerCast(voidmemberptr, Generator.GetLLVMType(membertype)->getPointerTo());

	LibJITContext.ValuesOnStack.pop();
	LibJITContext.ValuesOnStack.push(memberptr);
	TypeAnnotations.push(membertype);
}

void FunctionJITHelper::Invoke(size_t& offset)
{
	StringHandle target = Fetch<StringHandle>(Bytecode, offset);
	std::map<StringHandle, JIT::JITHelper>::const_iterator iter = Generator.OwnerVM.JITHelpers.InvokeHelpers.find(target);
	if(iter == Generator.OwnerVM.JITHelpers.InvokeHelpers.end())
		throw FatalException("Cannot invoke this function, no native code support!");

	iter->second(LibJITContext, true);
}

void FunctionJITHelper::InvokeOffset(size_t& offset)
{
	StringHandle functionname = Fetch<StringHandle>(Bytecode, offset);
	size_t internaloffset = Fetch<size_t>(Bytecode, offset);

	if(Generator.OwnerVM.TypeMatcherParamCount.find(functionname) == Generator.OwnerVM.TypeMatcherParamCount.end())
		throw FatalException("Cannot invoke VM code from native code");

	Function* nativetypematcher = Generator.GetGeneratedTypeMatcher(functionname, internaloffset);

	std::vector<Value*> matchervarargs;
	size_t numparams = Generator.OwnerVM.TypeMatcherParamCount.find(functionname)->second;
	matchervarargs.push_back(LibJITContext.InnerFunction->arg_begin());
	for(size_t i = 0; i < numparams; ++i)
	{
		Value* v1 = LibJITContext.ValuesOnStack.top();
		LibJITContext.ValuesOnStack.pop();

		Value* v2 = LibJITContext.ValuesOnStack.top();
		LibJITContext.ValuesOnStack.pop();

		if(v2->getType()->isPointerTy())
		{
			Metadata::EpochTypeID paramepochtype = TypeAnnotations.top();
			Value* annotation = ConstantInt::get(Type::getInt32Ty(Context), paramepochtype);
			TypeAnnotations.pop();

			matchervarargs.push_back(annotation);
			matchervarargs.push_back(Builder.CreatePointerCast(v2, Type::getInt8PtrTy(Context)));
		}
		else
		{
			LoadInst* load = dyn_cast<LoadInst>(v2);
			if(load)
			{
				matchervarargs.push_back(v1);
				matchervarargs.push_back(Builder.CreatePointerCast(load->getOperand(0), Type::getInt8PtrTy(Context)));
			}
			else
			{
				Value* stacktemp = Builder.CreateAlloca(v2->getType());
				Builder.CreateStore(v2, stacktemp);

				matchervarargs.push_back(v1);
				matchervarargs.push_back(Builder.CreatePointerCast(stacktemp, Type::getInt8PtrTy(Context)));
			}
		}
	}

	LibJITContext.ValuesOnStack.push(Builder.CreateCall(nativetypematcher, matchervarargs));
}

void FunctionJITHelper::InvokeNative(size_t& offset)
{
	StringHandle target = Fetch<StringHandle>(Bytecode, offset);
	Fetch<size_t>(Bytecode, offset);		// skip dummy offset
	std::map<StringHandle, JIT::JITHelper>::const_iterator iter = Generator.OwnerVM.JITHelpers.InvokeHelpers.find(target);
	if(iter != Generator.OwnerVM.JITHelpers.InvokeHelpers.end())
		iter->second(LibJITContext, true);
	else
	{
		Function* targetfunc = Generator.GetGeneratedFunction(target, Generator.OwnerVM.GetFunctionInstructionOffsetNoThrow(target));

		std::vector<Value*> targetargs;
		for(size_t i = 1; i < targetfunc->getFunctionType()->getNumParams(); ++i)
		{
			Value* p = LibJITContext.ValuesOnStack.top();
			LibJITContext.ValuesOnStack.pop();

			targetargs.push_back(p);
		}
		targetargs.push_back(LibJITContext.InnerFunction->arg_begin());
		std::reverse(targetargs.begin(), targetargs.end());

		LibJITContext.ValuesOnStack.push(Builder.CreateCall(targetfunc, targetargs));
	}
}

void FunctionJITHelper::TypeMatch(size_t& offset)
{
	std::ostringstream nextname;
	nextname << "nexttypematcher" << ++TypeMatchIndex;
	BasicBlock* nexttypematcher = BasicBlock::Create(Context, nextname.str(), BridgeFunction);

	Value* providedtype = Builder.CreateAlloca(Type::getInt32Ty(Context), NULL, "providedtype");
	Value* initialstackptr = Builder.CreateLoad(Builder.CreateLoad(PStackPtr));
	Value* stackoffsettracker = Builder.CreateAlloca(Type::getInt32Ty(Context), NULL, "stackoffset");

	TypeMatchConsumedBytes = Builder.CreateAlloca(Type::getInt32Ty(Context)), NULL, "consumedbytes";
	Builder.CreateStore(ConstantInt::get(Type::getInt32Ty(Context), 0), stackoffsettracker);

	StringHandle funcname = Fetch<StringHandle>(Bytecode, offset);
	size_t matchoffset = Fetch<size_t>(Bytecode, offset);
	size_t paramcount = Fetch<size_t>(Bytecode, offset);

	std::vector<Value*> actualparams;

	for(size_t i = 0; i < paramcount; ++i)
	{
		bool expectref = Fetch<bool>(Bytecode, offset);
		Metadata::EpochTypeID expecttype = Fetch<Metadata::EpochTypeID>(Bytecode, offset);

		BasicBlock* checkmatchblock = BasicBlock::Create(Context, "checkmatch", BridgeFunction);

		Value* parampayloadptr = Builder.CreateAlloca(Type::getInt8PtrTy(Context), NULL, "parampayloadptr");

		if(!expectref)
		{
			Value* lateststackptr = Builder.CreateGEP(initialstackptr, Builder.CreateLoad(stackoffsettracker));
			Value* gettype = Builder.CreateLoad(Builder.CreatePointerCast(lateststackptr, Type::getInt32PtrTy(Context)));
			Builder.CreateStore(gettype, providedtype);

			// TODO - support undecomposed sum types

			Value* providedrefflag = Builder.CreateICmpEQ(gettype, ConstantInt::get(Type::getInt32Ty(Context), Metadata::EpochType_RefFlag));

			BasicBlock* providedrefblock = BasicBlock::Create(Context, "providedref", BridgeFunction);
			BasicBlock* providedvalueblock = BasicBlock::Create(Context, "providedvalue", BridgeFunction);
			Builder.CreateCondBr(providedrefflag, providedrefblock, providedvalueblock);

			Builder.SetInsertPoint(providedrefblock);
			Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(Context), sizeof(Metadata::EpochTypeID))), stackoffsettracker);

			Value* providednothingflag = Builder.CreateICmpEQ(gettype, ConstantInt::get(Type::getInt32Ty(Context), Metadata::EpochType_Nothing));
			Value* expectnothing = (expecttype == Metadata::EpochType_Nothing ? ConstantInt::get(Type::getInt1Ty(Context), 1) : ConstantInt::get(Type::getInt1Ty(Context), 0));
			Value* provideandexpectnothing = Builder.CreateAnd(providednothingflag, expectnothing);

			BasicBlock* handlesomethingblock = BasicBlock::Create(Context, "handlesomething", BridgeFunction);
			BasicBlock* handlenothingblock = BasicBlock::Create(Context, "handlenothing", BridgeFunction);

			Builder.CreateCondBr(provideandexpectnothing, handlenothingblock, handlesomethingblock);
			Builder.SetInsertPoint(handlenothingblock);
			Builder.CreateStore(ConstantInt::get(Type::getInt32Ty(Context), Metadata::EpochType_Nothing), providedtype);
			Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(Context), sizeof(void*))), stackoffsettracker);
			Builder.CreateBr(checkmatchblock);

			Builder.SetInsertPoint(handlesomethingblock);
			BasicBlock* handlerefblock = BasicBlock::Create(Context, "handleref", BridgeFunction);
			Builder.CreateCondBr(providednothingflag, nexttypematcher, handlerefblock);
			Builder.SetInsertPoint(handlerefblock);

			Value* reftarget = Builder.CreateGEP(initialstackptr, Builder.CreateLoad(stackoffsettracker));
			Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(Context), sizeof(void*))), stackoffsettracker);

			Value* reftype = Builder.CreateLoad(Builder.CreatePointerCast(Builder.CreateGEP(initialstackptr, Builder.CreateLoad(stackoffsettracker)), Type::getInt32PtrTy(Context)));
			Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(Context), sizeof(Metadata::EpochTypeID))), stackoffsettracker);

			Value* refisnothingflag = Builder.CreateICmpEQ(reftype, ConstantInt::get(Type::getInt32Ty(Context), Metadata::EpochType_Nothing));
			Value* refandexpectnothing = Builder.CreateAnd(expectnothing, refisnothingflag);

			BasicBlock* handlenothingrefblock = BasicBlock::Create(Context, "handlenothingref", BridgeFunction);
			BasicBlock* checksumtypeblock = BasicBlock::Create(Context, "checksumtype", BridgeFunction);
			Builder.CreateCondBr(refandexpectnothing, handlenothingrefblock, checksumtypeblock);

			Builder.SetInsertPoint(handlenothingrefblock);
			Builder.CreateStore(ConstantInt::get(Type::getInt32Ty(Context), Metadata::EpochType_Nothing), providedtype);
			Builder.CreateBr(checkmatchblock);

			BasicBlock* handlesumtypeblock = BasicBlock::Create(Context, "handlesumtype", BridgeFunction);

			Builder.SetInsertPoint(checksumtypeblock);
			Value* providedtypefamily = Builder.CreateAnd(gettype, 0xff000000);
			Value* issumtype = Builder.CreateICmpEQ(providedtypefamily, ConstantInt::get(Type::getInt32Ty(Context), Metadata::EpochTypeFamily_SumType));
			Builder.CreateCondBr(issumtype, handlesumtypeblock, nexttypematcher);

			Builder.SetInsertPoint(handlesumtypeblock);
			Value* reftypeptr = Builder.CreateGEP(reftarget, ConstantInt::get(Type::getInt32Ty(Context), static_cast<uint64_t>(-(signed)sizeof(Metadata::EpochTypeID))));
			reftype = Builder.CreateLoad(Builder.CreatePointerCast(reftypeptr, Type::getInt32PtrTy(Context)));

			BasicBlock* handlereftonothingblock = BasicBlock::Create(Context, "handlereftonothing", BridgeFunction);
			refisnothingflag = Builder.CreateICmpEQ(reftype, ConstantInt::get(Type::getInt32Ty(Context), Metadata::EpochType_Nothing));
			refandexpectnothing = Builder.CreateAnd(expectnothing, refisnothingflag);
			Builder.CreateCondBr(refandexpectnothing, handlereftonothingblock, nexttypematcher);

			Builder.SetInsertPoint(handlereftonothingblock);
			Builder.CreateStore(reftype, providedtype);
			Builder.CreateBr(checkmatchblock);

			Builder.SetInsertPoint(providedvalueblock);
			Builder.CreateStore(Builder.CreateLoad(Builder.CreatePointerCast(Builder.CreateGEP(initialstackptr, Builder.CreateLoad(stackoffsettracker)), Type::getInt32PtrTy(Context))), providedtype);
			Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(Context), sizeof(Metadata::EpochTypeID))), stackoffsettracker);
			Builder.CreateStore(Builder.CreateGEP(initialstackptr, Builder.CreateLoad(stackoffsettracker)), parampayloadptr);
			Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(Context), Metadata::GetStorageSize(expecttype))), stackoffsettracker);
			Builder.CreateBr(checkmatchblock);
		}
		else
		{
			Value* magic = Builder.CreateLoad(Builder.CreatePointerCast(Builder.CreateGEP(initialstackptr, Builder.CreateLoad(stackoffsettracker)), Type::getInt32PtrTy(Context)), true);
			Value* providedrefflag = Builder.CreateICmpEQ(magic, ConstantInt::get(Type::getInt32Ty(Context), Metadata::EpochType_RefFlag));

			BasicBlock* providedrefblock = BasicBlock::Create(Context, "providedexpectedref", BridgeFunction);
			Builder.CreateCondBr(providedrefflag, providedrefblock, nexttypematcher);
			Builder.SetInsertPoint(providedrefblock);

			Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(Context), sizeof(Metadata::EpochTypeID))), stackoffsettracker);

			Value* reftarget = Builder.CreateLoad(Builder.CreatePointerCast(Builder.CreateGEP(initialstackptr, Builder.CreateLoad(stackoffsettracker)), PointerType::get(Type::getInt8PtrTy(Context), 0)));
			Builder.CreateStore(reftarget, parampayloadptr);
			Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(Context), sizeof(void*))), stackoffsettracker);

			Value* reftype = Builder.CreateLoad(Builder.CreatePointerCast(Builder.CreateGEP(initialstackptr, Builder.CreateLoad(stackoffsettracker)), Type::getInt32PtrTy(Context)));
			Builder.CreateStore(reftype, providedtype);

			BasicBlock* setnothingrefflagblock = BasicBlock::Create(Context, "setnothingrefflag", BridgeFunction);
			BasicBlock* skipblock = BasicBlock::Create(Context, "skip", BridgeFunction);

			Value* providednothingflag = Builder.CreateICmpEQ(reftype, ConstantInt::get(Type::getInt32Ty(Context), Metadata::EpochType_Nothing));
			Builder.CreateCondBr(providednothingflag, setnothingrefflagblock, skipblock);

			Builder.SetInsertPoint(setnothingrefflagblock);
			Builder.CreateBr(skipblock);

			Builder.SetInsertPoint(skipblock);
			Builder.CreateStore(Builder.CreateAdd(Builder.CreateLoad(stackoffsettracker), ConstantInt::get(Type::getInt32Ty(Context), sizeof(Metadata::EpochTypeID))), stackoffsettracker);


			BasicBlock* handlesumtypeblock = BasicBlock::Create(Context, "handlesumtype", BridgeFunction);

			Value* providedtypefamily = Builder.CreateAnd(Builder.CreateLoad(providedtype), 0xff000000);
			Value* issumtype = Builder.CreateICmpEQ(providedtypefamily, ConstantInt::get(Type::getInt32Ty(Context), Metadata::EpochTypeFamily_SumType));
			Builder.CreateCondBr(issumtype, handlesumtypeblock, checkmatchblock);

			Builder.SetInsertPoint(handlesumtypeblock);
			Value* reftypeptr = Builder.CreateGEP(reftarget, ConstantInt::get(Type::getInt32Ty(Context), static_cast<uint64_t>(-(signed)sizeof(Metadata::EpochTypeID))));
			reftype = Builder.CreateLoad(Builder.CreatePointerCast(reftypeptr, Type::getInt32PtrTy(Context)));
			Builder.CreateStore(reftype, providedtype);
			Builder.CreateStore(reftarget, parampayloadptr);
			Builder.CreateBr(checkmatchblock);
		}

		Builder.SetInsertPoint(checkmatchblock);

		Value* nomatch = Builder.CreateICmpNE(Builder.CreateLoad(providedtype), ConstantInt::get(Type::getInt32Ty(Context), expecttype));
		Value* notexpectsumtype = ConstantInt::get(Type::getInt1Ty(Context), Metadata::GetTypeFamily(expecttype) != Metadata::EpochTypeFamily_SumType);

		BasicBlock* setflagsblock = BasicBlock::Create(Context, "setflags", BridgeFunction);
		BasicBlock* nextparamblock = BasicBlock::Create(Context, "nextparam", BridgeFunction);

		Value* bailflag = Builder.CreateAnd(nomatch, notexpectsumtype);
		Builder.CreateCondBr(bailflag, nexttypematcher, setflagsblock);

		Builder.SetInsertPoint(setflagsblock);
		BasicBlock* moreflagsblock = BasicBlock::Create(Context, "moreflags", BridgeFunction);
		Builder.CreateBr(moreflagsblock);
					
		Builder.SetInsertPoint(moreflagsblock);
		BasicBlock* setnothingflagblock = BasicBlock::Create(Context, "setnothingflag", BridgeFunction);
		Value* providednothingflag = Builder.CreateICmpEQ(Builder.CreateLoad(providedtype), ConstantInt::get(Type::getInt32Ty(Context), Metadata::EpochType_Nothing));
		Builder.CreateCondBr(providednothingflag, setnothingflagblock, nextparamblock);
		Builder.SetInsertPoint(setnothingflagblock);
		Builder.CreateBr(nextparamblock);

		Builder.SetInsertPoint(nextparamblock);

		if(expecttype != Metadata::EpochType_Nothing)
		{
			if(expectref)
			{
				Value* actualparam = Builder.CreatePointerCast(Builder.CreateLoad(parampayloadptr), Generator.GetLLVMType(expecttype)->getPointerTo());
				actualparams.push_back(actualparam);
			}
			else
			{
				Value* actualparam = Builder.CreatePointerCast(Builder.CreateLoad(parampayloadptr), Generator.GetLLVMType(expecttype)->getPointerTo());
				actualparams.push_back(Builder.CreateLoad(actualparam));
			}
		}
		else
			actualparams.push_back(ConstantInt::get(Type::getInt32Ty(Context), 0));
	}

	BasicBlock* invokeblock = BasicBlock::Create(Context, "invokesuccess", BridgeFunction);
	Builder.CreateBr(invokeblock);

	Builder.SetInsertPoint(invokeblock);

	Function* targetinnerfunc = Generator.GetGeneratedFunction(funcname, matchoffset);

	std::vector<Value*> resolvedargs;
	resolvedargs.push_back(VMContextPtr);
	for(std::vector<Value*>::const_reverse_iterator argiter = actualparams.rbegin(); argiter != actualparams.rend(); ++argiter)
		resolvedargs.push_back(*argiter);

	TypeMatchRetVal = Builder.CreateCall(targetinnerfunc, resolvedargs);

	Builder.CreateStore(Builder.CreateLoad(stackoffsettracker), TypeMatchConsumedBytes);

	// Fixup VM stack with return value
	Type* matchrettype = TypeMatchRetVal->getType();
	if(matchrettype != Type::getVoidTy(Context))
	{
		Value* offset = Builder.CreateSub(Builder.CreateLoad(TypeMatchConsumedBytes), ConstantInt::get(Type::getInt32Ty(Context), 4));
		Value* stackptr = Builder.CreateLoad(Builder.CreateLoad(PStackPtr));
		Value* retstackptr = Builder.CreateGEP(stackptr, offset);
		Builder.CreateStore(TypeMatchRetVal, Builder.CreatePointerCast(retstackptr, matchrettype->getPointerTo()));
		Builder.CreateStore(retstackptr, Builder.CreateLoad(PStackPtr));
	}
	else
	{
		Value* offset = Builder.CreateLoad(TypeMatchConsumedBytes);
		Value* stackptr = Builder.CreateLoad(Builder.CreateLoad(PStackPtr));
		Value* retstackptr = Builder.CreateGEP(stackptr, offset);
		Builder.CreateStore(retstackptr, Builder.CreateLoad(PStackPtr));
	}
				
	Builder.CreateRetVoid();
	Builder.SetInsertPoint(nexttypematcher);
}


void NativeCodeGenerator::AddNativeTypeMatcher(size_t beginoffset, size_t endoffset)
{
	Function* matcherfunction = Builder.GetInsertBlock()->getParent();

	std::vector<Value*> reftypes;
	std::vector<Value*> reftargets;

	std::vector<std::vector<Value*> > parampayloadptrs;
	std::vector<std::vector<Value*> > providedtypeholders;

	unsigned typematchindex = 0;
	StringHandle entityname = 0;

	// The purpose of this loop is to hoist all stack allocations from the inner
	// type matching basic blocks out to the entry block. This allows the LLVM
	// optimizer to convert the allocas to registers, which eliminates dynamic
	// stack resizing during the type match process - a solid performance win.
	for(size_t offset = beginoffset; offset <= endoffset; )
	{
		Bytecode::Instruction instruction = Bytecode[offset++];
		switch(instruction)
		{
		// Need to handle this so we can skip through the byte stream correctly
		case Bytecode::Instructions::BeginEntity:
			Fetch<Integer32>(Bytecode, offset);
			Fetch<StringHandle>(Bytecode, offset);
			break;

		// Ignore these for now
		case Bytecode::Instructions::EndEntity:
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
						Value* v = Builder.CreatePointerCast(parampayloadptrs[typematchindex][i], GetLLVMType(expecttype)->getPointerTo()->getPointerTo());
						v = Builder.CreateLoad(v);

						if(!expectref)
							v = Builder.CreateLoad(v);

						actualparams.push_back(v);
					}
					else
					{
						actualparams.push_back(ConstantInt::get(Type::getInt32Ty(Data->Context), 0xbaadf00d));
					}
				}

				Function* targetinnerfunc = GetGeneratedFunction(funcname, matchoffset);

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

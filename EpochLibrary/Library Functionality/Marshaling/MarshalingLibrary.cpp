//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for data/code marshaling
//

#include "pch.h"

#include "Library Functionality/Marshaling/MarshalingLibrary.h"

#include "Utility/StringPool.h"
#include "Utility/NoDupeMap.h"

#include "Runtime/Runtime.h"
#include "Runtime/Marshaling.h"

#include "Metadata/TypeInfo.h"


extern Runtime::ExecutionContext* GlobalExecutionContext;


namespace
{

	StringHandle SizeOfHandle = 0;
	StringHandle MarshalStructureHandle = 0;

	StringHandle WriteBufferHandle = 0;
	StringHandle WriteBufferStringHandle = 0;
	StringHandle WriteBufferMultipleHandle = 0;


	bool SizeOfJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* c = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::ConstantInt* cint = llvm::dyn_cast<llvm::ConstantInt>(c);
		size_t vartarget = static_cast<size_t>(cint->getValue().getLimitedValue());

		size_t size = context.Generator->ExecContext.GetStructureDefinition(context.CurrentScope->GetVariableTypeByID(vartarget)).GetMarshaledSize();
		llvm::ConstantInt* csize = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*reinterpret_cast<llvm::LLVMContext*>(context.Context)), size);

		context.ValuesOnStack.push(csize);

		return true;
	}

	bool MarshalStructureJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* ptr = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::Value* c = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::ConstantInt* cint = llvm::dyn_cast<llvm::ConstantInt>(c);
		size_t vartarget = static_cast<size_t>(cint->getValue().getLimitedValue());

		size_t ignored;
		size_t index = context.CurrentScope->FindVariable(vartarget, ignored);

		context.ValuesOnStack.push(ptr);
		context.ValuesOnStack.push(reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateLoad(context.VariableMap[index]));

		return false;		// Don't stop looking for helpers!
	}

	bool WriteBufferJIT(JIT::JITContext& context, bool)
	{
		llvm::Value* byte = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::Value* offset = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::Value* pbufferhandle = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::Value* bufferhandle = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateLoad(pbufferhandle);

		llvm::Value* bufferptr = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateCall((*context.BuiltInFunctions)[JIT::JITFunc_Runtime_GetBuffer], bufferhandle);
		llvm::Value* castbufferptr = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreatePointerCast(bufferptr, llvm::Type::getInt8PtrTy(*reinterpret_cast<llvm::LLVMContext*>(context.Context)));

		llvm::Value* gep = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateGEP(castbufferptr, offset);
		llvm::Value* truncated = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateCast(llvm::Instruction::Trunc, byte, llvm::Type::getInt8Ty(*reinterpret_cast<llvm::LLVMContext*>(context.Context)));
		reinterpret_cast<llvm::IRBuilder<>*>(context.Builder)->CreateStore(truncated, gep);

		return true;
	}

	bool WriteBufferStringJIT(JIT::JITContext& context, bool)
	{
		llvm::IRBuilder<>* builder = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder);

		llvm::Value* length = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::Value* byteptr = builder->CreateCall((*context.BuiltInFunctions)[JIT::JITFunc_Runtime_GetString], context.ValuesOnStack.top());
		context.ValuesOnStack.pop();

		llvm::Value* offset = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::Value* pbufferhandle = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::Value* bufferhandle = builder->CreateLoad(pbufferhandle);

		llvm::Value* bufferptr = builder->CreateCall((*context.BuiltInFunctions)[JIT::JITFunc_Runtime_GetBuffer], bufferhandle);
		llvm::Value* castbufferptr = builder->CreatePointerCast(bufferptr, llvm::Type::getInt8PtrTy(*reinterpret_cast<llvm::LLVMContext*>(context.Context)));

		llvm::Value* counter = builder->CreateAlloca(llvm::Type::getInt32Ty(*reinterpret_cast<llvm::LLVMContext*>(context.Context)));
		builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*reinterpret_cast<llvm::LLVMContext*>(context.Context)), 0), counter);

		llvm::BasicBlock* loopcheck = llvm::BasicBlock::Create(*reinterpret_cast<llvm::LLVMContext*>(context.Context), "memcpycheck", context.InnerFunction);
		llvm::BasicBlock* loopbody = llvm::BasicBlock::Create(*reinterpret_cast<llvm::LLVMContext*>(context.Context), "memcpyloop", context.InnerFunction);
		llvm::BasicBlock* loopexit = llvm::BasicBlock::Create(*reinterpret_cast<llvm::LLVMContext*>(context.Context), "memcpyexit", context.InnerFunction);

		builder->CreateBr(loopcheck);
		builder->SetInsertPoint(loopcheck);

		llvm::Value* counternotdone = builder->CreateICmpSLT(builder->CreateLoad(counter), length, "checksize");
		builder->CreateCondBr(counternotdone, loopbody, loopexit);

		builder->SetInsertPoint(loopbody);
		llvm::Value* origingep = builder->CreateGEP(byteptr, builder->CreateLoad(counter));
		llvm::Value* byte = builder->CreateLoad(origingep);

		llvm::Value* gep = builder->CreateGEP(castbufferptr, builder->CreateAdd(builder->CreateLoad(counter), offset));
		llvm::Value* truncated = builder->CreateCast(llvm::Instruction::Trunc, byte, llvm::Type::getInt8Ty(*reinterpret_cast<llvm::LLVMContext*>(context.Context)));
		builder->CreateStore(truncated, gep);
		builder->CreateStore(builder->CreateAdd(builder->CreateLoad(counter), llvm::ConstantInt::get(llvm::Type::getInt32Ty(*reinterpret_cast<llvm::LLVMContext*>(context.Context)), 1)), counter);
		builder->CreateBr(loopcheck);

		builder->SetInsertPoint(loopexit);
		return true;
	}

	bool WriteBufferMultipleJIT(JIT::JITContext& context, bool)
	{
		llvm::IRBuilder<>* builder = reinterpret_cast<llvm::IRBuilder<>*>(context.Builder);

		llvm::Value* length = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::Value* byteptr = builder->CreateCast(llvm::Instruction::IntToPtr, context.ValuesOnStack.top(), llvm::Type::getInt8PtrTy(*reinterpret_cast<llvm::LLVMContext*>(context.Context)));
		context.ValuesOnStack.pop();

		llvm::Value* offset = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::Value* pbufferhandle = context.ValuesOnStack.top();
		context.ValuesOnStack.pop();

		llvm::Value* bufferhandle = builder->CreateLoad(pbufferhandle);

		//builder->CreateCall((*context.BuiltInFunctions)[JIT::JITFunc_Runtime_Break]);

		llvm::Value* bufferptr = builder->CreateCall((*context.BuiltInFunctions)[JIT::JITFunc_Runtime_GetBuffer], bufferhandle);
		llvm::Value* castbufferptr = builder->CreatePointerCast(bufferptr, llvm::Type::getInt8PtrTy(*reinterpret_cast<llvm::LLVMContext*>(context.Context)));

		llvm::Value* counter = builder->CreateAlloca(llvm::Type::getInt32Ty(*reinterpret_cast<llvm::LLVMContext*>(context.Context)));
		builder->CreateStore(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*reinterpret_cast<llvm::LLVMContext*>(context.Context)), 0), counter);

		llvm::BasicBlock* loopcheck = llvm::BasicBlock::Create(*reinterpret_cast<llvm::LLVMContext*>(context.Context), "memcpycheck", context.InnerFunction);
		llvm::BasicBlock* loopbody = llvm::BasicBlock::Create(*reinterpret_cast<llvm::LLVMContext*>(context.Context), "memcpyloop", context.InnerFunction);
		llvm::BasicBlock* loopexit = llvm::BasicBlock::Create(*reinterpret_cast<llvm::LLVMContext*>(context.Context), "memcpyexit", context.InnerFunction);

		builder->CreateBr(loopcheck);
		builder->SetInsertPoint(loopcheck);

		llvm::Value* counternotdone = builder->CreateICmpSLT(builder->CreateLoad(counter), length, "checksize");
		builder->CreateCondBr(counternotdone, loopbody, loopexit);

		builder->SetInsertPoint(loopbody);
		llvm::Value* origingep = builder->CreateGEP(byteptr, builder->CreateLoad(counter));
		llvm::Value* byte = builder->CreateLoad(origingep);

		llvm::Value* gep = builder->CreateGEP(castbufferptr, builder->CreateAdd(builder->CreateLoad(counter), offset));
		llvm::Value* truncated = builder->CreateCast(llvm::Instruction::Trunc, byte, llvm::Type::getInt8Ty(*reinterpret_cast<llvm::LLVMContext*>(context.Context)));
		builder->CreateStore(truncated, gep);
		builder->CreateStore(builder->CreateAdd(builder->CreateLoad(counter), llvm::ConstantInt::get(llvm::Type::getInt32Ty(*reinterpret_cast<llvm::LLVMContext*>(context.Context)), 1)), counter);
		builder->CreateBr(loopcheck);

		builder->SetInsertPoint(loopexit);
		return true;
	}

}


//
// Bind the library to a function metadata table
//
void MarshalingLibrary::RegisterLibraryFunctions(FunctionSignatureSet& signatureset)
{
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier, false);
		signature.SetReturnType(Metadata::EpochType_Integer);
		AddToMapNoDupe(signatureset, std::make_pair(SizeOfHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"identifier", Metadata::EpochType_Identifier, false);
		signature.AddParameter(L"pointer", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Void);
		AddToMapNoDupe(signatureset, std::make_pair(MarshalStructureHandle, signature));
	}

	{
		FunctionSignature signature;
		signature.AddParameter(L"buffer", Metadata::EpochType_Buffer, true);
		signature.AddParameter(L"index", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"byte", Metadata::EpochType_Integer, false);		// TODO - should be a single byte
		signature.SetReturnType(Metadata::EpochType_Void);
		AddToMapNoDupe(signatureset, std::make_pair(WriteBufferHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"buffer", Metadata::EpochType_Buffer, true);
		signature.AddParameter(L"index", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"bytes", Metadata::EpochType_String, false);
		signature.AddParameter(L"size", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Void);
		AddToMapNoDupe(signatureset, std::make_pair(WriteBufferStringHandle, signature));
	}
	{
		FunctionSignature signature;
		signature.AddParameter(L"buffer", Metadata::EpochType_Buffer, true);
		signature.AddParameter(L"index", Metadata::EpochType_Integer, false);
		signature.AddParameter(L"bytes", Metadata::EpochType_Integer, false);		// TODO - should be a byte pointer
		signature.AddParameter(L"size", Metadata::EpochType_Integer, false);
		signature.SetReturnType(Metadata::EpochType_Void);
		AddToMapNoDupe(signatureset, std::make_pair(WriteBufferMultipleHandle, signature));
	}
}


void MarshalingLibrary::PoolStrings(StringPoolManager& stringpool)
{
	SizeOfHandle = stringpool.Pool(L"sizeof");
	MarshalStructureHandle = stringpool.Pool(L"marshalstructure");
	WriteBufferHandle = stringpool.Pool(L"writebuffer");
	WriteBufferStringHandle = stringpool.Pool(L"writebuffer@@string");
	WriteBufferMultipleHandle = stringpool.Pool(L"writebuffer@@multiple");
}


void MarshalingLibrary::RegisterJITTable(JIT::JITTable& table)
{
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(SizeOfHandle, &SizeOfJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(MarshalStructureHandle, &MarshalStructureJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(WriteBufferHandle, &WriteBufferJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(WriteBufferStringHandle, &WriteBufferStringJIT));
	AddToMapNoDupe(table.InvokeHelpers, std::make_pair(WriteBufferMultipleHandle, &WriteBufferMultipleJIT));

	AddToMapNoDupe(table.LibraryExports, std::make_pair(MarshalStructureHandle, "EpochLib_MarshalStructure"));
}

void MarshalingLibrary::RegisterLibraryOverloads(OverloadMap& overloadmap)
{
	overloadmap[WriteBufferHandle].insert(WriteBufferStringHandle);
	overloadmap[WriteBufferHandle].insert(WriteBufferMultipleHandle);
}

extern "C" void EpochLib_MarshalStructure(StructureHandle activestruct, const Byte* buffer)
{
	const StructureDefinition& def = GlobalExecutionContext->FindStructureMetadata(activestruct).Definition;
	Runtime::MarshalBufferIntoStructureData(*GlobalExecutionContext, activestruct, def, buffer);
}


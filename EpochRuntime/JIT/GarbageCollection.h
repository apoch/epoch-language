//
// The Epoch Language Project
// EPOCHRUNTIME Runtime Library
//
// Garbage collection declarations
//

#pragma once


// Forward declarations
namespace llvm
{
	class Function;
}


namespace EpochGC
{

	void ClearGCContextInfo();

	void SetGCFunctionBounds(const llvm::Function* func, void* start, size_t size);

	void RegisterGlobalVariable(void* ptr, Metadata::EpochTypeID type);

}


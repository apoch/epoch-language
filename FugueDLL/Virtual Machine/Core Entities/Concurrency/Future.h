//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper class for futures
//

#pragma once


// Dependencies
#include "Utility/Types/EpochTypeIDs.h"
#include "Virtual Machine/Core Entities/RValue.h"


class StackSpace;


namespace VM
{

	// Forward declarations
	class Operation;
	class ScopeDescription;

	typedef std::auto_ptr<VM::Operation> OperationPtr;


	//
	// Wrapper class for representing a future
	//
	// A future is simply a value that is computed asynchronously.
	// Code can retrieve this value at any time, but attempting to
	// read the value before it is finished computing results in the
	// calling thread blocking until the value is ready.
	//
	class Future
	{
	// Construction
	public:
		Future(OperationPtr op);
		~Future();

	// Value retrieval
	public:
		EpochVariableTypeID GetType(const VM::ScopeDescription& scope) const;
		RValuePtr GetValue() const;

	// Value storage
	public:
		void SetResult(RValuePtr value);

	// Operation retrieval
	public:
		virtual Operation* GetNestedOperation() const
		{ return Op.get(); }

	// Internal tracking
	private:
		OperationPtr Op;
		RValuePtr Result;
		HANDLE CompletionEventHandle;
	};

}

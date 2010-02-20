//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Virtual machine operations for working with built-in containers
//

#pragma once


// Dependencies
#include "Virtual Machine/Core Entities/Operation.h"


namespace VM
{

	namespace Operations
	{

		//
		// This operation constructs an anonymous array container on the stack
		// It is primarily useful for passing an indeterminate number of values
		// into a variadic function, such as or() or add().
		//
		class ConsArray : public Operation, public SelfAware<ConsArray>
		{
		// Construction
		public:
			ConsArray(size_t numentries, EpochVariableTypeID elementtype);

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Array; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return NumEntries; }

		// Queries
		public:
			EpochVariableTypeID GetElementType() const
			{ return ElementType; }

			size_t GetNumEntries() const
			{ return NumEntries; }

		// Internal tracking
		private:
			size_t NumEntries;
			EpochVariableTypeID ElementType;
		};

	}

}


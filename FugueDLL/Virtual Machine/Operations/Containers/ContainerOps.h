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
		// This operation constructs an anonymous list container on the stack
		// It is primarily useful for passing an indeterminate number of values
		// into a variadic function, such as or() or add().
		//
		class ConsList : public Operation, public SelfAware<ConsList>
		{
		// Construction
		public:
			ConsList(size_t numentries, EpochVariableTypeID elementtype);

		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_List; }

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


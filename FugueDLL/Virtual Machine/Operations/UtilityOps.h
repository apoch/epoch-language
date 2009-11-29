//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Assorted utility operations used by the VM
//

#pragma once


// Dependencies
#include "Virtual Machine/Core Entities/Operation.h"


namespace VM
{

	namespace Operations
	{

		//
		// Operation which does nothing
		//
		// No-ops are generally used by the parser to indicate that an actual instruction
		// could not be generated, usually due to the occurrence of a parsing error.
		//
		class NoOp : public Operation, public SelfAware<NoOp>
		{
		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
			{ }

			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
			{ return RValuePtr(new NullRValue); }

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Null; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }
		};

	}

}


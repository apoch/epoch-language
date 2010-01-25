//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operations for working with futures
//

#pragma once


// Dependencies
#include "Virtual Machine/Core Entities/Operation.h"


namespace VM
{

	namespace Operations
	{

		//
		// Operation for forking a task that computes a future
		//
		class ForkFuture : public Operation, public SelfAware<ForkFuture>
		{
		// Construction
		public:
			ForkFuture(const std::wstring& varname, EpochVariableTypeID type, bool usethreadpool)
				: Type(type),
				  VarName(varname),
				  UseThreadPool(usethreadpool)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return Type; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

		// Additional queries
		public:
			EpochVariableTypeID GetType() const			{ return Type; }
			const std::wstring& GetVarName() const		{ return VarName; }
			bool UsesThreadPool() const					{ return UseThreadPool; }

		// Internal tracking
		private:
			EpochVariableTypeID Type;
			const std::wstring& VarName;
			bool UseThreadPool;
		};

	}

}


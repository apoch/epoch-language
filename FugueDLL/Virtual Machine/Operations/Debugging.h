//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operations for debugging Epoch programs
//

#pragma once


// Dependencies
#include "Virtual Machine/Core Entities/Operation.h"


namespace VM
{

	namespace Operations
	{

		//
		// Write an expression which evaluates to a string to the
		// debugger. The expression should be pushed onto the stack
		// prior to invoking the operation.
		//
		class DebugWriteStringExpression : public Operation, public SelfAware<DebugWriteStringExpression>
		{
		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);
			
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Null; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 1; }
		};


		//
		// Perform a blocking read from the debugger UI, and
		// return the resulting string.
		//
		class DebugReadStaticString : public Operation, public SelfAware<DebugReadStaticString>
		{
		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_String; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }
		};


		//
		// Deliberately crash the VM for testing for memory leaks etc.
		//
		class DebugCrashVM : public Operation, public SelfAware<DebugCrashVM>
		{
		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context)
			{ throw Exception("VM deliberately crashed"); }

			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context)
			{ throw Exception("VM deliberately crashed"); }

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Null; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }
		};

	}

}


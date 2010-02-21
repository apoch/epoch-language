//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operations for working with variables and their values
//

#pragma once

// Dependencies
#include "Virtual Machine/Core Entities/Operation.h"


namespace VM
{

	// Forward declarations
	class ScopeDescription;
	class FunctionBase;

	namespace Operations
	{

		//
		// Operation for assigning an r-value into a variable (l-value)
		//
		class AssignValue : public Operation, public SelfAware<AssignValue>
		{
		// Construction
		public:
			AssignValue(const std::wstring& varname);

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const;

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const;

		// Additional value retrieval
		public:
			const std::wstring& GetAssociatedIdentifier() const
			{ return VarName; }

		// Traversal
		public:
			virtual Traverser::Payload GetNodeTraversalPayload(const VM::ScopeDescription* scope) const;

		// Internal tracking
		private:
			const std::wstring& VarName;
		};

		//
		// Operation for initializing a variable
		// The main distinction between this and AssignValue is that any
		// initializatin operations are directed NOT to treat the existing
		// stack/heap storage as valid data.
		//
		class InitializeValue : public Operation, public SelfAware<InitializeValue>
		{
		// Construction
		public:
			InitializeValue(const std::wstring& varname);

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const;

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const;

			const std::wstring& GetAssociatedIdentifier() const
			{ return VarName; }

		// Traversal
		public:
			virtual Traverser::Payload GetNodeTraversalPayload(const VM::ScopeDescription* scope) const;

		// Internal tracking
		private:
			const std::wstring& VarName;
		};

		//
		// Operation for retrieving a variable's value
		//
		class GetVariableValue : public Operation, public SelfAware<GetVariableValue>
		{
		// Construction
		public:
			GetVariableValue(const std::wstring& varname);

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const;

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

		// Additional queries
		public:
			const std::wstring& GetAssociatedIdentifier() const
			{ return VarName; }

		// Traversal
		public:
			virtual Traverser::Payload GetNodeTraversalPayload(const VM::ScopeDescription* scope) const;
			
		// Internal tracking
		private:
			const std::wstring& VarName;
		};


		//
		// Operation for retrieving the storage size of a variable
		//
		class SizeOf : public Operation, public SelfAware<SizeOf>
		{
		// Construction
		public:
			SizeOf(const std::wstring& varname)
				: VarName(varname)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);
			
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Integer; }
		
			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

			const std::wstring& GetAssociatedIdentifier() const
			{ return VarName; }

		// Traversal
		public:
			virtual Traverser::Payload GetNodeTraversalPayload(const VM::ScopeDescription* scope) const;

		// Internal tracking
		private:
			const std::wstring& VarName;
		};


		//
		// Operation which evaluates to a literal Integer constant.
		// This is mainly used for representing operations that can
		// be reduced to constants at compile/load time.
		//
		class IntegerConstant : public Operation, public SelfAware<IntegerConstant>
		{
		// Construction
		public:
			IntegerConstant(Integer32 value)
				: Value(value)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context)
			{ }

			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context)
			{ return RValuePtr(new IntegerRValue(Value)); }

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Integer; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

		// Traversal interface
		public:
			virtual Traverser::Payload GetNodeTraversalPayload(const VM::ScopeDescription* scope) const
			{
				Traverser::Payload payload;
				payload.SetValue(Value);
				payload.ParameterCount = GetNumParameters(*scope);
				return payload;
			}

		// Additional accessors
		public:
			static EpochVariableTypeID GetTypeStatic()
			{ return EpochVariableType_Integer; }

		// Internal tracking
		private:
			Integer32 Value;
		};

		//
		// Operation which evaluates to a literal Integer constant (16 bits wide)
		// This is mainly used for representing operations that can
		// be reduced to constants at compile/load time.
		//
		class Integer16Constant : public Operation, public SelfAware<Integer16Constant>
		{
		// Construction
		public:
			Integer16Constant(Integer16 value)
				: Value(value)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context)
			{ }

			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context)
			{ return RValuePtr(new Integer16RValue(Value)); }

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Integer16; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

		// Traversal interface
		public:
			virtual Traverser::Payload GetNodeTraversalPayload(const VM::ScopeDescription* scope) const
			{
				Traverser::Payload payload;
				payload.SetValue(Value);
				payload.ParameterCount = GetNumParameters(*scope);
				return payload;
			}

		// Additional accessors
		public:
			static EpochVariableTypeID GetTypeStatic()
			{ return EpochVariableType_Integer16; }

		// Internal tracking
		private:
			Integer16 Value;
		};

		//
		// Operation which evaluates to a literal real constant.
		// This is mainly used for representing operations that can
		// be reduced to constants at compile/load time.
		//
		class RealConstant : public Operation, public SelfAware<RealConstant>
		{
		// Construction
		public:
			RealConstant(Real value)
				: Value(value)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context)
			{ }

			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context)
			{ return RValuePtr(new RealRValue(Value)); }

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Real; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

		// Traversal interface
		public:
			virtual Traverser::Payload GetNodeTraversalPayload(const VM::ScopeDescription* scope) const
			{
				Traverser::Payload payload;
				payload.SetValue(Value);
				payload.ParameterCount = GetNumParameters(*scope);
				return payload;
			}

		// Additional accessors
		public:
			static EpochVariableTypeID GetTypeStatic()
			{ return EpochVariableType_Real; }

		// Internal tracking
		private:
			Real Value;
		};

		//
		// Operation which evaluates to a literal boolean constant.
		// This is mainly used for representing operations that can
		// be reduced to constants at compile/load time.
		//
		class BooleanConstant : public Operation, public SelfAware<BooleanConstant>
		{
		// Construction
		public:
			BooleanConstant(bool value)
				: Value(value)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context)
			{ }

			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context)
			{ return RValuePtr(new BooleanRValue(Value)); }

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Boolean; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

		// Traversal interface
		public:
			virtual Traverser::Payload GetNodeTraversalPayload(const VM::ScopeDescription* scope) const
			{
				Traverser::Payload payload;
				payload.SetValue(Value);
				payload.ParameterCount = GetNumParameters(*scope);
				return payload;
			}

		// Additional accessors
		public:
			static EpochVariableTypeID GetTypeStatic()
			{ return EpochVariableType_Boolean; }

		// Internal tracking
		private:
			bool Value;
		};

	}
}


//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operations for working with tuples
//

#pragma once


// Dependencies
#include "Virtual Machine/Core Entities/Operation.h"
#include "Virtual Machine/Core Entities/Types/Tuple.h"


namespace VM
{
	namespace Operations
	{

		//
		// Operation for reading a member out of a tuple
		//
		class ReadTuple : public Operation, public SelfAware<ReadTuple>
		{
		// Construction
		public:
			ReadTuple(const std::wstring& varname, const std::wstring& membername);

		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const;

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

			const std::wstring& GetAssociatedIdentifier() const
			{ return VarName; }

			const std::wstring& GetMemberName() const
			{ return MemberName; }

		// Internal tracking
		private:
			const std::wstring& VarName;
			const std::wstring& MemberName;
		};

		//
		// Operation for writing a tuple member's value
		//
		class AssignTuple : public Operation, public SelfAware<AssignTuple>
		{
		// Construction
		public:
			AssignTuple(const std::wstring& varname, const std::wstring& membername);

		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const;

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 1; }

		// Access to bound lvalue information
		public:
			const std::wstring& GetAssociatedIdentifier() const		{ return VarName; }
			const std::wstring& GetMemberName() const				{ return MemberName; }

		// Internal tracking
		private:
			const std::wstring& VarName;
			const std::wstring& MemberName;
		};

	}
}

//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operations for working with structures
//

#pragma once


// Dependencies
#include "Virtual Machine/Core Entities/Operation.h"
#include "Virtual Machine/Core Entities/Types/Structure.h"


namespace VM
{
	namespace Operations
	{

		//
		// Operation for reading a member out of a structure
		//
		class ReadStructure : public Operation, public SelfAware<ReadStructure>
		{
		// Allow nested readstructure ops to peek into us for type information
		public:
			friend class ReadStructureIndirect;

		// Construction
		public:
			ReadStructure(const std::wstring& varname, const std::wstring& membername);

		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const;

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 0; }

		// Additional inspection
		public:
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
		// Operation for reading a member out of a structure returned by an operation on the stack
		//
		class ReadStructureIndirect : public Operation, public SelfAware<ReadStructureIndirect>
		{
		// Construction
		public:
			ReadStructureIndirect(const std::wstring& membername, VM::Operation* priorop);

		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const;

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 1; }

		// Additional helpers
		public:
			IDType WalkInstructionsForReadStruct(const ScopeDescription& scope, Operation* op) const;
			
			const std::wstring& GetMemberName() const
			{ return MemberName; }

		// Internal tracking
		private:
			const std::wstring& MemberName;
			VM::Operation* PriorOp;
		};

		//
		// Operation for writing a structure member's value
		//
		class AssignStructure : public Operation, public SelfAware<AssignStructure>
		{
		// Construction
		public:
			AssignStructure(const std::wstring& varname, const std::wstring& membername);

		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const;

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 1; }

		// Additional inspection
		public:
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
		// Operation for writing a structure member's value
		// This variant allows us to write to reference-bound locations,
		// which makes nested structure manipulation possible.
		//
		class AssignStructureIndirect : public Operation, public SelfAware<AssignStructureIndirect>
		{
		// Construction
		public:
			AssignStructureIndirect(const std::wstring& membername);

		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const;

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 2; }

		// Additional inspection
		public:
			const std::wstring& GetMemberName() const
			{ return MemberName; }

		// Internal tracking
		private:
			const std::wstring& MemberName;
		};

		//
		// Operation for binding a temporary reference to a structure member.
		// Primarily used for working with nested structures.
		//
		class BindStructMemberReference : public Operation, public SelfAware<BindStructMemberReference>
		{
		// Construction
		public:
			BindStructMemberReference(const std::wstring& membername);
			BindStructMemberReference(const std::wstring& varname, const std::wstring& membername);

		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const;

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return (Chained ? 1 : 0); }

		// Additional inspection
		public:
			const std::wstring& GetAssociatedIdentifier() const
			{ return *VarName; }

			const std::wstring& GetMemberName() const
			{ return MemberName; }

			bool IsChained() const
			{ return Chained; }

		// Internal tracking
		private:
			const std::wstring* VarName;
			const std::wstring& MemberName;
			bool Chained;
		};

	}
}

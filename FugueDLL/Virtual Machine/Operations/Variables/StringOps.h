//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operations for working with strings
//

#pragma once

// Dependencies
#include "Virtual Machine/Core Entities/Operation.h"


namespace VM
{

	namespace Operations
	{

		//
		// Operation for concatenating two strings
		//
		class Concatenate : public Operation, public SelfAware<Concatenate>
		{
		// Construction
		public:
			Concatenate()
				: FirstIsList(true),
				  SecondIsList(false),
				  NumParams(1)
			{ }

			Concatenate(bool firstislist, bool secondislist)
				: FirstIsList(firstislist),
				  SecondIsList(secondislist),
				  NumParams(2)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);
			
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_String; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 2; }

		// List support
		public:
			void AddOperation(VM::Operation* op);
			void AddOperationToFront(VM::Operation* op);

		// Additional queries
		public:
			bool IsFirstList() const			{ return FirstIsList; }
			bool IsSecondList() const			{ return SecondIsList; }
			size_t GetNumParameters() const		{ return NumParams; }

		// Internal helpers
		private:
			std::wstring OperateOnList(StackSpace& stack) const;

		// Internal tracking
		private:
			bool FirstIsList;
			bool SecondIsList;
			unsigned NumParams;
		};


		//
		// Operation for retrieving the length of a string
		//
		class Length : public Operation, public SelfAware<Length>
		{
		// Construction
		public:
			Length(const std::wstring& varname)
				: VarName(varname)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context);
			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context);
			
			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Integer; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 1; }

			const std::wstring& GetAssociatedIdentifier() const
			{ return VarName; }

		// Traversal interface
		public:
			virtual Traverser::Payload GetNodeTraversalPayload() const
			{
				Traverser::Payload payload;
				payload.SetValue(VarName.c_str());
				payload.IsIdentifier = true;
				return payload;
			}

		// Internal tracking
		private:
			const std::wstring& VarName;
		};


	}

}


//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Built in comparison operators
//

#pragma once


// Dependencies
#include "Virtual Machine/Core Entities/Operation.h"


namespace VM
{
	// Forward declarations
	class ScopeDescription;

	namespace Operations
	{

		//
		// Base class for comparison operations; eliminates a minor
		// bit of duplication in the comparator code.
		//
		class Comparator : public Operation
		{
		// Construction
		public:
			Comparator(EpochVariableTypeID type)
				: Type(type)
			{ }

		// Operation interface (partial)
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult) = 0;
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult) = 0;

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return EpochVariableType_Boolean; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 2; }

		// Traversal
		public:
			virtual Traverser::Payload GetNodeTraversalPayload() const;

		// Internal tracking
		protected:
			EpochVariableTypeID Type;
		};

		//
		// Compare two values to see if they are equal
		//
		class IsEqual : public Comparator, public SelfAware<IsEqual>
		{
		// Construction
		public:
			IsEqual(EpochVariableTypeID type)
				: Comparator(type)
			{ }

		// Comparator/operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
		};

		//
		// Compare two values to see if they are inequal
		//
		class IsNotEqual : public Comparator, public SelfAware<IsNotEqual>
		{
		// Construction
		public:
			IsNotEqual(EpochVariableTypeID type)
				: Comparator(type)
			{ }

		// Comparator/operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
		};

		//
		// Compare two values to see if one is greater than the other
		//
		class IsGreater : public Comparator, public SelfAware<IsGreater>
		{
		// Construction
		public:
			IsGreater(EpochVariableTypeID type)
				: Comparator(type)
			{ }

		// Comparator/operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
		};

		//
		// Compare two values to see if one is greater than or equal to the other
		//
		class IsGreaterOrEqual : public Comparator, public SelfAware<IsGreaterOrEqual>
		{
		// Construction
		public:
			IsGreaterOrEqual(EpochVariableTypeID type)
				: Comparator(type)
			{ }

		// Comparator/operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
		};

		//
		// Compare two values to see if one is less than the other
		//
		class IsLesser : public Comparator, public SelfAware<IsLesser>
		{
		// Construction
		public:
			IsLesser(EpochVariableTypeID type)
				: Comparator(type)
			{ }

		// Comparator/operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
		};


		//
		// Compare two values to see if one is less than or equal to the other
		//
		class IsLesserOrEqual : public Comparator, public SelfAware<IsLesserOrEqual>
		{
		// Construction
		public:
			IsLesserOrEqual(EpochVariableTypeID type)
				: Comparator(type)
			{ }

		// Comparator/operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
		};
	}

}


//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Built-in operators and operations. Epoch supports a fairly
// standard set of arithmetic operators and first-class data
// operations.
//

#pragma once


// Dependencies
#include "Virtual Machine/Core Entities/Operation.h"
#include "Virtual Machine/Core Entities/Variables/Variable.h"


namespace VM
{
	// Forward declarations
	class ScopeDescription;

	namespace Operations
	{

		//
		// Arithmetic operations available
		//
		enum ArithmeticOpType
		{
			Arithmetic_Add,
			Arithmetic_Subtract,
			Arithmetic_Multiply,
			Arithmetic_Divide
		};

		//
		// Base arithmetic operation
		//
		template<ArithmeticOpType OpType, class VarType, class RValueType>
		class ArithmeticOp : public Operation, public SelfAware<ArithmeticOp<OpType, VarType, RValueType> >
		{
		// Construction
		public:
			ArithmeticOp()
				: FirstIsList(true),
				  SecondIsList(false),
				  NumParams(1)
			{ }

			ArithmeticOp(bool firstislist, bool secondislist)
				: FirstIsList(firstislist),
				  SecondIsList(secondislist),
				  NumParams(2)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return VarType::GetStaticType(); }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return NumParams; }

		// Additional queries
		public:
			bool IsFirstList() const			{ return FirstIsList; }
			bool IsSecondList() const			{ return SecondIsList; }
			size_t GetNumParameters() const		{ return NumParams; }

		// Internal helpers
		protected:
			typename VarType::BaseStorage OperateOnList(StackSpace& stack) const;

		// Internal tracking
		protected:
			bool FirstIsList;
			bool SecondIsList;
			unsigned NumParams;
		};


		class Negate : public Operation, public SelfAware<Negate>
		{
		// Construction
		public:
			Negate(VM::EpochVariableTypeID type)
				: Type(type)
			{ }

		// Operation interface
		public:
			virtual void ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);
			virtual RValuePtr ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult);

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return Type; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 1; }

		// Internal tracking
		private:
			VM::EpochVariableTypeID Type;
		};


		//
		// Handy type shortcuts
		//
		typedef ArithmeticOp<Arithmetic_Add, IntegerVariable, IntegerRValue> SumIntegers;
		typedef ArithmeticOp<Arithmetic_Add, Integer16Variable, Integer16RValue> SumInteger16s;
		typedef ArithmeticOp<Arithmetic_Add, RealVariable, RealRValue> SumReals;
		typedef ArithmeticOp<Arithmetic_Subtract, IntegerVariable, IntegerRValue> SubtractIntegers;
		typedef ArithmeticOp<Arithmetic_Subtract, Integer16Variable, Integer16RValue> SubtractInteger16s;
		typedef ArithmeticOp<Arithmetic_Subtract, RealVariable, RealRValue> SubtractReals;
		typedef ArithmeticOp<Arithmetic_Multiply, IntegerVariable, IntegerRValue> MultiplyIntegers;
		typedef ArithmeticOp<Arithmetic_Multiply, Integer16Variable, Integer16RValue> MultiplyInteger16s;
		typedef ArithmeticOp<Arithmetic_Multiply, RealVariable, RealRValue> MultiplyReals;
		typedef ArithmeticOp<Arithmetic_Divide, IntegerVariable, IntegerRValue> DivideIntegers;
		typedef ArithmeticOp<Arithmetic_Divide, Integer16Variable, Integer16RValue> DivideInteger16s;
		typedef ArithmeticOp<Arithmetic_Divide, RealVariable, RealRValue> DivideReals;
	}
}


#include "Arithmetic.inl"


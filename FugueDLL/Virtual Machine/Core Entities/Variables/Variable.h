//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Interfaces for storing variables.
//

#pragma once


// Dependencies
#include "Utility/Memory/Stack.h"
#include "Virtual Machine/Core Entities/RValue.h"
#include "Utility/Types/EpochTypeIDs.h"
#include "Virtual Machine/VMExceptions.h"


namespace VM
{

	// Forward declarations
	class TupleType;
	class FunctionBase;


	//
	// Base class for storing variables
	//
	// Note that we do not use virtual functions for variable storage.
	// This ensures a minimum of overhead per variable, and simplifies
	// the lexical scope container somewhat.
	//
	// A consequence if this is that derived variable classes should
	// NOT have any member data! Anything that needs to be stored by
	// the variable should be kept in the storage slot provided. See
	// the Array variable class for an example. This prevents slicing
	// of the variable objects when they are stored by value.
	//
	class Variable
	{
	// Construction - we do not allow construction of untyped variables; see other classes
	protected:
		Variable(EpochVariableTypeID vartype, void* storage)
			: VarType(vartype), Storage(storage)
		{ }

	// Type retrieval
	public:
		EpochVariableTypeID GetType() const			{ return VarType; }

	// Storage space management
	public:
		void* GetStorage() const					{ return Storage; }

		void BindToStorage(void* storage)
		{
			Storage = storage;
		}

	// Casting helpers for brevity
	public:
		template <class VarClass>
		VarClass& CastTo()
		{ return TypesManager::CastVariable<VarClass>(*this); }

		template <class VarClass>
		const VarClass& CastTo() const
		{ return TypesManager::CastVariable<VarClass>(*this); }
		
	// Internal tracking
	protected:
		void* Storage;
		EpochVariableTypeID VarType;
	};


	//
	// Template for simple built-in types
	//
	// This generally corresponds directly to types that are
	// built into the host language, C++.
	//
	template<typename BaseStorageT, EpochVariableTypeID EpochTypeID, class RValueType>
	class SimpleVariable : public Variable
	{
	// Handy type shortcut
	public:
		typedef BaseStorageT BaseStorage;

	// Construction
	public:
		SimpleVariable(void* storage)
			: Variable(EpochTypeID, storage)
		{
		}

		SimpleVariable(void* storage, RValuePtr rvalue)
			: Variable(EpochTypeID, storage)
		{
			SetValue(rvalue->CastTo<RValueType>().GetValue());
		}

	// Variable interface
	public:
		BaseStorage GetValue() const
		{ return *(reinterpret_cast<BaseStorage*>(Storage)); }

		RValuePtr GetAsRValue() const
		{ return RValuePtr(new RValueType(GetValue())); }

		BaseStorage SetValue(BaseStorage newvalue)
		{
			BaseStorage* pValue = reinterpret_cast<BaseStorage*>(Storage);
			return *pValue = newvalue;
		}

		size_t BindToStack(StackSpace& stack)
		{
			stack.Push(GetStorageSize());
			Storage = stack.GetCurrentTopOfStack();
			return GetStorageSize();
		}

	// Shared storage size/type retrieval
	public:
		static size_t GetStorageSize()
		{ return sizeof(BaseStorage); }

		static size_t GetBaseStorageSize()
		{ return GetStorageSize(); }

		static EpochVariableTypeID GetStaticType()
		{ return EpochTypeID; }
	};


	// Type shortcuts for basic built-in variables
	typedef SimpleVariable<Integer32, EpochVariableType_Null, NullRValue> NullVariable;
	typedef SimpleVariable<Integer32, EpochVariableType_Integer, IntegerRValue> IntegerVariable;
	typedef SimpleVariable<Real, EpochVariableType_Real, RealRValue> RealVariable;
	typedef SimpleVariable<bool, EpochVariableType_Boolean, BooleanRValue> BooleanVariable;
	typedef SimpleVariable<Integer16, EpochVariableType_Integer16, Integer16RValue> Integer16Variable;

	typedef SimpleVariable<Variable*, EpochVariableType_Null, NullRValue> ReferenceBinding;
	typedef SimpleVariable<FunctionBase*, EpochVariableType_Function, FunctionRValue> FunctionBinding;
	typedef SimpleVariable<void*, EpochVariableType_Address, AddressRValue> AddressVariable;
	typedef SimpleVariable<TaskHandle, EpochVariableType_TaskHandle, TaskHandleRValue> TaskHandleVariable;
}


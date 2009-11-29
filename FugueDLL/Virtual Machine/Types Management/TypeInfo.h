//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Central interface for retrieving information about types
//

#pragma once

// Dependencies
#include "Virtual Machine/Core Entities/Variables/Variable.h"
#include "Virtual Machine/Core Entities/Variables/StringVariable.h"
#include "Virtual Machine/Core Entities/Variables/BufferVariable.h"
#include "Virtual Machine/Core Entities/Variables/TupleVariable.h"
#include "Virtual Machine/Core Entities/Variables/StructureVariable.h"


namespace TypeInfo
{
	size_t GetStorageSize(VM::EpochVariableTypeID type);
	bool IsNumeric(VM::EpochVariableTypeID type);


#define DECLARE_TYPE(name, vartype, rvaltype)	\
	struct name									\
	{											\
		typedef VM::vartype VariableType;		\
		typedef VM::rvaltype RValueType;		\
	};											\


	DECLARE_TYPE(IntegerT, IntegerVariable, IntegerRValue)
	DECLARE_TYPE(Integer16T, Integer16Variable, Integer16RValue)
	DECLARE_TYPE(BooleanT, BooleanVariable, BooleanRValue)
	DECLARE_TYPE(StringT, StringVariable, StringRValue)
	DECLARE_TYPE(RealT, RealVariable, RealRValue)
	DECLARE_TYPE(FunctionT, FunctionBinding, FunctionRValue)
	DECLARE_TYPE(AddressT, AddressVariable, AddressRValue)
	DECLARE_TYPE(TaskHandleT, TaskHandleVariable, TaskHandleRValue)
	DECLARE_TYPE(ReferenceBindingT, ReferenceBinding, NullRValue)
	DECLARE_TYPE(FunctionBindingT, FunctionBinding, FunctionRValue)
	DECLARE_TYPE(TupleT, TupleVariable, TupleRValue)
	DECLARE_TYPE(StructureT, StructureVariable, StructureRValue)
	DECLARE_TYPE(BufferT, BufferVariable, BufferRValue)


#undef DECLARE_TYPE

}

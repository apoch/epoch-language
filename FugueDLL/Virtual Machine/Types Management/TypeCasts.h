//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Internal variable/r-value type casting support
//
// These stubs should not be used by code directly; instead, use support functions
// such as RValue::CastTo() and Variable::CastTo().
//

#pragma once


namespace VM
{

	// Forward declarations
	class RValue;
	class Variable;

	namespace TypesManager
	{

		//
		// Cast an untyped variable container to a specific variable type
		//
		template<class VariableClass>
		VariableClass& CastVariable(Variable& var)
		{
			if(var.GetType() != VariableClass::GetStaticType())
				throw ExecutionException("Invalid variable cast attempted!");

			return *(reinterpret_cast<VariableClass*>(&var));
		}

		template<class VariableClass>
		const VariableClass& CastVariable(const Variable& var)
		{
			if(var.GetType() != VariableClass::GetStaticType())
				throw ExecutionException("Invalid variable cast attempted!");

			return *(reinterpret_cast<const VariableClass*>(&var));
		}


		//
		// Cast an untyped r-value to a specific type
		//
		template<class RValueClass>
		const RValueClass& CastRValue(const RValue& rval)
		{
			if(rval.GetType() != RValueClass::GetType())
				throw ExecutionException("Invalid rvalue cast attempted!");

			return dynamic_cast<const RValueClass&>(rval);
		}

		template<class RValueClass>
		RValueClass& CastRValue(RValue& rval)
		{
			if(rval.GetType() != RValueClass::GetType())
				throw ExecutionException("Invalid rvalue cast attempted!");

			return dynamic_cast<RValueClass&>(rval);
		}

	}

}


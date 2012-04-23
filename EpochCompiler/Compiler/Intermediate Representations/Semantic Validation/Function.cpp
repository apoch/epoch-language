//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class containing a function
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Function.h"
#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Expression.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"

#include "Compiler/Exceptions.h"
#include "Compiler/CompileErrors.h"
#include "Compiler/Session.h"

#include "Libraries/Library.h"


using namespace IRSemantics;


// External prototypes (yeah, I'm lazy)
void CompileConstructorStructure(IRSemantics::Statement& statement, IRSemantics::Program& program, IRSemantics::CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);


//
// Destruct and clean up a function
//
Function::~Function()
{
	for(std::vector<Param>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
		delete iter->Parameter;

	delete Code;
	delete Return;
}

//
// Add a parameter to a function's signature
//
void Function::AddParameter(StringHandle name, FunctionParam* param, CompileErrors& errors)
{
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(iter->Name == name)
		{
			errors.SemanticError("Duplicate function parameter name");
			return;
		}
	}

	Parameters.push_back(Param(name, param));
}

//
// Set a function's return expression
//
void Function::SetReturnExpression(IRSemantics::Expression* expression)
{
	delete Return;
	Return = expression;
}

//
// Retrieve the return type of a function
//
VM::EpochTypeID Function::GetReturnType(const Program& program) const
{
	if(Return)
		return Return->GetEpochType(program);

	return VM::EpochType_Void;
}

//
// Set a function's code body
//
void Function::SetCode(CodeBlock* code)
{
	delete Code;
	Code = code;
}

//
// Determine if a given parameter is a local variable
// or not; primarily useful for differentiating between
// actual formal parameters and pattern-matched values.
//
bool Function::IsParameterLocalVariable(StringHandle name) const
{
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(iter->Name == name)
			return iter->Parameter->IsLocalVariable();
	}

	throw InternalException("Provided string handle does not correspond to a parameter of this function");
}

//
// Determine if the given parameter is passed to the
// function by reference.
//
bool Function::IsParameterReference(StringHandle name) const
{
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(iter->Name == name)
			return iter->Parameter->IsReference();
	}

	throw InternalException("Provided string handle does not correspond to a parameter of this function");
}

//
// Retrieve the type of the given parameter
//
VM::EpochTypeID Function::GetParameterType(StringHandle name, const IRSemantics::Program& program) const
{
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(iter->Name == name)
			return iter->Parameter->GetParamType(program);
	}

	throw InternalException("Provided string handle does not correspond to a parameter of this function");
}

//
// Retrieve the type of a parameter, using its index instead of its name
//
VM::EpochTypeID Function::GetParameterTypeByIndex(size_t index, const IRSemantics::Program& program) const
{
	return Parameters[index].Parameter->GetParamType(program);
}

//
// Retrieve a container of all the names of parameters to this function
//
std::vector<StringHandle> Function::GetParameterNames() const
{
	std::vector<StringHandle> ret;
	ret.reserve(Parameters.size());

	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
		ret.push_back(iter->Name);

	return ret;
}

//
// Determine if the given string handle corresponds to a parameter to this function
//
bool Function::HasParameter(StringHandle paramname) const
{
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(iter->Name == paramname)
			return true;
	}

	return false;
}


//
// Validate a function definition
//
bool Function::Validate(const IRSemantics::Program& program) const
{
	bool valid = true;

	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!iter->Parameter->Validate(program))
			valid = false;
	}

	if(Return)
	{
		if(!Return->Validate(program))
			valid = false;
	}

	if(Code)
	{
		if(!Code->Validate(program))
			valid = false;
	}

	return valid;
}


//
// Perform compile-time code execution on a function
//
// Note that the function body is not requested to perform compile-time code
// execution at this point; we wait to do so until the type inference pass
// is performed in order to prevent variables from being used prior to
// their point of definition in the program. Since variable definitions are
// compile-time code that modifies the local lexical scope, if we perform
// this process too soon, variables can be referenced anywhere in the scope
// in which they are defined, rather than only after the point of definition.
//
bool Function::CompileTimeCodeExecution(Program& program, CompileErrors& errors)
{
	for(std::vector<FunctionTag>::const_iterator iter = Tags.begin(); iter != Tags.end(); ++iter)
	{
		FunctionTagHelperTable::const_iterator helperiter = program.Session.FunctionTagHelpers.find(program.GetString(iter->TagName));
		if(helperiter != program.Session.FunctionTagHelpers.end())
		{
			TagHelperReturn help = helperiter->second(RawName, iter->Parameters, true);
			if(help.SetConstructorFunction)
			{
				FunctionSignature signature = GetFunctionSignature(program);
				signature.PrependParameter(L"@id", VM::EpochType_Identifier, false);
				signature.SetReturnType(VM::EpochType_Void);
				program.Session.FunctionSignatures[Name] = signature;
				Code->GetScope()->PrependVariable(L"@id", program.AddString(L"@id"), VM::EpochType_Identifier, false, VARIABLE_ORIGIN_PARAMETER);

				program.Session.InfoTable.FunctionHelpers->insert(std::make_pair(Name, &CompileConstructorStructure));
			}
		}
		else
		{
			errors.SetContext(iter->OriginalTag);
			errors.SemanticError("Unrecognized function tag");
		}
	}

	if(Return)
	{
		if(!Code)
			return false;

		if(!Return->CompileTimeCodeExecution(program, *Code, true, errors))
			return false;
	}

	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!iter->Parameter->IsLocalVariable())
			program.MarkFunctionWithStaticPatternMatching(RawName, Name);
	}

	return true;
}

//
// Perform type inference on a function
//
bool Function::TypeInference(Program& program, InferenceContext&, CompileErrors& errors)
{
	if(InferenceDone)
		return true;

	if(!Code)
	{
		InferenceDone = true;
		return true;
	}

	if(Return)
	{
		InferenceContext newcontext(Name, InferenceContext::CONTEXT_FUNCTION_RETURN);
		newcontext.FunctionName = Name;
		if(!Return->TypeInference(program, *Code, newcontext, 0, 1, errors))
		{
			InferenceDone = true;
			return false;
		}
	}

	bool result = true;
	for(std::vector<Param>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!iter->Parameter->TypeInference(program, errors))
			result = false;
	}

	if(!result)
	{
		InferenceDone = true;
		return false;
	}

	program.AddScope(Code->GetScope());		// TODO - better solution than aliasing the scope
	program.AddScope(Code->GetScope(), Name);

	InferenceContext newcontext(Name, InferenceContext::CONTEXT_FUNCTION);
	newcontext.FunctionName = Name;
	result = Code->TypeInference(program, newcontext, errors);
	InferenceDone = true;
	return result;
}

//
// Get the return type of a function signature passed
// as a higher-order function to this function.
//
// That's confusing, so here's what this does:
//   - The function is passed a higher-order parameter foo
//   - foo has some statically known signature
//   - This function obtains foo's return type from that signature
//
VM::EpochTypeID Function::GetParameterSignatureType(StringHandle name, const IRSemantics::Program& program) const
{
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(iter->Name == name)
		{
			StringHandle rettypename = dynamic_cast<const FunctionParamFuncRef*>(iter->Parameter)->GetReturnType();
			if(!rettypename)
				return VM::EpochType_Void;

			return program.LookupType(rettypename);
		}
	}

	throw InternalException("Provided string handle does not correspond to a parameter of this function");
}

//
// Determine if the given function signature is compatible
// with the higher-order function signature of this function
// passed in the given parameter slot.
//
// That's also confusing, so here's the breakdown:
//   - Function takes a higher-order parameter foo
//   - foo has a static signature
//   - This function checks if another signature matches foo's signature
//
bool Function::DoesParameterSignatureMatch(size_t index, const FunctionSignature& signature, const IRSemantics::Program& program) const
{
	const FunctionParamFuncRef* param = dynamic_cast<const FunctionParamFuncRef*>(Parameters[index].Parameter);
	StringHandle nameoftype = param->GetReturnType();
	VM::EpochTypeID type = nameoftype ? program.LookupType(nameoftype) : VM::EpochType_Void;
	if(type != signature.GetReturnType())
		return false;

	const std::vector<StringHandle>& paramtypes = param->GetParamTypes();
	if(paramtypes.size() != signature.GetNumParameters())
		return false;

	for(size_t i = 0; i < paramtypes.size(); ++i)
	{
		const CompileTimeParameter& parameter = signature.GetParameter(i);
		if(parameter.Type != program.LookupType(paramtypes[i]))
			return false;

		if(parameter.Type == VM::EpochType_Function)
		{
			throw std::runtime_error("Not implemented");		// TODO - better exceptions
		}
	}

	return true;
}

//
// Retrieve the higher-order function signature of the
// function parameter with the given name.
//
// All this stuff is confusing!
//   - Function takes higher-order parameter foo
//   - foo has a static signature
//   - This returns foo's signature
//
FunctionSignature Function::GetParameterSignature(StringHandle name, const IRSemantics::Program& program) const
{
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(iter->Name == name)
		{
			FunctionSignature ret;
			const FunctionParamFuncRef* param = dynamic_cast<const FunctionParamFuncRef*>(iter->Parameter);
			if(!param)
				return ret;

			StringHandle nameoftype = param->GetReturnType();
			VM::EpochTypeID type = nameoftype ? program.LookupType(nameoftype) : VM::EpochType_Void;
			ret.SetReturnType(type);

			const std::vector<StringHandle>& paramtypes = param->GetParamTypes();
			for(size_t i = 0; i < paramtypes.size(); ++i)
			{
				VM::EpochTypeID paramtype = program.LookupType(paramtypes[i]);
				ret.AddParameter(L"@@internal", paramtype, false);
			}

			return ret;
		}
	}

	throw InternalException("Provided string handle does not correspond to a parameter of this function");
}

//
// Get the complete signature of this function
//
FunctionSignature Function::GetFunctionSignature(const Program& program) const
{
	FunctionSignature ret;

	ret.SetReturnType(GetReturnType(program));

	size_t index = 0;
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		VM::EpochTypeID paramtype = iter->Parameter->GetParamType(program);
		if(iter->Parameter->IsLocalVariable())
		{
			ret.AddParameter(program.GetString(iter->Name), paramtype, iter->Parameter->IsReference());
			if(paramtype == VM::EpochType_Function)
				ret.SetFunctionSignature(index, GetParameterSignature(iter->Name, program));
		}
		else
		{
			iter->Parameter->AddToSignature(ret, program);
		}

		++index;
	}

	return ret;
}

//
// Perform static pattern matching on the parameter at
// the given index. Checks if statically-known values
// of function arguments match the formal patterns
// specified in the function definition.
//
bool Function::PatternMatchParameter(size_t index, const CompileTimeParameter& param, const IRSemantics::Program& program) const
{
	return Parameters[index].Parameter->PatternMatchValue(param, program);
}

//
// Add a tag to this function
//
void Function::AddTag(const FunctionTag& tag)
{
	Tags.push_back(tag);
}


//
// Validate a named function parameter
//
bool FunctionParamNamed::Validate(const IRSemantics::Program& program) const
{
	return (program.LookupType(MyType) != VM::EpochType_Error);
}

//
// Get the type of a named function parameter
//
VM::EpochTypeID FunctionParamNamed::GetParamType(const IRSemantics::Program& program) const
{
	return program.LookupType(MyType);
}

//
// Validate a higher order function parameter
//
bool FunctionParamFuncRef::Validate(const IRSemantics::Program& program) const
{
	bool valid = true;

	if(ReturnType && program.LookupType(ReturnType) == VM::EpochType_Error)
		valid = false;

	for(std::vector<StringHandle>::const_iterator iter = ParamTypes.begin(); iter != ParamTypes.end(); ++iter)
	{
		if(program.LookupType(*iter) == VM::EpochType_Error)
			valid = false;
	}

	return valid;
}

//
// Reject attempts to pattern-match on functions
//
// This is a feature that might be implemented in the
// future if demand is sufficient, but is currently
// deemed too complex to bother with just yet.
//
void FunctionParamFuncRef::AddToSignature(FunctionSignature&, const IRSemantics::Program&) const
{
	throw InternalException("Cannot pattern match on function signatures");
}

//
// Validate an expression function parameter
//
bool FunctionParamExpression::Validate(const IRSemantics::Program& program) const
{
	if(!MyExpression)
		return false;

	return MyExpression->Validate(program);
}

//
// Destruct and clean up an expression function parameter
//
FunctionParamExpression::~FunctionParamExpression()
{
	delete MyExpression;
}

//
// Get the type of an expression passed to a function
//
// Expressions are generally used as function formal parameters
// when performing pattern matching.
//
VM::EpochTypeID FunctionParamExpression::GetParamType(const IRSemantics::Program& program) const
{
	if(!MyExpression)
		return VM::EpochType_Error;

	return MyExpression->GetEpochType(program);
}

//
// Add an expression's compile-time form to a function signature
// for enabling static pattern matching.
//
void FunctionParamExpression::AddToSignature(FunctionSignature& signature, const IRSemantics::Program& program) const
{
	if(!MyExpression || MyExpression->GetAtoms().size() != 1)
		throw InternalException("Cannot generate pattern matched parameter");

	CompileTimeParameter value = MyExpression->GetAtoms()[0]->ConvertToCompileTimeParam(program);
	switch(value.Type)
	{
	case VM::EpochType_Integer:
		signature.AddPatternMatchedParameter(value.Payload.IntegerValue);
		break;

	default:
		throw NotImplementedException("Support for pattern matching on this type is not implemented");
	}
}

//
// Perform type inference on an expression passed to a function
// as a formal parameter. Useful for identifying the types of
// such parameters when validating pattern matching.
//
bool FunctionParamExpression::TypeInference(IRSemantics::Program& program, CompileErrors& errors)
{
	InferenceContext context(0, InferenceContext::CONTEXT_EXPRESSION);
	ScopeDescription scope;
	IRSemantics::CodeBlock fakeblock(&scope, false);
	return MyExpression->TypeInference(program, fakeblock, context, 0, 1, errors);
}

//
// Check if the given compile-time parameter matches
// the pattern expected by this function parameter.
//
bool FunctionParamExpression::PatternMatchValue(const CompileTimeParameter& param, const IRSemantics::Program& program) const
{
	if(!MyExpression || MyExpression->GetAtoms().size() != 1)
		throw InternalException("Cannot generate pattern matched parameter");

	CompileTimeParameter value = MyExpression->GetAtoms()[0]->ConvertToCompileTimeParam(program);
	switch(value.Type)
	{
	case VM::EpochType_Integer:
		return param.Payload.IntegerValue == value.Payload.IntegerValue;
		
	default:
		throw NotImplementedException("Support for pattern matching on this type is not implemented");
	}
}

//
// Nameless typed parameters cannot participate in pattern matching
//
void FunctionParamTyped::AddToSignature(FunctionSignature&, const IRSemantics::Program&) const
{
	throw InternalException("Cannot pattern match on this kind of function parameter");
}

//
// Named parameters cannot participate in pattern matching
//
void FunctionParamNamed::AddToSignature(FunctionSignature&, const IRSemantics::Program&) const
{
	throw InternalException("Cannot pattern match on this kind of function parameter");
}

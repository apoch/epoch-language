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
#include "Compiler/Intermediate Representations/Semantic Validation/Namespace.h"

#include "Compiler/Exceptions.h"
#include "Compiler/CompileErrors.h"
#include "Compiler/Session.h"

#include "Libraries/Library.h"


using namespace IRSemantics;


// External prototypes (yeah, I'm lazy)
void CompileConstructorStructure(IRSemantics::Statement& statement, Namespace& curnamespace, IRSemantics::CodeBlock& activescope, bool inreturnexpr, CompileErrors& errors);



Function::Function(const Function* templatefunc, Namespace& curnamespace, const CompileTimeParameterVector& args)
	: Code(templatefunc->Code ? templatefunc->Code->Clone() : NULL),
	  Return(templatefunc->Return ? templatefunc->Return->Clone() : NULL),
	  InferenceDone(false),
	  SuppressReturn(false),
	  Name(0),
	  RawName(0),
	  AnonymousReturn(false),
	  HintReturnType(VM::EpochType_Error),
	  DummyNamespace(NULL),
	  Tags(templatefunc->Tags),
	  TemplateParams(templatefunc->TemplateParams)
{
	for(std::vector<Param>::const_iterator iter = templatefunc->Parameters.begin(); iter != templatefunc->Parameters.end(); ++iter)
		Parameters.push_back(iter->Clone());

	SetTemplateArguments(curnamespace, args);

	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		FunctionParamNamed* named = dynamic_cast<FunctionParamNamed*>(iter->Parameter);
		if(named)
			named->SubstituteTemplateArgs(TemplateParams, args, curnamespace);
	}
}


//
// Destruct and clean up a function
//
Function::~Function()
{
	for(std::vector<Param>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
		delete iter->Parameter;

	delete Code;
	delete Return;

	delete DummyNamespace;
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
VM::EpochTypeID Function::GetReturnType(const Namespace& curnamespace) const
{
	if(Return)
	{
		if(Return->IsInferenceDone())
			return Return->GetEpochType(curnamespace);

		return HintReturnType;
	}

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
VM::EpochTypeID Function::GetParameterType(StringHandle name, Namespace& curnamespace, CompileErrors& errors) const
{
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(iter->Name == name)
		{
			iter->Parameter->TypeInference(curnamespace, errors);
			return iter->Parameter->GetParamType(curnamespace);
		}
	}

	throw InternalException("Provided string handle does not correspond to a parameter of this function");
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
bool Function::Validate(const Namespace& curnamespace) const
{
	if(IsTemplate())
		return true;

	bool valid = true;

	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!iter->Parameter->Validate(curnamespace))
			valid = false;
	}

	if(Return)
	{
		if(!Return->Validate(curnamespace))
			valid = false;
	}

	if(Code)
	{
		if(!Code->Validate(curnamespace))
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
bool Function::CompileTimeCodeExecution(Namespace& curnamespace, CompileErrors& errors)
{
	if(IsTemplate())
		return true;

	Namespace* activenamespace;

	if(TemplateArgs.empty())
		activenamespace = &curnamespace;
	else
		activenamespace = DummyNamespace;

	for(std::vector<FunctionTag>::const_iterator iter = Tags.begin(); iter != Tags.end(); ++iter)
	{
		if(curnamespace.FunctionTags.Exists(iter->TagName))
		{
			TagHelperReturn help = curnamespace.FunctionTags.GetHelper(iter->TagName)(RawName, iter->Parameters, true);
			if(help.SetConstructorFunction)
			{
				TypeInferenceParamsOnly(*activenamespace, errors);
				FunctionSignature signature = GetFunctionSignature(*activenamespace);
				signature.PrependParameter(L"@id", VM::EpochType_Identifier, false);
				signature.SetReturnType(VM::EpochType_Void);
				curnamespace.Functions.SetSignature(Name, signature);
				Code->GetScope()->PrependVariable(L"@id", curnamespace.Strings.Pool(L"@id"), curnamespace.Strings.Pool(L"identifier"),VM::EpochType_Identifier, false, VARIABLE_ORIGIN_PARAMETER);

				curnamespace.Functions.SetCompileHelper(Name, &CompileConstructorStructure);
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

		if(!Return->CompileTimeCodeExecution(*activenamespace, *Code, true, errors))
			return false;
	}

	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!iter->Parameter->IsLocalVariable())
			curnamespace.Functions.MarkFunctionWithStaticPatternMatching(RawName, Name);
	}

	return true;
}

//
// Perform type inference on a function
//
bool Function::TypeInference(Namespace& curnamespace, InferenceContext&, CompileErrors& errors)
{
	if(IsTemplate())
		return true;

	if(InferenceDone)
		return true;

	InferenceDone = true;

	if(!Code)
		return true;

	Namespace* activenamespace;

	if(TemplateArgs.empty())
		activenamespace = &curnamespace;
	else
		activenamespace = DummyNamespace;

	if(Return)
	{
		InferenceContext newcontext(Name, InferenceContext::CONTEXT_FUNCTION_RETURN);
		newcontext.FunctionName = Name;
		if(!Return->TypeInference(*activenamespace, *Code, newcontext, 0, 1, errors))
			return false;

		VM::EpochTypeID rettype = Return->GetEpochType(*activenamespace);
		if(rettype != VM::EpochType_Void)
		{
			if(!Code->GetScope()->HasReturnVariable())
			{
				Code->AddVariable(L"@@anonymousret", curnamespace.Strings.Pool(L"@@anonymousret"), activenamespace->Types.GetNameOfType(rettype), rettype, false, VARIABLE_ORIGIN_RETURN);
				AnonymousReturn = true;
			}
		}
	}

	bool result = true;
	for(std::vector<Param>::iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(!iter->Parameter->TypeInference(*activenamespace, errors))
			result = false;
	}

	if(!result)
		return false;

	curnamespace.AddScope(Code->GetScope(), Name);

	InferenceContext newcontext(Name, InferenceContext::CONTEXT_FUNCTION);
	newcontext.FunctionName = Name;
	result = Code->TypeInference(*activenamespace, newcontext, errors);
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
VM::EpochTypeID Function::GetParameterSignatureType(StringHandle name, const Namespace& curnamespace) const
{
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(iter->Name == name)
		{
			StringHandle rettypename = dynamic_cast<const FunctionParamFuncRef*>(iter->Parameter)->GetReturnType();
			if(!rettypename)
				return VM::EpochType_Void;

			return curnamespace.Types.GetTypeByName(rettypename);
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
bool Function::DoesParameterSignatureMatch(size_t index, const FunctionSignature& signature, const Namespace& curnamespace) const
{
	const FunctionParamFuncRef* param = dynamic_cast<const FunctionParamFuncRef*>(Parameters[index].Parameter);
	StringHandle nameoftype = param->GetReturnType();
	VM::EpochTypeID type = nameoftype ? curnamespace.Types.GetTypeByName(nameoftype) : VM::EpochType_Void;
	if(type != signature.GetReturnType())
		return false;

	const std::vector<StringHandle>& paramtypes = param->GetParamTypes();
	if(paramtypes.size() != signature.GetNumParameters())
		return false;

	for(size_t i = 0; i < paramtypes.size(); ++i)
	{
		const CompileTimeParameter& parameter = signature.GetParameter(i);
		if(parameter.Type != curnamespace.Types.GetTypeByName(paramtypes[i]))
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
FunctionSignature Function::GetParameterSignature(StringHandle name, const Namespace& curnamespace) const
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
			VM::EpochTypeID type = nameoftype ? curnamespace.Types.GetTypeByName(nameoftype) : VM::EpochType_Void;
			ret.SetReturnType(type);

			const std::vector<StringHandle>& paramtypes = param->GetParamTypes();
			for(size_t i = 0; i < paramtypes.size(); ++i)
			{
				VM::EpochTypeID paramtype = curnamespace.Types.GetTypeByName(paramtypes[i]);
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
FunctionSignature Function::GetFunctionSignature(const Namespace& curnamespace) const
{
	const Namespace* activenamespace;

	if(TemplateArgs.empty())
		activenamespace = &curnamespace;
	else
		activenamespace = DummyNamespace;

	FunctionSignature ret;

	ret.SetReturnType(GetReturnType(*activenamespace));

	size_t index = 0;
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		VM::EpochTypeID paramtype = iter->Parameter->GetParamType(*activenamespace);
		if(iter->Parameter->IsLocalVariable())
		{
			ret.AddParameter(activenamespace->Strings.GetPooledString(iter->Name), paramtype, iter->Parameter->IsReference());
			if(paramtype == VM::EpochType_Function)
				ret.SetFunctionSignature(index, GetParameterSignature(iter->Name, *activenamespace));
		}
		else
		{
			iter->Parameter->AddToSignature(ret, *activenamespace);
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
bool Function::PatternMatchParameter(size_t index, const CompileTimeParameter& param, const Namespace& curnamespace) const
{
	return Parameters[index].Parameter->PatternMatchValue(param, curnamespace);
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
bool FunctionParamNamed::Validate(const Namespace& curnamespace) const
{
	return (GetParamType(curnamespace) != VM::EpochType_Error);
}

//
// Get the type of a named function parameter
//
VM::EpochTypeID FunctionParamNamed::GetParamType(const Namespace&) const
{
	return MyActualType;
}

bool FunctionParamNamed::TypeInference(Namespace& curnamespace, CompileErrors&)
{
	StringHandle name;
	if(TemplateArgs.empty())
		name = MyTypeName;
	else
		name = curnamespace.Types.Templates.InstantiateStructure(MyTypeName, TemplateArgs);

	MyActualType = curnamespace.Types.GetTypeByName(name);
	return true;
}

void FunctionParamNamed::SubstituteTemplateArgs(const std::vector<std::pair<StringHandle, VM::EpochTypeID> >& params, const CompileTimeParameterVector& args, Namespace& curnamespace)
{
	if(TemplateArgs.empty())
		return;

	// Simplify arguments as much as possible first
	for(CompileTimeParameterVector::iterator iter = TemplateArgs.begin(); iter != TemplateArgs.end(); ++iter)
	{
		for(size_t i = 0; i < params.size(); ++i)
		{
			if(params[i].first == iter->Payload.LiteralStringHandleValue)
				iter->Payload.LiteralStringHandleValue = args[i].Payload.LiteralStringHandleValue;
		}
	}

	// Now modify my name and type as appropriate
	MyTypeName = curnamespace.Types.Templates.InstantiateStructure(MyTypeName, TemplateArgs);
}



//
// Validate a higher order function parameter
//
bool FunctionParamFuncRef::Validate(const Namespace& curnamespace) const
{
	bool valid = true;

	if(ReturnType && curnamespace.Types.GetTypeByName(ReturnType) == VM::EpochType_Error)
		valid = false;

	for(std::vector<StringHandle>::const_iterator iter = ParamTypes.begin(); iter != ParamTypes.end(); ++iter)
	{
		if(curnamespace.Types.GetTypeByName(*iter) == VM::EpochType_Error)
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
void FunctionParamFuncRef::AddToSignature(FunctionSignature&, const Namespace&) const
{
	throw InternalException("Cannot pattern match on function signatures");
}

FunctionParam* FunctionParamFuncRef::Clone() const
{
	FunctionParamFuncRef* clone = new FunctionParamFuncRef;
	clone->ParamTypes = ParamTypes;
	clone->ReturnType = ReturnType;
	return clone;
}

//
// Validate an expression function parameter
//
bool FunctionParamExpression::Validate(const Namespace& curnamespace) const
{
	if(!MyExpression)
		return false;

	return MyExpression->Validate(curnamespace);
}

//
// Destruct and clean up an expression function parameter
//
FunctionParamExpression::~FunctionParamExpression()
{
	delete MyExpression;
}

FunctionParam* FunctionParamExpression::Clone() const
{
	return new FunctionParamExpression(MyExpression->Clone());
}

//
// Get the type of an expression passed to a function
//
// Expressions are generally used as function formal parameters
// when performing pattern matching.
//
VM::EpochTypeID FunctionParamExpression::GetParamType(const Namespace& curnamespace) const
{
	if(!MyExpression)
		return VM::EpochType_Error;

	return MyExpression->GetEpochType(curnamespace);
}

//
// Add an expression's compile-time form to a function signature
// for enabling static pattern matching.
//
void FunctionParamExpression::AddToSignature(FunctionSignature& signature, const Namespace& curnamespace) const
{
	if(!MyExpression || MyExpression->GetAtoms().size() != 1)
		throw InternalException("Cannot generate pattern matched parameter");

	CompileTimeParameter value = MyExpression->GetAtoms()[0]->ConvertToCompileTimeParam(curnamespace);
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
bool FunctionParamExpression::TypeInference(Namespace& curnamespace, CompileErrors& errors)
{
	InferenceContext context(0, InferenceContext::CONTEXT_EXPRESSION);
	ScopeDescription scope;
	IRSemantics::CodeBlock fakeblock(&scope, false);
	return MyExpression->TypeInference(curnamespace, fakeblock, context, 0, 1, errors);
}

//
// Check if the given compile-time parameter matches
// the pattern expected by this function parameter.
//
bool FunctionParamExpression::PatternMatchValue(const CompileTimeParameter& param, const Namespace& curnamespace) const
{
	if(!MyExpression || MyExpression->GetAtoms().size() != 1)
		throw InternalException("Cannot generate pattern matched parameter");

	CompileTimeParameter value = MyExpression->GetAtoms()[0]->ConvertToCompileTimeParam(curnamespace);
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
void FunctionParamTyped::AddToSignature(FunctionSignature&, const Namespace&) const
{
	throw InternalException("Cannot pattern match on this kind of function parameter");
}

FunctionParam* FunctionParamTyped::Clone() const
{
	return new FunctionParamTyped(MyType, IsRef);
}

//
// Named parameters cannot participate in pattern matching
//
void FunctionParamNamed::AddToSignature(FunctionSignature&, const Namespace&) const
{
	throw InternalException("Cannot pattern match on this kind of function parameter");
}

FunctionParam* FunctionParamNamed::Clone() const
{
	FunctionParamNamed* clone = new FunctionParamNamed(MyTypeName, TemplateArgs, IsRef);
	clone->MyActualType = MyActualType;
	return clone;
}


void Function::AddTemplateParameter(VM::EpochTypeID type, StringHandle name)
{
	TemplateParams.push_back(std::make_pair(name, type));
}

void Function::SetTemplateArguments(Namespace& curnamespace, const CompileTimeParameterVector& args)
{
	TemplateArgs = args;
	delete DummyNamespace;
	DummyNamespace = Namespace::CreateTemplateDummy(curnamespace, TemplateParams, TemplateArgs);
}

void Function::FixupScope()
{
	CompileTimeParameterVector argtypes;
	for(size_t i = 0; i < TemplateArgs.size(); ++i)
	{
		if(TemplateParams[i].second == VM::EpochType_Wildcard)
		{
			CompileTimeParameter param(L"@templatearg", VM::EpochType_Wildcard);
			param.Payload.IntegerValue = static_cast<int>(DummyNamespace->Types.GetTypeByName(TemplateArgs[i].Payload.LiteralStringHandleValue));
			argtypes.push_back(param);
		}
		else
			argtypes.push_back(CompileTimeParameter(L"@templatearg", VM::EpochType_Error));
	}

	Code->GetScope()->Fixup(TemplateParams, TemplateArgs, argtypes);
}

StringHandle Function::GetParameterTypeName(StringHandle name) const
{
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(iter->Name == name)
		{
			const FunctionParamNamed* namedparam = dynamic_cast<const FunctionParamNamed*>(iter->Parameter);

			if(namedparam)
				return namedparam->GetTypeName();

			return 0;
		}
	}

	throw InternalException("Parameter not found");
}

void Function::PopulateScope(Namespace& curnamespace, CompileErrors& errors)
{
	Namespace* activenamespace;

	if(TemplateArgs.empty())
		activenamespace = &curnamespace;
	else
		activenamespace = DummyNamespace;

	TypeInferenceParamsOnly(curnamespace, errors);

	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
		iter->Parameter->AddToScope(iter->Name, *GetCode(), *activenamespace);
}



void FunctionParamNamed::AddToScope(StringHandle name, CodeBlock& code, const Namespace& curnamespace) const
{
	code.AddVariable(curnamespace.Strings.GetPooledString(name), name, MyTypeName, MyActualType, IsRef, VARIABLE_ORIGIN_PARAMETER);
}

void FunctionParamFuncRef::AddToScope(StringHandle name, CodeBlock& code, const Namespace& curnamespace) const
{
	code.AddVariable(curnamespace.Strings.GetPooledString(name), name, 0, VM::EpochType_Function, false, VARIABLE_ORIGIN_PARAMETER);
}


void Function::TypeInferenceParamsOnly(Namespace& curnamespace, CompileErrors& errors)
{
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
		iter->Parameter->TypeInference(curnamespace, errors);
}


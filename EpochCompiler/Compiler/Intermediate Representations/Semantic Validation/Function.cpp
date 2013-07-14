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
#include "Compiler/Intermediate Representations/Semantic Validation/Helpers.h"

#include "Compiler/Exceptions.h"
#include "Compiler/CompileErrors.h"
#include "Compiler/Session.h"

#include "Libraries/Library.h"


using namespace IRSemantics;


//
// Constructor used to deep copy a function
//
Function::Function(const Function* templatefunc, Namespace& curnamespace, const CompileTimeParameterVector& args, CompileErrors& errors)
	: Code(templatefunc->Code ? templatefunc->Code->Clone() : NULL),
	  Return(templatefunc->Return ? templatefunc->Return->Clone() : NULL),
	  InferenceDone(false),
	  SuppressReturn(false),
	  Name(0),
	  RawName(0),
	  AnonymousReturn(false),
	  HintReturnType(Metadata::EpochType_Error),
	  DummyNamespace(NULL),
	  Tags(templatefunc->Tags),
	  TemplateParams(templatefunc->TemplateParams),
	  SuppressGeneration(templatefunc->SuppressGeneration)
{
	for(std::vector<Param>::const_iterator iter = templatefunc->Parameters.begin(); iter != templatefunc->Parameters.end(); ++iter)
		Parameters.push_back(iter->Clone());

	SetTemplateArguments(curnamespace, args);

	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		FunctionParamNamed* named = dynamic_cast<FunctionParamNamed*>(iter->Parameter);
		if(named)
			named->SubstituteTemplateArgs(TemplateParams, args, curnamespace, errors);
		else
		{
			FunctionParamFuncRef* fref = dynamic_cast<FunctionParamFuncRef*>(iter->Parameter);
			if(fref)
				fref->SubstituteTemplateArgs(TemplateParams, args, curnamespace, errors);
		}
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
void Function::AddParameter(StringHandle name, FunctionParam* param, bool isnothing, CompileErrors& errors)
{
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if((iter->Name == name) && (!isnothing))
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
Metadata::EpochTypeID Function::GetReturnType(const Namespace& curnamespace) const
{
	if(Return)
	{
		if(Return->IsInferenceDone())
			return Return->GetEpochType(curnamespace);

		return HintReturnType;
	}

	return Metadata::EpochType_Void;
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
// Retrieve the type of the given parameter
//
Metadata::EpochTypeID Function::GetParameterType(StringHandle name, Namespace& curnamespace, CompileErrors& errors) const
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
				signature.PrependParameter(L"@id", Metadata::MakeReferenceType(Metadata::EpochType_Identifier));
				signature.SetReturnType(Metadata::EpochType_Void);
				curnamespace.Functions.SetSignature(Name, signature);
				Code->GetScope()->PrependVariable(L"@id", curnamespace.Strings.Pool(L"@id"), curnamespace.Strings.Pool(L"identifier"), Metadata::MakeReferenceType(Metadata::EpochType_Identifier), VARIABLE_ORIGIN_PARAMETER);

				curnamespace.Functions.SetCompileHelper(Name, &CompileConstructorHelper);
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

		Metadata::EpochTypeID rettype = Return->GetEpochType(*activenamespace);
		if(rettype != Metadata::EpochType_Void)
		{
			if(!Code->GetScope()->HasReturnVariable())
			{
				Code->AddVariable(L"@@anonymousret", curnamespace.Strings.Pool(L"@@anonymousret"), activenamespace->Types.GetNameOfType(rettype), rettype, VARIABLE_ORIGIN_RETURN);
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
Metadata::EpochTypeID Function::GetParameterSignatureType(StringHandle name, const Namespace& curnamespace) const
{
	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
	{
		if(iter->Name == name)
		{
			StringHandle rettypename = dynamic_cast<const FunctionParamFuncRef*>(iter->Parameter)->GetReturnType();
			if(!rettypename)
				return Metadata::EpochType_Void;

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
	Metadata::EpochTypeID type = nameoftype ? curnamespace.Types.GetTypeByName(nameoftype) : Metadata::EpochType_Void;
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

		if(Metadata::GetTypeFamily(parameter.Type) == Metadata::EpochTypeFamily_Function)
		{
			//
			// This is just a missing feature.
			//
			// Signature checks should be extended to be recursive so that
			// any number of higher-order parameters can be passed around.
			//
			throw NotImplementedException("Support for this degree of higher-order functions is not implemented");
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
			Metadata::EpochTypeID type = nameoftype ? curnamespace.Types.GetTypeByName(nameoftype) : Metadata::EpochType_Void;
			ret.SetReturnType(type);

			const std::vector<StringHandle>& paramtypes = param->GetParamTypes();
			for(size_t i = 0; i < paramtypes.size(); ++i)
			{
				Metadata::EpochTypeID paramtype = curnamespace.Types.GetTypeByName(paramtypes[i]);
				ret.AddParameter(L"@@internal", paramtype);
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
		Metadata::EpochTypeID paramtype = iter->Parameter->GetParamType(*activenamespace);
		if(iter->Parameter->IsLocalVariable())
		{
			ret.AddParameter(activenamespace->Strings.GetPooledString(iter->Name), paramtype);
			if(Metadata::GetTypeFamily(paramtype) == Metadata::EpochTypeFamily_Function)
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


void Function::HoistScopes()
{
	if(Code)
		Code->HoistScopes(NULL);
}


//
// Validate a named function parameter
//
bool FunctionParamNamed::Validate(const Namespace& curnamespace) const
{
	return (GetParamType(curnamespace) != Metadata::EpochType_Error);
}

//
// Get the type of a named function parameter
//
Metadata::EpochTypeID FunctionParamNamed::GetParamType(const Namespace&) const
{
	return MyActualType;
}

//
// Perform type inference on a named function parameter
//
// Makes sure to instantiate structure templates if necessary
//
bool FunctionParamNamed::TypeInference(Namespace& curnamespace, CompileErrors& errors)
{
	StringHandle name;
	if(TemplateArgs.empty())
		name = MyTypeName;
	else
		name = curnamespace.Types.Templates.InstantiateStructure(MyTypeName, TemplateArgs, errors);

	MyActualType = curnamespace.Types.GetTypeByName(name);
	if(IsRef)
		MyActualType = Metadata::MakeReferenceType(MyActualType);
	return true;
}

//
// Allow parameterized types to receive the correct
// arguments, when used on a named function parameter.
//
void FunctionParamNamed::SubstituteTemplateArgs(const std::vector<std::pair<StringHandle, Metadata::EpochTypeID> >& params, const CompileTimeParameterVector& args, Namespace& curnamespace, CompileErrors& errors)
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
	MyTypeName = curnamespace.Types.Templates.InstantiateStructure(MyTypeName, TemplateArgs, errors);
}



//
// Validate a higher order function parameter
//
bool FunctionParamFuncRef::Validate(const Namespace& curnamespace) const
{
	bool valid = true;

	if(ReturnType && curnamespace.Types.GetTypeByName(ReturnType) == Metadata::EpochType_Error)
		valid = false;

	for(std::vector<StringHandle>::const_iterator iter = ParamTypes.begin(); iter != ParamTypes.end(); ++iter)
	{
		if(curnamespace.Types.GetTypeByName(*iter) == Metadata::EpochType_Error)
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

//
// Deep copy a higher-order function parameter
//
FunctionParam* FunctionParamFuncRef::Clone() const
{
	FunctionParamFuncRef* clone = new FunctionParamFuncRef;
	clone->ParamTypes = ParamTypes;
	clone->ReturnType = ReturnType;
	return clone;
}

Metadata::EpochTypeID FunctionParamFuncRef::GetParamType(const Namespace& curnamespace) const
{
	FunctionSignature sig;
	if(ReturnType)
		sig.SetReturnType(curnamespace.Types.GetTypeByName(ReturnType));
	else
		sig.SetReturnType(Metadata::EpochType_Void);
	
	for(std::vector<StringHandle>::const_iterator iter = ParamTypes.begin(); iter != ParamTypes.end(); ++iter)
		sig.AddParameter(L"@@higherorder", curnamespace.Types.GetTypeByName(*iter));

	return curnamespace.Types.FunctionSignatures.FindEpochType(sig);
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

//
// Deep copy a pattern-matcher expression parameter
//
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
Metadata::EpochTypeID FunctionParamExpression::GetParamType(const Namespace& curnamespace) const
{
	if(!MyExpression)
		return Metadata::EpochType_Error;

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
	case Metadata::EpochType_Integer:
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
	case Metadata::EpochType_Integer:
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

//
// Deep copy an unnamed function parameter
//
FunctionParam* FunctionParamTyped::Clone() const
{
	return new FunctionParamTyped(MyType);
}

//
// Named parameters cannot participate in pattern matching
//
void FunctionParamNamed::AddToSignature(FunctionSignature&, const Namespace&) const
{
	throw InternalException("Cannot pattern match on this kind of function parameter");
}

//
// Deep copy a named function parameter
//
FunctionParam* FunctionParamNamed::Clone() const
{
	FunctionParamNamed* clone = new FunctionParamNamed(MyTypeName, TemplateArgs, IsRef);
	clone->MyActualType = MyActualType;
	return clone;
}

//
// Add a template parameter to a function
//
// Allows functions to be templatized on various sorts
// of parameters, either types or values.
//
void Function::AddTemplateParameter(Metadata::EpochTypeID type, StringHandle name)
{
	TemplateParams.push_back(std::make_pair(name, type));
}

//
// Set the arguments passed to a templatized function
//
// Creates mappings for any type parameters to the function,
// so that the code body and parameters and such can look up
// the "actual" type of something. For instance, given a
// function parameterized on type T, which is instantiated
// with T = integer, aliases T to point to integer.
//
void Function::SetTemplateArguments(Namespace& curnamespace, const CompileTimeParameterVector& args)
{
	TemplateArgs = args;
	delete DummyNamespace;
	DummyNamespace = Namespace::CreateTemplateDummy(curnamespace, TemplateParams, TemplateArgs);
}

//
// Helper to replace local variable definitions
// with the correctly typed variables, when a
// function template is instantiated.
//
void Function::FixupScope()
{
	CompileTimeParameterVector argtypes;
	for(size_t i = 0; i < TemplateArgs.size(); ++i)
	{
		if(TemplateParams[i].second == Metadata::EpochType_Wildcard)
		{
			CompileTimeParameter param(L"@templatearg", Metadata::EpochType_Wildcard);

			Metadata::EpochTypeID type = DummyNamespace->Types.GetTypeByName(TemplateArgs[i].Payload.LiteralStringHandleValue);
			param.Payload.IntegerValue = static_cast<int>(type);

			argtypes.push_back(param);
		}
		else
			argtypes.push_back(CompileTimeParameter(L"@templatearg", Metadata::EpochType_Error));
	}

	Code->GetScope()->Fixup(TemplateParams, TemplateArgs, argtypes);
}

//
// Get the name of a parameter's type
//
// This is currently pretty limited, since it
// isn't needed widely.
//
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

//
// Populate a function's scope with its parameters
//
// Done as a separate step so we can use templates to defer
// the types of parameters until instantiation time.
//
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


//
// Add a named function parameter to the function's lexical scope
//
void FunctionParamNamed::AddToScope(StringHandle name, CodeBlock& code, Namespace& curnamespace) const
{
	code.AddVariable(curnamespace.Strings.GetPooledString(name), name, MyTypeName, MyActualType, VARIABLE_ORIGIN_PARAMETER);
}

//
// Add a higher-order function parameter to the function's lexical scope
//
void FunctionParamFuncRef::AddToScope(StringHandle name, CodeBlock& code, Namespace& curnamespace) const
{
	// TODO - redundant code, factor out
	FunctionSignature sig;
	if(ReturnType)
		sig.SetReturnType(curnamespace.Types.GetTypeByName(ReturnType));
	else
		sig.SetReturnType(Metadata::EpochType_Void);
	
	for(std::vector<StringHandle>::const_iterator iter = ParamTypes.begin(); iter != ParamTypes.end(); ++iter)
		sig.AddParameter(L"@@higherorder", curnamespace.Types.GetTypeByName(*iter));

	Metadata::EpochTypeID type = curnamespace.Types.FunctionSignatures.AllocateEpochType(sig);
	code.AddVariable(curnamespace.Strings.GetPooledString(name), name, 0, type, VARIABLE_ORIGIN_PARAMETER);
}

void FunctionParamFuncRef::SubstituteTemplateArgs(const std::vector<std::pair<StringHandle, Metadata::EpochTypeID> >& params, const CompileTimeParameterVector& args, Namespace&, CompileErrors&)
{
	// Modify signature entry types as appropriate
	for(size_t tp = 0; tp < params.size(); ++tp)
	{
		for(size_t i = 0; i < ParamTypes.size(); ++i)
		{
			if(ParamTypes[i] == params[tp].first)
				ParamTypes[i] = args[tp].Payload.LiteralStringHandleValue;
		}

		if(ReturnType == params[tp].first)
			ReturnType = args[tp].Payload.LiteralStringHandleValue;
	}
}


//
// Perform type inference on just a function's parameters
//
// Used for helping to resolve function signatures without
// potentially tripping type inference on their code bodies
// too early.
//
void Function::TypeInferenceParamsOnly(Namespace& curnamespace, CompileErrors& errors)
{
	Namespace* activenamespace;

	if(TemplateArgs.empty())
		activenamespace = &curnamespace;
	else
		activenamespace = DummyNamespace;

	for(std::vector<Param>::const_iterator iter = Parameters.begin(); iter != Parameters.end(); ++iter)
		iter->Parameter->TypeInference(*activenamespace, errors);
}

void Function::TypeInferenceParamsReturnOnly(Namespace& curnamespace, InferenceContext&, CompileErrors& errors)
{
	TypeInferenceParamsOnly(curnamespace, errors);

	if(Return)
	{
		Namespace* activenamespace;

		if(TemplateArgs.empty())
			activenamespace = &curnamespace;
		else
			activenamespace = DummyNamespace;

		InferenceContext newcontext(Name, InferenceContext::CONTEXT_FUNCTION_RETURN);
		newcontext.FunctionName = Name;
		Return->TypeInference(*activenamespace, *Code, newcontext, 0, 1, errors);
	}
}

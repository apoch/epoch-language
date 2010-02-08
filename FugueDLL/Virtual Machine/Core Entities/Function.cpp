//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper class for encapsulating a function
//

#include "pch.h"

#include "Virtual Machine/Core Entities/Function.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"

#include "Virtual Machine/SelfAware.inl"


using namespace VM;


//
// Construct and initialize a function wrapper
//
Function::Function(Program& program, Block* codeblock, ScopeDescription* params, ScopeDescription* returns)
	: CodeBlock(codeblock),
	  Params(params),
	  Returns(returns),
	  RunningProgram(&program)
{
}

//
// Destruct and clean up a function wrapper
//
Function::~Function()
{
	delete CodeBlock;
	delete Params;
	delete Returns;
}

//
// Invoke the function and execute its code
//
RValuePtr Function::Invoke(ExecutionContext& context)
{
	std::auto_ptr<ActivatedScope> paramclone(new ActivatedScope(*Params));
	std::auto_ptr<ActivatedScope> returnclone(new ActivatedScope(*Returns));

	paramclone->BindToStack(context.Stack);
	returnclone->Enter(context.Stack);

	std::auto_ptr<ActivatedScope> codescope(new ActivatedScope(*CodeBlock->GetBoundScope()));
	codescope->TaskOrigin = context.Scope.TaskOrigin;
	codescope->LastMessageOrigin = context.Scope.LastMessageOrigin;
	codescope->ParentScope = &context.Scope;
	codescope->PushNewGhostSet();
	paramclone->GhostIntoScope(*codescope);
	returnclone->GhostIntoScope(*codescope);

	CodeBlock->ExecuteBlock(ExecutionContext(context.RunningProgram, *codescope, context.Stack, context.FlowResult), NULL);
	RValuePtr ret(returnclone->GetEffectiveTuple());
	codescope->Exit(context.Stack);
	
	returnclone->Exit(context.Stack);
	paramclone->Exit(context.Stack);
	codescope->PopGhostSet();
	return ret;
}

//
// Invoke the function and execute its code
//
// This variant of the invocation interface is intended for use by the
// marshalling system, which may invoke functions using direct parameters
// from the host hardware rather than the internal VM stack.
//
RValuePtr Function::InvokeWithExternalParams(ExecutionContext& context, void* externalstack)
{
	std::auto_ptr<ActivatedScope> paramclone(new ActivatedScope(*Params));
	std::auto_ptr<ActivatedScope> returnclone(new ActivatedScope(*Returns));

	paramclone->BindToMachineStack(externalstack);
	returnclone->Enter(context.Stack);

	std::auto_ptr<ActivatedScope> codescope(new ActivatedScope(*CodeBlock->GetBoundScope()));
	codescope->ParentScope = &context.Scope;
	codescope->PushNewGhostSet();
	paramclone->GhostIntoScope(*codescope);
	returnclone->GhostIntoScope(*codescope);

	FlowControlResult flowresult = FLOWCONTROL_NORMAL;
	CodeBlock->ExecuteBlock(ExecutionContext(context.RunningProgram, *codescope, context.Stack, flowresult), NULL);
	RValuePtr ret(returnclone->GetEffectiveTuple());
	codescope->Exit(context.Stack);
	
	returnclone->Exit(context.Stack);
	paramclone->ExitFromMachineStack();
	codescope->PopGhostSet();
	return ret;
}

//
// Retrieve the function's return type
//
EpochVariableTypeID Function::GetType(const ScopeDescription& scope) const
{
	return Returns->GetEffectiveType();
}


template <typename TraverserT>
void Function::TraverseHelper(TraverserT& traverser)
{
	if(Params)
		Params->Traverse(traverser);

	if(Returns)
		Returns->Traverse(traverser);
	
	if(CodeBlock)
		CodeBlock->Traverse(traverser);
}

void Function::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void Function::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraverseHelper(traverser);
}



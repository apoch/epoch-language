//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Logic for handing execution off from the VM to a language extension module
//

#include "pch.h"

#include "Language Extensions/Handoff.h"
#include "Language Extensions/FunctionPointerTypes.h"
#include "Language Extensions/ExtensionCatalog.h"

#include "Virtual Machine/Core Entities/Block.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/SelfAware.inl"

#include "Virtual Machine/Operations/StackOps.h"
#include "Virtual Machine/Operations/Variables/VariableOps.h"
#include "Virtual Machine/Operations/Flow/Invoke.h"
#include "Virtual Machine/Operations/Flow/FlowControl.h"

#include "Utility/Threading/MachineInfo.h"


using namespace Extensions;
using namespace VM;


//
// Construct a handoff operation wrapper
//
HandoffOperation::HandoffOperation(const std::wstring& extensionname, std::auto_ptr<VM::Block> codeblock)
	: ExtensionName(extensionname),
	  CodeBlock(codeblock)
{
	ExtensionHandle = Extensions::GetLibraryProvidingExtension(extensionname);
	
	CodeHandle = Extensions::BindLibraryToCode(ExtensionHandle, extensionname, CodeBlock.get());
	if(!CodeHandle)
		throw Exception("Failed to bind to an Epoch language extension library");
}

HandoffOperation::HandoffOperation(const std::wstring& extensionname, std::auto_ptr<VM::Block> codeblock, Extensions::CodeBlockHandle codehandle)
	: ExtensionName(extensionname),
	  CodeBlock(codeblock),
	  CodeHandle(codehandle)
{
	ExtensionHandle = Extensions::GetLibraryProvidingExtension(extensionname);
}


//
// Invoke the extension code attached to this handoff block
//
void HandoffOperation::ExecuteFast(ExecutionContext& context)
{
	if(Extensions::ExtensionIsAvailableForExecution(ExtensionHandle))
		Extensions::ExecuteBoundCodeBlock(ExtensionHandle, CodeHandle, reinterpret_cast<HandleType>(&context.Scope));
	else
	{
		std::auto_ptr<ActivatedScope> codescope(new ActivatedScope(*CodeBlock->GetBoundScope()));
		codescope->TaskOrigin = context.Scope.TaskOrigin;
		codescope->LastMessageOrigin = context.Scope.LastMessageOrigin;
		codescope->ParentScope = &context.Scope;

		CodeBlock->ExecuteBlock(ExecutionContext(context.RunningProgram, *codescope, context.Stack, context.FlowResult), NULL);
		codescope->Exit(context.Stack);
	}
}

RValuePtr HandoffOperation::ExecuteAndStoreRValue(ExecutionContext& context)
{
	ExecuteFast(context);
	return RValuePtr(new NullRValue);
}


template <typename TraverserT>
void HandoffOperation::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);
	if(CodeBlock.get() != NULL)
		CodeBlock->Traverse(traverser);
}

void HandoffOperation::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void HandoffOperation::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void HandoffOperation::PrepareForExecution()
{
	Extensions::PrepareCodeBlockForExecution(ExtensionHandle, CodeHandle);
}





HandoffControlOperation::HandoffControlOperation(const std::wstring& controlkeyword, Block* body, const std::wstring& countervarname, const VM::ScopeDescription& scope)
	: Body(body),
	  CounterVariableName(countervarname),
	  ExtensionName(controlkeyword)
{
	ExtensionHandle = Extensions::GetLibraryProvidingExtension(controlkeyword);
	
	// TODO - better bindings of local variables to the actual stuff the language extension expects
	Body->InsertHeadOperation(VM::OperationPtr(new VM::Operations::InitializeValue(countervarname)));
	VM::OperationPtr readop(new VM::Operations::Invoke(Body->GetBoundScope()->GetFunction(L"CUDAGetThreadIndex"), false));
	Body->InsertHeadOperation(VM::OperationPtr(new VM::Operations::PushOperation(readop.release(), scope)));

	CodeHandle = Extensions::BindLibraryToCode(ExtensionHandle, controlkeyword, Body);
	if(!CodeHandle)
		throw Exception("Failed to bind to an Epoch language extension library");
}

HandoffControlOperation::HandoffControlOperation(const std::wstring& controlkeyword, Block* body, const std::wstring& countervarname, const VM::ScopeDescription& scope, Extensions::CodeBlockHandle codehandle)
	: Body(body),
	  CounterVariableName(countervarname),
	  ExtensionName(controlkeyword),
	  CodeHandle(codehandle)
{
	ExtensionHandle = Extensions::GetLibraryProvidingExtension(controlkeyword);
}



HandoffControlOperation::~HandoffControlOperation()
{
	delete Body;
}

RValuePtr HandoffControlOperation::ExecuteAndStoreRValue(ExecutionContext& context)
{
	ExecuteFast(context);
	return RValuePtr(new NullRValue);
}


void HandoffControlOperation::ExecuteFast(ExecutionContext& context)
{
	if(Extensions::ExtensionIsAvailableForExecution(ExtensionHandle))
	{
		std::vector<Traverser::Payload> convertedparams;

		const std::vector<Extensions::ExtensionControlParamInfo>& params = Extensions::GetParamsForControl(ExtensionName);
		for(std::vector<Extensions::ExtensionControlParamInfo>::const_iterator iter = params.begin(); iter != params.end(); ++iter)
		{
			if(iter->CreatesLocalVariable)
				continue;

			switch(iter->LocalVariableType)
			{
			case VM::EpochVariableType_Integer:
				{
					IntegerVariable var(context.Stack.GetCurrentTopOfStack());
					Traverser::Payload payload;
					payload.SetValue(var.GetValue());
					convertedparams.push_back(payload);
					context.Stack.Pop(IntegerVariable::GetBaseStorageSize());
				}
				break;

			case VM::EpochVariableType_Integer16:
				{
					Integer16Variable var(context.Stack.GetCurrentTopOfStack());
					Traverser::Payload payload;
					payload.SetValue(var.GetValue());
					convertedparams.push_back(payload);
					context.Stack.Pop(Integer16Variable::GetBaseStorageSize());
				}
				break;

			case VM::EpochVariableType_Real:
				{
					RealVariable var(context.Stack.GetCurrentTopOfStack());
					Traverser::Payload payload;
					payload.SetValue(var.GetValue());
					convertedparams.push_back(payload);
					context.Stack.Pop(RealVariable::GetBaseStorageSize());
				}
				break;

			case VM::EpochVariableType_Boolean:
				{
					BooleanVariable var(context.Stack.GetCurrentTopOfStack());
					Traverser::Payload payload;
					payload.SetValue(var.GetValue());
					convertedparams.push_back(payload);
					context.Stack.Pop(BooleanVariable::GetBaseStorageSize());
				}
				break;

			default:
				throw VM::NotImplementedException("Support for passing parameters of this type to a language extension is not implemented");
			}
		}

		std::reverse(convertedparams.begin(), convertedparams.end());
		Extensions::ExecuteBoundCodeBlock(ExtensionHandle, CodeHandle, reinterpret_cast<HandleType>(&context.Scope), convertedparams);
	}
	else
	{
		// TODO - this makes a hard-coded assumption that the failover should be a parallelfor; implement a more flexible system

		context.Stack.Push(sizeof(Integer32));
		*reinterpret_cast<Integer32*>(context.Stack.GetCurrentTopOfStack()) = Threads::GetCPUCount();

		VM::OperationPtr op(new VM::Operations::ParallelFor(Body, CounterVariableName, false, 2));
		op->ExecuteFast(context);
	}
}

template <typename TraverserT>
void HandoffControlOperation::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);
	traverser.EnterTask();
	if(Body)
		Body->Traverse(traverser);
	traverser.ExitTask();
}

void HandoffControlOperation::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void HandoffControlOperation::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void HandoffControlOperation::PrepareForExecution()
{
	Extensions::PrepareCodeBlockForExecution(ExtensionHandle, CodeHandle);
}


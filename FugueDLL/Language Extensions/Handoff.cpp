//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Logic for handing execution off from the VM to a language extension module
//

#include "pch.h"

#include "Language Extensions/Handoff.h"

#include "Virtual Machine/Core Entities/Block.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/SelfAware.inl"


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
	
	CodeHandle = Extensions::BindLibraryToCode(ExtensionHandle, CodeBlock.get());
	if(!CodeHandle)
		throw Exception("Failed to bind to an Epoch language extension library");
}


//
// Invoke the extension code attached to this handoff block
//
void HandoffOperation::ExecuteFast(ExecutionContext& context)
{
	Extensions::ExecuteBoundCodeBlock(ExtensionHandle, CodeHandle, reinterpret_cast<HandleType>(&context.Scope));
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


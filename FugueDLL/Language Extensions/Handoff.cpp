//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Logic for handing execution off from the VM to a language extension module
//

#include "pch.h"

#include "Language Extensions/Handoff.h"

#include "Virtual Machine/Core Entities/Block.h"
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
void HandoffOperation::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	Extensions::ExecuteBoundCodeBlock(ExtensionHandle, CodeHandle, reinterpret_cast<HandleType>(&scope));
}

RValuePtr HandoffOperation::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteFast(scope, stack, flowresult);
	return RValuePtr(new NullRValue);
}



//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Logic for handing execution off from the VM to a language extension module
//

#pragma once


// Dependencies
#include "Virtual Machine/Core Entities/Operation.h"
#include "Language Extensions/ExtensionCatalog.h"


// Forward declarations
namespace VM
{
	class Block;
}


namespace Extensions
{

	//
	// Virtual machine operation that wraps the logic for handing
	// off program execution to an external extension module
	//
	class HandoffOperation : public VM::Operation, public VM::SelfAware<HandoffOperation>
	{
	// Construction
	public:
		HandoffOperation(const std::wstring& extensionname, std::auto_ptr<VM::Block> codeblock);

	// Operation interface
	public:
		virtual void ExecuteFast(VM::ExecutionContext& context);
		virtual VM::RValuePtr ExecuteAndStoreRValue(VM::ExecutionContext& context);
		
		virtual VM::EpochVariableTypeID GetType(const VM::ScopeDescription& scope) const
		{ return VM::EpochVariableType_Null; }

		virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
		{ return 0; }

	// Internal tracking
	protected:
		std::wstring ExtensionName;
		Extensions::ExtensionLibraryHandle ExtensionHandle;
		std::auto_ptr<VM::Block> CodeBlock;
		Extensions::CodeBlockHandle CodeHandle;
	};

}


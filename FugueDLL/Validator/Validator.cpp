//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrappers for investigating a code tree and ensuring that it is
// valid and safe. Note that this involves more complex static
// checks than are strictly practical within the parser layer; the
// validator runs immediately after parsing and before execution.
//

#include "pch.h"

#include "Validator/Validator.h"

#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/VMExceptions.h"


using namespace Validator;


//
// Construct and initialize a validation traverser
//
ValidationTraverser::ValidationTraverser()
	: Valid(true),
	  TaskDepthCounter(0),
	  CurrentProgram(NULL),
	  CurrentScope(NULL)
{
}

//
// Link the traverser to a program representation object
//
void ValidationTraverser::SetProgram(VM::Program& program)
{
	CurrentProgram = &program;
}

//
// Register that we are handling the global variable initialization block
//
void ValidationTraverser::TraverseGlobalInitBlock(VM::Block* block)
{
	// Nothing to do for validation; the block will be passed in and validated another way
}

//
// Register that we have entered a code block/lexical scope
//
void ValidationTraverser::EnterBlock(const VM::Block& block)
{
	// Nothing to do for validation.
}

//
// Register that we have left a code block/lexical scope
//
void ValidationTraverser::ExitBlock(const VM::Block& block)
{
	// Nothing to do for validation.
}

//
// Register that an optional code block has not been supplied
//
void ValidationTraverser::NullBlock()
{
	// Nothing to do for validation.
}

//
// Set the currently processed lexical scope and validate its contents
//
void ValidationTraverser::RegisterScope(VM::ScopeDescription& scope)
{
	CurrentScope = &scope;
	TraverseScope(scope);
}

//
// Validate the contents of a lexical scope
//
void ValidationTraverser::TraverseScope(VM::ScopeDescription& scope)
{
	for(VM::ScopeDescription::FunctionMap::iterator iter = scope.Functions.begin(); iter != scope.Functions.end(); ++iter)
	{
		VM::SelfAwareBase* func = dynamic_cast<VM::SelfAwareBase*>(iter->second);
		if(func)
			func->Traverse(*this);
	}

	for(VM::ScopeDescription::ResponseMapList::const_iterator iter = scope.ResponseMaps.begin(); iter != scope.ResponseMaps.end(); ++iter)
	{
		VM::ResponseMap* themap = iter->second;
		const std::vector<VM::ResponseMapEntry*>& responses = themap->GetEntries();
		for(std::vector<VM::ResponseMapEntry*>::const_iterator inner_iter = responses.begin(); inner_iter != responses.end(); ++inner_iter)
			(*inner_iter)->GetResponseBlock()->Traverse(*this);
	}
}


//
// Register that we have entered an asynchronous task block
//
void ValidationTraverser::EnterTask()
{
	++TaskDepthCounter;
}

//
// Register that we have left an asynchronous task block
//
void ValidationTraverser::ExitTask()
{
	--TaskDepthCounter;
}


//
// Record a problem found during validation
//
void ValidationTraverser::FlagError(const VM::Operation* op, const std::wstring& errortext)
{
	ValidationError error;
	error.Operation = op;
	error.ErrorText = errortext;

	ErrorList.push_back(error);
}



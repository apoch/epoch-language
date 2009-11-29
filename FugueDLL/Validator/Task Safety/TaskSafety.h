//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Validation logic for ensuring that asynchronous tasks are safe
//

#pragma once


// Forward declarations
namespace VM { class Operation; }


namespace Validator
{

	class ValidationTraverser;

	//
	// Explicit specializations of this function are used to perform
	// the actual validation. Using a template makes it easier to
	// expose the logic to ValidationTraverser::TraverseNode, i.e. we
	// don't have to define lots of overloads by hand for every
	// operation class.
	//
	template <class OperationClass>
	void TaskSafetyCheck(const OperationClass& op, ValidationTraverser& traverser);


	//
	// This class provides a handy way to pass "friend" access over to
	// the validation logic from the traverser code. All the functions
	// needed in the validation process are defined here as members of
	// the class; all of them can obtain access to the traverser class
	// internals with one easy friend declaration.
	//
	class TaskSafetyWrapper
	{
	// Validation checks
	public:
		static void OpInvalidInTask(ValidationTraverser& traverser, const VM::Operation* op);
		static void OpMustNotAccessGlobalState(ValidationTraverser& traverser, const std::wstring& varname, const VM::Operation* op);
	};

}


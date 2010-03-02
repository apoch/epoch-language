//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Routines for integrating with external DLLs
//
// WARNING - all marshalling code is platform-specific!
//

#pragma once

// Dependencies
#include "Virtual Machine/Core Entities/Function.h"


namespace Marshalling
{

	//
	// Operation for invoking functions in external DLLs
	//
	class CallDLL : public VM::FunctionBase, public VM::SelfAware<CallDLL>
	{
	// Construction and destruction
	public:
		CallDLL(const std::wstring& dllname, const std::wstring& functionname, VM::ScopeDescription* params, VM::EpochVariableTypeID returntype, VM::EpochVariableTypeID returntypehint);
		virtual ~CallDLL();

	// Function/operation interface
	public:
		virtual VM::RValuePtr Invoke(VM::ExecutionContext& context);

		virtual VM::ScopeDescription& GetParams()
		{ return *Params; }

		virtual const VM::ScopeDescription& GetParams() const
		{ return *Params; }

		virtual VM::EpochVariableTypeID GetType(const VM::ScopeDescription& scope) const
		{ return ReturnType; }

		virtual VM::EpochVariableTypeID GetTypeHint(const VM::ScopeDescription& scope) const
		{ return GetReturnTypeHint(); }

	// Information retrieval
	public:
		const std::wstring& GetDLLName() const				{ return DLLName; }
		const std::wstring& GetFunctionName() const			{ return FunctionName; }
		VM::EpochVariableTypeID GetReturnType() const		{ return ReturnType; }
		VM::EpochVariableTypeID GetReturnTypeHint() const	{ return ReturnTypeHint; }

	// Traversal
	public:
		template <typename TraverserT>
		void TraverseHelper(TraverserT& traverser);

		virtual void Traverse(Validator::ValidationTraverser& traverser);
		virtual void Traverse(Serialization::SerializationTraverser& traverser);

	// Internal storage
	private:
		std::wstring DLLName;
		std::wstring FunctionName;
		VM::ScopeDescription* Params;
		VM::EpochVariableTypeID ReturnType;
		VM::EpochVariableTypeID ReturnTypeHint;
	};

}


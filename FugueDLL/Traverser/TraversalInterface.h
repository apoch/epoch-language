//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Interface for allowing external libraries to traverse Epoch code
//
// We define a function-pointer-based interface here in order to provide
// a maximally compatible method for integrating language extensions and
// other libraries with the Fugue virtual machine. Although the internal
// code in Fugue uses a more direct C++ interface for similar functions,
// we elect to use a C-style interface in order to allow other compilers
// and languages to interact gracefully with the traversal system.
//

#pragma once


// Dependencies
#include "Utility/Types/EpochTypeIDs.h"


namespace Traverser
{
	
	// Forward declarations
	struct Payload;
	struct ScopeContents;


	typedef HandleType TraversalSessionHandle;


	// Function pointer types
	typedef void (__stdcall *NodeEntryCallbackPtr)(TraversalSessionHandle sessionhandle);
	typedef void (__stdcall *NodeExitCallbackPtr)(TraversalSessionHandle sessionhandle);
	typedef void (__stdcall *NodeTraversalCallbackPtr)(TraversalSessionHandle sessionhandle, const wchar_t* token, const Payload* payload);
	typedef void (__stdcall *ScopeTraversalCallbackPtr)(TraversalSessionHandle sessionhandle, size_t numcontents, const ScopeContents* contents);


	//
	// This is simply a set of callback pointers defining the traversal interface
	//
	// A traverser is expected to provide valid pointers for each of these functions,
	// so that the traversal process can invoke the remote logic as necessary.
	//
	struct Interface
	{
		NodeEntryCallbackPtr NodeEntryCallback;
		NodeExitCallbackPtr NodeExitCallback;
		NodeTraversalCallbackPtr NodeTraversalCallback;
		ScopeTraversalCallbackPtr ScopeTraversalCallback;
	};


	//
	// General wrapper for data chunks
	//
	// This interface is used to provide a more universally compatible
	// means of accessing data in an Epoch program; exporting Fugue's
	// internal C++ interfaces would require a prohibitive degree of
	// compiler lock-in for clients who wish to use other tools.
	//
	struct Payload
	{
		Payload()
			: PointerValue(NULL),
			  Type(VM::EpochVariableType_Error),
			  IsIdentifier(false)
		{ }

		union
		{
			Integer32 Int32Value;
			Integer16 Int16Value;
			Real FloatValue;
			const wchar_t* StringValue;
			bool BoolValue;
			void* PointerValue;
		};
		
		VM::EpochVariableTypeID Type;
		bool IsIdentifier;


		void SetValue(Integer32 value)									{ Int32Value = value; Type = VM::EpochVariableType_Integer; }
		void SetValue(Integer16 value)									{ Int16Value = value; Type = VM::EpochVariableType_Integer16; }
		void SetValue(Real value)										{ FloatValue = value; Type = VM::EpochVariableType_Real; }
		void SetValue(const wchar_t* value)								{ StringValue = value; Type = VM::EpochVariableType_String; }
		void SetValue(bool value)										{ BoolValue = value; Type = VM::EpochVariableType_Boolean; }
		void SetValue(void* value)										{ PointerValue = value; Type = VM::EpochVariableType_Address; }

		template <typename T> T GetValueByType()						{ CannotRetrievePayloadOfThisType(); }	// Generate compile error for invalid types
		template <> Integer32 GetValueByType<Integer32>()				{ return Int32Value; }
		template <> Real GetValueByType<Real>()							{ return FloatValue; }
		template <> const wchar_t* GetValueByType<const wchar_t*>()		{ return StringValue; }
		template <> std::wstring GetValueByType<std::wstring>()			{ return std::wstring(StringValue); }
	};

	
	//
	// Simple POD descriptor of a variable in a lexical scope
	//
	struct ScopeContents
	{
		ScopeContents()
			: Identifier(NULL),
			  Type(VM::EpochVariableType_Error)
		{ }

		const wchar_t* Identifier;
		VM::EpochVariableTypeID Type;
	};

}


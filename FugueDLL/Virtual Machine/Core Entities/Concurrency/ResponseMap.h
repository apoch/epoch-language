//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper object for handling message response maps
//

#pragma once


// Dependencies
#include "Utility/Types/EpochTypeIDs.h"
#include "Virtual Machine/Core Entities/Operation.h"


namespace VM
{
	// Forward declarations
	class Block;


	class ResponseMapEntry
	{
	// Construction and destruction
	public:
		ResponseMapEntry(const std::wstring& messagename, const std::list<EpochVariableTypeID>& payloadtypes, VM::Block* responseblock, VM::ScopeDescription* helperscope)
			: MessageName(messagename),
			  PayloadTypes(payloadtypes),
			  ResponseBlock(responseblock),
			  HelperScope(helperscope)
		{ }

		~ResponseMapEntry();

	// Access interface
	public:
		const std::wstring& GetMessageName() const
		{ return MessageName; }

		const std::list<EpochVariableTypeID>& GetPayloadTypes() const
		{ return PayloadTypes; }

		VM::Block* GetResponseBlock() const
		{ return ResponseBlock; }

		VM::ScopeDescription* GetHelperScope() const
		{ return HelperScope; }

	// Internal tracking
	private:
		const std::wstring& MessageName;
		std::list<EpochVariableTypeID> PayloadTypes;
		VM::Block* ResponseBlock;
		VM::ScopeDescription* HelperScope;
	};

	class ResponseMap
	{
	// Destruction
	public:
		~ResponseMap();

	// Entry management interface
	public:
		void AddEntry(ResponseMapEntry* entry);

		const std::vector<ResponseMapEntry*>& GetEntries() const
		{ return ResponseEntries; }

	// Internal tracking
	private:
		std::vector<ResponseMapEntry*> ResponseEntries;
	};

}


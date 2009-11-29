//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper object for handling message response maps
//

#include "pch.h"

#include "Virtual Machine/Core Entities/Concurrency/ResponseMap.h"

#include "Virtual Machine/Core Entities/Block.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"


using namespace VM;


//
// Destruct and clean up a response map entry wrapper
//
ResponseMapEntry::~ResponseMapEntry()
{
	delete HelperScope;
	delete ResponseBlock;
}

//
// Destruct and clean up a response map wrapper
//
ResponseMap::~ResponseMap()
{
	for(std::vector<ResponseMapEntry*>::iterator iter = ResponseEntries.begin(); iter != ResponseEntries.end(); ++iter)
		delete *iter;
}

//
// Add a response map entry to the response map
//
void ResponseMap::AddEntry(ResponseMapEntry* entry)
{
	ResponseEntries.push_back(entry);
}

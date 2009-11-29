//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Service routines for working with raw bytecode
//

#pragma once


namespace BinaryServices
{
	bool ExecuteFile(const char* filename);
	bool ExecuteMemoryBuffer(const void* buffer);
}


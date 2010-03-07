//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Utility code for reading and writing files
//

#pragma once

namespace Files
{
	void Load(const char* filename, std::vector<Byte>& memory);
	size_t GetFileSize(const char* filename);
}


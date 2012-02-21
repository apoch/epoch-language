//
// The Epoch Language Project
// Shared Library Code
//
// Utility code for reading and writing files
//

#pragma once


// Dependencies
#include <vector>


namespace Files
{
	std::wstring Load(const std::wstring& filename);
	std::vector<std::wstring> GetMatchingFiles(const std::wstring& filespec);
}


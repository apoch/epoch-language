//
// The Epoch Language Project
// Shared Library Code
//
// Utility code for reading and writing files
//

#include "pch.h"

#include "Utility/Files/Files.h"

#include <fstream>


//
// Wrapper for loading a file's contents into a string
//
std::wstring Files::Load(const std::wstring& filename)
{
	std::wifstream infile(filename.c_str(), std::ios::binary);

    if(!infile)
		throw std::exception("Failed to load input file");

	infile.unsetf(std::ios::skipws);

	return std::wstring(std::istream_iterator<wchar_t, wchar_t>(infile), std::istream_iterator<wchar_t, wchar_t>());
}



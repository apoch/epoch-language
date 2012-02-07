//
// The Epoch Language Project
// Shared Library Code
//
// Utility code for reading and writing files
//

#include "pch.h"

#include "Utility/Files/Files.h"
#include "Utility/Strings.h"

#include <fstream>
#include <iterator>

//
// Wrapper for loading a file's contents into a string
//
std::wstring Files::Load(const std::wstring& filename)
{
	std::wifstream infile(filename.c_str(), std::ios::binary);

    if(!infile)
		throw FileException("Could not open the input file " + narrow(filename));

	infile.unsetf(std::ios::skipws);

	return std::wstring(std::istream_iterator<wchar_t, wchar_t>(infile), std::istream_iterator<wchar_t, wchar_t>());
}



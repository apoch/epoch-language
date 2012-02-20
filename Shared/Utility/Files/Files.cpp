//
// The Epoch Language Project
// Shared Library Code
//
// Utility code for reading and writing files
//

#include "pch.h"

#include "Utility/Files/Files.h"
#include "Utility/Files/FilesAndPaths.h"
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

//
// Wrapper for finding all files matching a specification (supports wildcards)
//
std::vector<std::wstring> Files::GetMatchingFiles(const std::wstring& filespec)
{
	std::vector<std::wstring> ret;

#ifdef BOOST_WINDOWS
	std::wstring path = StripFilename(filespec);

	WIN32_FIND_DATA find;
	HANDLE findhandle = ::FindFirstFile(filespec.c_str(), &find);
	if(findhandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			ret.push_back(path + std::wstring(find.cFileName));
		}
		while(::FindNextFile(findhandle, &find));
	}
#else
#warning Implement on non-Windows platforms
#endif

	return ret;
}


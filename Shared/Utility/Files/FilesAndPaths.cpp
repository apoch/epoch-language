//
// The Epoch Language Project
// Shared Library Code
//
// Common library functions for working with filenames and paths
//

#include "pch.h"
#include "Utility/Files/FilesAndPaths.h"


//
// Helper for stripping file extensions
//
std::wstring StripExtension(const std::wstring& filename)
{
	std::wstring::size_type dotpos = filename.find_last_of(L'.');
	std::wstring::size_type slashpos = filename.find_last_of(L'\\');

	if(dotpos == std::wstring::npos)
		return filename;

	if(slashpos == std::wstring::npos)
		return filename.substr(0, dotpos);

	if(slashpos > dotpos)
		return filename;

	return filename.substr(0, dotpos);
}


//
// Helper for stripping file paths
//
std::wstring StripPath(const std::wstring& filename)
{
	std::wstring::size_type slashpos = filename.find_last_of(L'\\');
	if(slashpos == std::wstring::npos)
		return filename;

	return filename.substr(slashpos + 1);
}


//
// Helper for stripping file names off of path specifiers
//
std::wstring StripFilename(const std::wstring& path)
{
	std::wstring::size_type slashpos = path.find_last_of(L'\\');
	if(slashpos == std::wstring::npos)
		return L".\\";

	return path.substr(0, slashpos + 1);
}


//
// Helper for reducing a path containing spaces to a sanitized "short" path
// Note that this is platform-specific for Windows.
//
std::wstring ShortenPathName(const std::wstring& path)
{
	if(path.empty())
		return L"";

	DWORD size = ::GetShortPathName(path.c_str(), NULL, 0) + 2;
	std::vector<wchar_t> buffer(size, L'\0');
	::GetShortPathName(path.c_str(), &buffer[0], size);
	return std::wstring(buffer.begin(), buffer.end() - 3);	// strip NULL terminator
}


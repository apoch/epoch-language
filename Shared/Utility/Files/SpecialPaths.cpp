//
// The Epoch Language Project
// Shared Library Code
//
// Routines for locating special directories
//

#include "pch.h"

#include "Utility/Files/SpecialPaths.h"

#include <ShlObj.h>

#include <vector>


std::wstring SpecialPaths::InvalidPath = L"@";


namespace
{
	//
	// Internal helper for retrieving specially defined paths on Windows
	//
	std::wstring GetGeneralPath(int csidl)
	{
		WCHAR path[MAX_PATH + 1] = {0,};

		if(::SHGetFolderPath(NULL, csidl, NULL, SHGFP_TYPE_CURRENT, path) != S_OK)
			return SpecialPaths::InvalidPath;

		return std::wstring(path);
	}
}


//
// Retrieve a path where we can store configuration data etc.
//
std::wstring SpecialPaths::GetAppDataPath()
{
	return GetGeneralPath(CSIDL_APPDATA);
}

//
// Retrieve the path where we can expect to find system files (i.e. System32 on Windows)
//
std::wstring SpecialPaths::GetSystemPath()
{
	return GetGeneralPath(CSIDL_SYSTEM);
}

//
// Retrieve a path where we can store temporary files
//
std::wstring SpecialPaths::GetTemporaryPath()
{
	std::vector<wchar_t> buffer(MAX_PATH + 2, L'\0');
	::GetTempPath(static_cast<DWORD>(buffer.size()), &buffer[0]);
	std::wstring path(buffer.begin(), buffer.end());
	size_t slashpos = path.rfind(L'\\');
	if(slashpos != std::wstring::npos)
		path = path.substr(0, slashpos);
	return path;
}


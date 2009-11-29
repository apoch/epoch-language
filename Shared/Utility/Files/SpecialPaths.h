//
// The Epoch Language Project
// Shared Library Code
//
// Routines for locating special directories
//

#pragma once


namespace SpecialPaths
{

	extern std::wstring InvalidPath;

	std::wstring GetAppDataPath();
	std::wstring GetSystemPath();
	std::wstring GetTemporaryPath();

}


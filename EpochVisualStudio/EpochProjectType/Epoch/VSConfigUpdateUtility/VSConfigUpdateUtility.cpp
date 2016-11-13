// VSConfigUpdateUtility.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


int main()
{
	PWSTR pwstr;
	::SHGetKnownFolderPath(FOLDERID_ProgramFilesX86, 0, 0, &pwstr);


	WCHAR path[MAX_PATH];
	::wsprintf(path, L"%s\\Microsoft Visual Studio 14.0\\Common7\\IDE\\Extensions\\extensions.configurationchanged", pwstr);


	HANDLE h = ::CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if(h != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(h);
		/*::MessageBox(NULL, L"Success", L"VSUpdateConfiguration", 0);*/
	}
	else
	{
		/*::MessageBox(NULL, L"FAILURE", L"VSUpdateConfiguration", 0);*/
	}

	::CoTaskMemFree(pwstr);

    return 0;
}


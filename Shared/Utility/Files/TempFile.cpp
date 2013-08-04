//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper for working with temporary data files
//

#include "pch.h"

#include "Utility/Files/TempFile.h"
#include "Utility/Files/SpecialPaths.h"

#include "Utility/Strings.h"

#include <boost/filesystem/operations.hpp>


std::set<std::wstring> TemporaryFileWriter::AllGeneratedFiles;


//
// Construct the temporary file wrapper
//
// This process will create a temporary file using the appropriate
// OS-specific logic, then open an output stream to the file so that
// clients can write to the file directly. The file is closed and
// committed when the wrapper is destructed.
//
TemporaryFileWriter::TemporaryFileWriter(std::ios_base::openmode mode, const std::wstring& forceextension)
{
	std::vector<wchar_t> buffer(MAX_PATH + 2, L'\0');
	::GetTempFileName(SpecialPaths::GetTemporaryPath().c_str(), L"elp", 0, &buffer[0]);
	FileName = std::wstring(buffer.begin(), buffer.end());
	FileName = FileName.substr(0, FileName.find(L'\0'));

	if(!forceextension.empty())
	{
		boost::filesystem::remove(narrow(FileName).c_str());
		FileName = FileName.substr(0, FileName.rfind(L'.') + 1) + forceextension;
	}

	AllGeneratedFiles.insert(FileName);

	OutputStream.open(FileName.c_str(), mode);
}


//
// Clean up all temporary files created during the course of program execution
//
void TemporaryFileWriter::RemoveAllFiles()
{
	for(std::set<std::wstring>::const_iterator iter = AllGeneratedFiles.begin(); iter != AllGeneratedFiles.end(); ++iter)
		boost::filesystem::remove(narrow(*iter).c_str());

	AllGeneratedFiles.clear();
}


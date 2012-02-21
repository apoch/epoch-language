//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper for working with temporary data files
//

#pragma once


// Dependencies
#include <fstream>
#include <set>


class TemporaryFileWriter
{
// Construction
public:
	TemporaryFileWriter(std::ios_base::openmode mode = std::ios_base::out, const std::wstring& forceextension = L"");

// Output interface
public:
	std::wofstream OutputStream;

// Filename access interface
public:
	const std::wstring& GetFileName() const
	{ return FileName; }

// Cleanup interface
public:
	static void RemoveAllFiles();

// Internal tracking
private:
	std::wstring FileName;

	static std::set<std::wstring> AllGeneratedFiles;
};


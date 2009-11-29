//
// The Epoch Language Project
// Win32 EXE Generator
//
// Wrapper object for managing Epoch project files
//

#pragma once


namespace Projects
{

	class Project
	{
	// Construction
	public:
		explicit Project(const std::wstring& filename);

	// Project information retrieval
	public:
		const std::wstring& GetIntermediatesPath() const			{ return IntermediatesPath; }
		const std::wstring& GetOutputPath() const					{ return OutputPath; }
		const std::wstring& GetOutputFileName() const				{ return OutputFileName; }

		const std::list<std::wstring>& GetSourceFileList() const	{ return SourceFiles; }
		const std::list<std::wstring>& GetResourceFileList() const	{ return ResourceFiles; }

		std::wstring GetAssemblyFileName(const std::wstring& sourcefilename) const;
		std::wstring GetBinaryFileName(const std::wstring& sourcefilename) const;

	// Internal tracking
	private:
		std::wstring IntermediatesPath;
		std::wstring OutputPath;
		std::wstring OutputFileName;

		std::list<std::wstring> SourceFiles;
		std::list<std::wstring> ResourceFiles;
	};

}


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
	// Construction and destruction
	public:
		Project(const std::wstring& codefilename, const std::wstring& outputfilename, bool useconsole);
		explicit Project(const std::wstring& filename);

		~Project();

	// Project information retrieval
	public:
		const std::wstring& GetIntermediatesPath() const			{ return IntermediatesPath; }
		const std::wstring& GetOutputPath() const					{ return OutputPath; }
		const std::wstring& GetOutputFileName() const				{ return OutputFileName; }
		std::wstring GetQualifiedOutputFilename() const;

		const std::list<std::wstring>& GetSourceFileList() const	{ return SourceFiles; }
		const std::list<std::wstring>& GetResourceFileList() const	{ return ResourceFiles; }

		std::wstring GetAssemblyFileName(const std::wstring& sourcefilename) const;
		std::wstring GetBinaryFileName(const std::wstring& sourcefilename) const;

		bool GetUsesConsoleFlag() const								{ return UsesConsole; }

	// Internal tracking
	private:
		std::wstring IntermediatesPath;
		std::wstring OutputPath;
		std::wstring OutputFileName;

		std::list<std::wstring> SourceFiles;
		std::list<std::wstring> ResourceFiles;

		std::list<std::wstring> TemporaryFiles;

		bool UsesConsole;
	};

}


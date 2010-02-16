//
// The Epoch Language Project
// Win32 EXE Generator
//
// Wrapper object for managing Epoch project files
//

#include "pch.h"

#include "Project.h"

#include "Utility/Files/FilesAndPaths.h"
#include "Utility/Strings.h"

#include <boost/filesystem/operations.hpp>


using namespace Projects;


//
// Construct a project wrapper for a stand-alone Epoch source file with no resources
//
Project::Project(const std::wstring& codefilename, const std::wstring& outputfilename, bool useconsole)
	: UsesConsole(useconsole)
{
	IntermediatesPath = widen(boost::filesystem::current_path().string());
	if(*IntermediatesPath.rbegin() != L'\\')
		IntermediatesPath += L'\\';

	OutputPath = IntermediatesPath;
	OutputFileName = outputfilename;

	SourceFiles.push_back(codefilename);

	std::wstring tempfilenameroot = IntermediatesPath + StripPath(StripExtension(codefilename));
	TemporaryFiles.push_back(StripExtension(tempfilenameroot) + L".easm");
	TemporaryFiles.push_back(StripExtension(tempfilenameroot) + L".epb");
}

//
// Construct a project wrapper by reading a project file from disk
//
Project::Project(const std::wstring& filename)
{
	// Defaults, in case they are omitted from the project file
	IntermediatesPath = L"build\\";
	OutputPath = L"compiled\\";
	OutputFileName = OutputPath + StripPath(StripExtension(filename)) + L".exe";
	UsesConsole = false;


	std::wifstream infile(filename.c_str());
	std::wstring line;

	if(!infile)
		throw FileException("Unable to open project file");

	while(true)
	{
		std::getline(infile, line);
		if(infile.eof())
			break;

		if(line == L"[source]")
		{
			while(true)
			{
				std::getline(infile, line);
				if(line.empty() || infile.eof())
					break;

				SourceFiles.push_back(line);
			}
		}
		else if(line == L"[resources]")
		{
			while(true)
			{
				std::getline(infile, line);
				if(line.empty() || infile.eof())
					break;

				ResourceFiles.push_back(line);
			}
		}
		else if(line == L"[output]")
		{
			while(true)
			{
				std::getline(infile, line);
				if(line.empty() || infile.eof())
					break;

				size_t spacepos = line.find(L' ');
				if(spacepos == std::wstring::npos || spacepos >= line.length() - 1)
					throw Exception("Invalid directive in [output] block");

				std::wstring directive = line.substr(0, spacepos);
				std::wstring parameter = line.substr(spacepos + 1);

				if(directive == L"intermediates-path")
				{
					IntermediatesPath = parameter;
					if(*IntermediatesPath.rbegin() != L'\\')
						IntermediatesPath += L'\\';
				}
				else if(directive == L"output-path")
				{
					OutputPath = parameter;
					if(*OutputPath.rbegin() != L'\\')
						OutputPath += L'\\';
				}
				else if(directive == L"output-file")
					OutputFileName = parameter;
			}
		}
		else if(line == L"[options]")
		{
			while(true)
			{
				std::getline(infile, line);
				if(line.empty() || infile.eof())
					break;

				if(line == L"use-console")
					UsesConsole = true;
			}
		}
	}

	// Validate the file we just loaded
	if(SourceFiles.empty())
		throw Exception("Project must include at least one source file before it can be compiled!");
}

//
// Destruct a project and clean up any files marked for deletion
//
Project::~Project()
{
	for(std::list<std::wstring>::const_iterator iter = TemporaryFiles.begin(); iter != TemporaryFiles.end(); ++iter)
		boost::filesystem::remove(narrow(*iter).c_str());
}


//
// Given a source code file, determine the filename of the corresponding assembly language file
//
std::wstring Project::GetAssemblyFileName(const std::wstring& sourcefilename) const
{
	std::wstring strippedname = StripPath(StripExtension(sourcefilename));
	return IntermediatesPath + strippedname + L".easm";
}

//
// Given a source code file, determine the filename of the corresponding assembled binary file
//
std::wstring Project::GetBinaryFileName(const std::wstring& sourcefilename) const
{
	std::wstring strippedname = StripPath(StripExtension(sourcefilename));
	return IntermediatesPath + strippedname + L".epb";
}


//
// Return the fully qualified path to the output file
//
std::wstring Project::GetQualifiedOutputFilename() const
{
	if(StripPath(OutputFileName) == OutputFileName)
		return OutputPath + OutputFileName;

	return OutputFileName;
}

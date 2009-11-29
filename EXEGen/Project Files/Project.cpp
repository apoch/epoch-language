//
// The Epoch Language Project
// Win32 EXE Generator
//
// Wrapper object for managing Epoch project files
//

#include "pch.h"

#include "Project.h"
#include "Utility/Files/FilesAndPaths.h"


using namespace Projects;


//
// Construct a project wrapper by reading a project file from disk
//
Project::Project(const std::wstring& filename)
{
	// Defaults, in case they are omitted from the project file
	IntermediatesPath = L"build\\";
	OutputPath = L"compiled\\";
	OutputFileName = StripExtension(filename) + L".exe";


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
	}

	// Validate the file we just loaded
	if(SourceFiles.empty())
		throw Exception("Project must include at least one source file before it can be compiled!");
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



//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper for accessing the Epoch configuration file
//

#include "pch.h"

#include "Configuration/ConfigFile.h"

#include "Utility/Files/SpecialPaths.h"

#include <fstream>
#include <string>


using namespace Config;


//
// Construct the configuration reader and load the config data
//
// Note that this is designed to fail safely and quietly; should anything
// go wrong during the config load process, there will *not* be any error
// indication! Since the Epoch virtual machine uses this config file when
// running compiled Epoch .EXE programs, it's best not to alarm end users
// if something happens.
//
ConfigReader::ConfigReader()
{
	std::wstring filename = SpecialPaths::GetAppDataPath() + L"\\epoch.cfg";

	try
	{
		std::wifstream infile(filename.c_str());
		if(!infile)
			return;

		std::wstring line;
		while(infile)
		{
			std::getline(infile, line);
			size_t equalpos = line.find(L'=');
			if(equalpos == std::string::npos)
				continue;

			std::wstring keyname = line.substr(0, equalpos);
			std::wstring value = line.substr(equalpos + 1);

			if(keyname.empty() || value.empty())
				continue;

			Values.insert(KeyValuePair(keyname, value));
		}
	}
	catch(const std::exception&)
	{
		return;
	}
}

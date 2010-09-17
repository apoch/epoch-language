//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper for accessing the Fugue configuration file
//

#pragma once


// Dependencies
#include <map>
#include <sstream>


namespace Config
{

	typedef std::pair<std::wstring, std::wstring> KeyValuePair;
	typedef std::map<std::wstring, std::wstring> KeyValueMap;


	class ConfigReader
	{
	// Construction
	public:
		ConfigReader();

	// Configuration access
	public:

		//
		// Read the requested configuration option's value
		//
		template <typename T>
		T ReadConfig(const std::wstring& keyname) const
		{
			T retval;

			KeyValueMap::const_iterator iter = Values.find(keyname);
			if(iter == Values.end())
				return retval;

			std::wstringstream stream;
			stream << iter->second;
			stream >> retval;

			return retval;
		}

		//
		// Read the requested configuration option's value, preserving
		// the default value passed in the "out" parameter should the
		// requested option not be present in the config file.
		//
		template <typename T>
		void ReadConfig(const std::wstring& keyname, T& out) const
		{
			KeyValueMap::const_iterator iter = Values.find(keyname);
			if(iter == Values.end())
				return;

			std::wstringstream stream;
			stream << iter->second;
			stream >> out;
		}


		//
		// Read the requesting configuration option's string value
		//
		// We specialize for strings in order to allow white space
		// in the value; the above implementation based on streams
		// will not correctly handle spaces in the value string.
		//
		template <>
		std::wstring ReadConfig<std::wstring>(const std::wstring& keyname) const
		{
			KeyValueMap::const_iterator iter = Values.find(keyname);
			if(iter == Values.end())
				return std::wstring();

			return iter->second;
		}

	// Internal tracking
	private:
		KeyValueMap Values;
	};

}


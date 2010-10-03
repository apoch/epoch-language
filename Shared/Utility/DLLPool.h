//
// The Epoch Language Project
// Shared Library Code
//
// Helper class for loading and managing DLLs
//

#pragma once


// Dependencies
#include "Utility/Threading/Synchronization.h"

#include <string>
#include <map>


namespace Marshaling
{

	//
	// Helper class for loading DLLs as needed. By default a library is kept
	// active once it has been loaded. This interface ensures that the library
	// is only opened once, regardless of how many calls are made.
	//
	class DLLPool
	{
	// Destruction
	public:
		~DLLPool()
		{
			for(std::map<std::wstring, HINSTANCE>::iterator iter = LoadedDLLs.begin(); iter != LoadedDLLs.end(); ++iter)
				::FreeLibrary(iter->second);
		}

	// DLL loading interface
	public:
		HINSTANCE OpenDLL(const std::wstring& name)
		{
			Threads::CriticalSection::Auto lock(CritSec);
			
			std::map<std::wstring, HINSTANCE>::const_iterator iter = LoadedDLLs.find(name);
			if(iter != LoadedDLLs.end())
				return iter->second;

			HINSTANCE hdll = ::LoadLibrary(name.c_str());
			if(!hdll)
				throw FatalException("Failed to load external DLL");

			LoadedDLLs.insert(std::make_pair(name, hdll));
			return hdll;
		}

		bool HasOpenedDLL(const std::wstring& name) const
		{
			Threads::CriticalSection::Auto lock(CritSec);
			return (LoadedDLLs.find(name) != LoadedDLLs.end());
		}

	// Internal tracking
	private:
		std::map<std::wstring, HINSTANCE> LoadedDLLs;
		Threads::CriticalSection CritSec;
	};


	// Global shared pool
	extern DLLPool TheDLLPool;

}


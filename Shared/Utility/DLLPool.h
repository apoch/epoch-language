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
#ifdef BOOST_WINDOWS
        typedef HINSTANCE DLLPoolHandle;
#else
        typedef void* DLLPoolHandle;
#endif

		~DLLPool()
		{
			for(std::map<std::wstring, DLLPoolHandle>::iterator iter = LoadedDLLs.begin(); iter != LoadedDLLs.end(); ++iter)
            {
#ifdef BOOST_WINDOWS
				::FreeLibrary(iter->second);
#else
                dlclose(iter->second);
#endif
            }
		}

	// DLL loading interface
	public:
		DLLPoolHandle OpenDLL(const std::wstring& name)
		{
			Threads::CriticalSection::Auto lock(CritSec);
			
            std::map<std::wstring, DLLPoolHandle>::const_iterator iter = LoadedDLLs.find(name);
			if(iter != LoadedDLLs.end())
				return iter->second;

#ifdef BOOST_WINDOWS
			DLLPoolHandle hdll = ::LoadLibrary(name.c_str());
#else
            DLLPoolHandle hdll = dlopen(std::string(name.begin(), name.end()).c_str(), RTLD_LAZY);
#endif

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

        template<typename Signature>
        static Signature GetFunction(DLLPoolHandle handle, std::string const& name) 
        {
#ifdef BOOST_WINDOWS
            return reinterpret_cast<Signature>(::GetProcAddress(handle, name.c_str()));
#else
            return reinterpret_cast<Signature>(dlsym(handle, name.c_str()));
#endif
        }
	// Internal tracking
	private:
		std::map<std::wstring, DLLPoolHandle> LoadedDLLs;
		Threads::CriticalSection CritSec;
    };


	// Global shared pool
	extern DLLPool TheDLLPool;

}


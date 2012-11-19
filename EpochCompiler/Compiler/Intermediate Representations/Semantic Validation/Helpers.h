//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Semantic validation helper routines
//

#pragma once


// Dependencies
#include "Utility/Types/IDTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include <vector>
#include <boost/unordered_map.hpp>


// Forward declarations
class CompileErrors;


namespace IRSemantics
{

	// Forward declarations
	class Namespace;
	class CodeBlock;

	// Helper functions
	Metadata::EpochTypeID InferMemberAccessType(const std::vector<StringHandle>& accesslist, const Namespace& curnamespace, const CodeBlock& activescope, CompileErrors& errors);


	namespace impl
	{
		//
		// String caches are used to handle various name/identifier
		// lookups and make them a bit faster than going through
		// potentially several layers of indirection.
		//
		template<typename T>
		class StringCache
		{
		private:
			typedef boost::unordered_map<T, StringHandle> CacheType;

		public:
			StringHandle Find(const T& key) const
			{
                typename CacheType::const_iterator iter = Cache.find(key);
				if(iter == Cache.end())
					return 0;

				return iter->second;
			}

			void Add(const T& key, StringHandle str)
			{
				Cache[key] = str;
			}

		private:
			CacheType Cache;
		};
	}


}


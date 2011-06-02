//
// The Epoch Language Project
// Shared Library Code
//
// Simple handle allocation manager for ensuring that handle values are
// recycled cleanly and do not overflow the boundaries of the base type
// used to store them.
//
// WARNING: no thread safety is provided by this class! Ensure that the
// associated containers are protected via mutual exclusion/other means
// prior to using the handle allocation routines. Otherwise handles may
// be allocated more than once.
//

#pragma once


// Dependencies
#include <limits>
#include <algorithm>


template <typename HandleT>
class HandleAllocator
{
public:
	//
	// Allocate a handle safely, using the given container as a guide of
	// which handle values are already in use. Assumes the container has
	// semantics equivalent to std::map for accesses/searches.
	//
	template <class ContainerT>
	HandleT AllocateHandle(const ContainerT& container)
	{
		// Early exit check
		if(container.empty())
			return 1;

		// Order of operations is significant here! Otherwise we risk overflow
		const HandleT maxhandle = (std::numeric_limits<HandleT>::max() / 4) * 3;

		// If we haven't run out of monotonically allocated handles, then we
		// can simply increment the last provided handle value and return it
		HandleT largestusedhandle = container.rbegin()->first;
		if(largestusedhandle < maxhandle)
			return ++largestusedhandle;

		// Otherwise, we need to do a quick search of the container looking
		// for the first unused handle value we can return
		return SearchForUnusedHandle(container, largestusedhandle / 2);
	}

private:
	//
	// Search for an unused handle value in a given container
	//
	template <class ContainerT>
	HandleT SearchForUnusedHandle(const ContainerT& container, HandleT pivot)
	{
		ContainerT::const_iterator iter = container.lower_bound(pivot);
		if(iter->first != pivot)
			return pivot;

		if(pivot > 1)
		{
			HandleT attempt = SearchForUnusedHandle(container, pivot / 2);
			if(attempt)
				return attempt;
		}

		HandleT nextpivot = (pivot / 2) * 3;
		if((nextpivot < pivot) || (nextpivot > (std::numeric_limits<HandleT>::max() / 4) * 3))
			return 0;

		return SearchForUnusedHandle(container, nextpivot);
	}
};


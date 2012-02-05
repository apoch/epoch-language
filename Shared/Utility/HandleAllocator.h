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
	HandleAllocator()
		: CurrentMonotonic(1)
	{ }

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

		const HandleT maxhandle = GetMaxHandleValue();

		// If we haven't run out of monotonically allocated handles, then we
		// can simply increment the last provided handle value and return it
		if(CurrentMonotonic < maxhandle)
			return ++CurrentMonotonic;

		// Otherwise, we need to do a quick search of the container looking
		// for the first unused handle value we can return
		HandleT ret = SearchForUnusedHandle(container, 1, CurrentMonotonic);
		if(!ret)
			throw Exception("Handle values exhausted!");

		return ret;
	}

private:
	//
	// Compute the largest safe handle value we can use
	//
	static HandleT GetMaxHandleValue()
	{
		// Order of operations is significant here! Otherwise we risk overflow
		return (std::numeric_limits<HandleT>::max() / 4) * 3;
	}

	//
	// Search for an unused handle value in a given container
	//
	template <class ContainerT>
	HandleT SearchForUnusedHandle(const ContainerT& container, HandleT min, HandleT max)
	{
		if(min >= max)
			return 0;

		HandleT pivot = ((max - min) / 2) + min;	// Order of operations is necessary to prevent overflow!
		ContainerT::const_iterator iter = container.find(pivot);
		if(iter == container.end())
			return pivot;

		HandleT attempt = SearchForUnusedHandle(container, min, pivot - 1);
		if(attempt)
			return attempt;

		attempt = SearchForUnusedHandle(container, pivot + 1, max);
		if(attempt)
			return attempt;

		return 0;
	}

	HandleT CurrentMonotonic;
};


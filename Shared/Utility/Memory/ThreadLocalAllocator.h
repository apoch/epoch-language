//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// A lock-free allocator local to the current thread
//

#pragma once


// Dependencies
#include <limits>


template<typename T> class ThreadLocalAllocator
{
public:
	typedef T value_type;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;
	typedef value_type& reference;
	typedef const value_type& const_reference;

	template<typename Other> struct rebind
	{
		typedef ThreadLocalAllocator<Other> other;
	};

public:
	ThreadLocalAllocator() throw()
	{
	}

	ThreadLocalAllocator(const ThreadLocalAllocator<T>& rhs) throw()
	{
	}

	template<typename OtherT> ThreadLocalAllocator(const ThreadLocalAllocator<OtherT>& rhs) throw()
	{
	}

	~ThreadLocalAllocator() throw()
	{
	}

	pointer address(reference ref) const
	{
		return &ref;
	}

	const_pointer address(const_reference ref) const
	{
		return &ref;
	}

	size_type max_size() const
	{
		return std::numeric_limits<size_type>::max();
	}

	pointer allocate(size_type numelements)
	{
		void* ThreadLocalAlloc(size_t size);

		size_t size = numelements * sizeof(T);

		return static_cast<pointer>(ThreadLocalAlloc(size));
	}

	void deallocate(pointer ptr, size_type numelements)
	{
		void ThreadLocalFree(void* ptr);
		ThreadLocalFree(ptr);
	}

	void construct(pointer ptr, const_reference value) const
	{
		::new(static_cast<void*>(ptr)) T(value);
	}

	void destroy(pointer ptr) const
	{
		(ptr)->~T();
	}
};


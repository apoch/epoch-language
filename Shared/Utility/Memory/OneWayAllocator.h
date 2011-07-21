#pragma once

namespace Memory
{

	void* OneWayAllocate(size_t bytes);
	void OneWayRecordDealloc(size_t bytes);

	void DisposeOneWayBlocks();

	template <typename T>
	T* OneWayAllocateObject(size_t count)
	{
		return reinterpret_cast<T*>(OneWayAllocate(sizeof(T) * count));
	}

	template <typename T>
	void OneWayRecordDeallocObject(T* p)
	{
		p->~T();
		OneWayRecordDealloc(sizeof(T));
	}


	template <typename T>
	class OneWayAlloc
	{
	public:
		typedef T value_type;
		typedef value_type* pointer;
		typedef const value_type* const_pointer;
		typedef value_type& reference;
		typedef const value_type& const_reference;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

	public:
		template<typename U>
		struct rebind
		{
			typedef OneWayAlloc<U> other;
		};

	public:
		explicit OneWayAlloc() {}
		~OneWayAlloc() {}
		explicit OneWayAlloc(OneWayAlloc const&) {}
		template<typename U>
		explicit OneWayAlloc(OneWayAlloc<U> const&) {}

		pointer address(reference r) { return &r; }
		const_pointer address(const_reference r) { return &r; }

		pointer allocate(size_type cnt, typename std::allocator<void>::const_pointer = 0)
		{
			return reinterpret_cast<pointer>(OneWayAllocate(cnt * sizeof(T)));
		}

		void deallocate(pointer p, size_type count)
		{
			OneWayRecordDealloc(count * sizeof(T));
		}

		size_type max_size() const
		{
			return std::numeric_limits<size_type>::max() / sizeof(T);
		}

		void construct(pointer p, const T& t)
		{
			new(p) T(t);
		}

		void destroy(pointer p)
		{
			p->~T();
		}

		bool operator==(OneWayAlloc const&) { return true; }
		bool operator!=(OneWayAlloc const& a) { return !operator==(a); }
	};

}


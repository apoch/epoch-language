//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Deferred-construction wrappers for AST nodes and containers
//
// The motivation for this is simple: as boost::spirit::qi parses the
// source, it eagerly constructs AST nodes every time it attempts to
// match a production in the grammar. This incurs a very high overhead
// if the node type is not cheap to construct. To avoid this, we use
// a simple wrapper class that does not construct the AST node itself
// until the production actually succeeds; furthermore, the node data
// is never copied by value. Instead, a pointer to the original node
// is handed off safely using reference counting. (Reference counts
// can be supplied by any suitable smart pointer; we tend to use a
// boost::intrusive_ptr where possible for minimal overhead, and a
// boost::shared_ptr where intrusive reference counting is impossible,
// such as for AST nodes which are simply boost::variants.)
//
// Measurement has shown this to be a substantial win for performance,
// despite the traversal overhead of extra indirection. The strategy
// is to create this deferred structure during parsing, then convert
// to an intermediate representation as soon as possible to eliminate
// the indirection overhead during further AST operations.
//
// Note that we do not allocate memory directly here, but rather allow
// classes to specify their own individual allocation behaviors via
// the Allocate<> and Deallocate<> template functions.
//

#pragma once


namespace AST
{

	//
	// Deferred construction wrapper for AST nodes
	//
	template <typename T, typename PointerType = boost::shared_ptr<T> >
	struct Deferred
	{
		//
		// Default-construct the wrapper with no contents
		//
		Deferred()
			: Contents(reinterpret_cast<T*>(NULL))
		{
			Content.Owner = this;
		}

		//
		// Construct the wrapper and copy a given value
		//
		Deferred(const T& expr)
			: Contents(new (Allocate<T>()) T(expr))
		{
			Content.Owner = this;
		}

		//
		// Copy the contents of a matching deferred wrapper
		//
		Deferred(const Deferred& rhs)
			: Contents(rhs.Contents)
		{
			Content.Owner = this;
		}

		//
		// Construct the wrapper around a value, where the
		// wrapper itself holds a variant rather than the
		// direct type being passed in. This overload is
		// necessary because the other value-copy constructor
		// demands that the contained type match the passed
		// type; this constructor allows us to construct the
		// wrapper from any type that can be converted into
		// the contained type.
		//
		template <typename VariantContentT>
		Deferred(const VariantContentT& content)
			: Contents(new (Allocate<T>()) T(content))
		{
			Content.Owner = this;
		}

		//
		// Assignment operator
		//
		// Relies on the underlying smart pointer class to
		// perform the correct copy semantics (i.e. should
		// only increase reference count, not build a true
		// copy of the object).
		//
		Deferred& operator = (const Deferred& rhs)
		{
			if(this != &rhs)
				Contents = rhs.Contents;
			return *this;
		}

		//
		// Wrapper for accessing the content safely
		//
		// Constructs a default node in the parent wrapper if necessary,
		// or simply returns the existing content of the wrapper. This
		// helper is necessary for exposing a compatible interface with
		// boost::fusion adapters.
		//
		struct SafeContentAccess
		{
			//
			// Dereference operator
			//
			T& operator * () const
			{
				if(!Owner->Contents)
					Owner->Contents.reset(new (Allocate<T>()) T());

				return *(Owner->Contents);
			}

			//
			// Indirection operator
			//
			T* operator -> () const
			{
				if(!Owner->Contents)
					Owner->Contents.reset(new (Allocate<T>()) T());

				return Owner->Contents.get();
			}

			Deferred* Owner;
		} Content;

	protected:
		mutable PointerType Contents;
	};

	//
	// Deferred construction wrapper for containers
	//
	// This is primarily intended to support deferred construction
	// of identifier lists (see Identifiers.h) which, being highly
	// common, benefit from postponing construction for as long as
	// possible. Measurements indicate that this is a net win.
	//
	template <typename T, typename PointerType = boost::shared_ptr<T> >
	struct DeferredContainer
	{
		// Type shortcuts for treating this as if it were itself a container
		typedef typename T::ContainedT::value_type value_type;
		typedef typename T::ContainedT::iterator iterator;
		typedef typename T::ContainedT::const_iterator const_iterator;

		//
		// Default construction
		//
		DeferredContainer()
			: Contents(reinterpret_cast<T*>(NULL))
		{
			Content.Owner = this;
		}

		//
		// Construction from the wrapped type
		//
		DeferredContainer(const T& expr)
			: Contents(new (Allocate<T>()) T(expr))
		{
			Content.Owner = this;
		}

		//
		// Copy construction
		//
		DeferredContainer(const DeferredContainer& rhs)
			: Contents(rhs.Contents)
		{
			Content.Owner = this;
		}

		//
		// Construction from a pair of some indeterminate values
		//
		// This is predominantly used when an iterator range is passed
		// to the constructor; to make the wrapper look as if it holds
		// an underlying iterator pair, we use this constructor.
		//
		template <typename ContentOfSomeKind>
		DeferredContainer(const ContentOfSomeKind& a, const ContentOfSomeKind& b)
			: Contents(new (Allocate<T>()) T())
		{
			Content.Owner = this;
			Contents->Container.push_back(value_type(a, b));
		}

		//
		// Assignment operator
		//
		DeferredContainer& operator = (const DeferredContainer& rhs)
		{
			if(this != &rhs)
				Contents = rhs.Contents;
			return *this;
		}

		//
		// Support insertion as if this were itself a container
		//
		void insert(const iterator& pos, const value_type& value)
		{
			Content->Container.insert(pos, value);
		}

		//
		// Retrieve a starting iterator as if this were itself a container
		//
		iterator begin()
		{
			return Content->Container.begin();
		}

		//
		// Retrieve an ending iterator as if this were itself a container
		//
		iterator end()
		{
			return Content->Container.end();
		}

		//
		// Check if the wrapped container is empty
		//
		bool empty() const
		{
			return Content->Container.empty();
		}

		//
		// Wrapper for accessing the content safely
		//
		// Constructs a default node in the parent wrapper if necessary,
		// or simply returns the existing content of the wrapper. This
		// helper is necessary for exposing a compatible interface with
		// boost::fusion adapters.
		//
		struct SafeContentAccess
		{
			//
			// Dereference operator
			//
			T& operator * () const
			{
				if(!Owner->Contents)
					Owner->Contents.reset(new (Allocate<T>()) T());

				return *(Owner->Contents);
			}

			//
			// Indirection operator
			//
			T* operator -> () const
			{
				if(!Owner->Contents)
					Owner->Contents.reset(new (Allocate<T>()) T());

				return Owner->Contents.get();
			}

			// Linkage to the owning wrapper
			DeferredContainer* Owner;
		} Content;

	// Internal state
	protected:
		mutable PointerType Contents;
	};


}

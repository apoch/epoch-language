#pragma once

namespace AST
{

	template <typename T, typename PointerType = boost::shared_ptr<T> >
	struct Deferred
	{
		Deferred()
			: Contents(reinterpret_cast<T*>(NULL))
		{
			Content.Owner = this;
		}

		Deferred(const T& expr)
			: Contents(new (Allocate<T>()) T(expr))
		{
			Content.Owner = this;
		}

		Deferred(const Deferred& rhs)
			: Contents(rhs.Contents)
		{
			Content.Owner = this;
		}

		template <typename VariantContentT>
		Deferred(const VariantContentT& content)
			: Contents(new (Allocate<T>()) T(content))
		{
			Content.Owner = this;
		}

		operator T ()
		{
			if(Contents)
				return *Contents;

			return T();
		}

		operator T () const
		{
			if(Contents)
				return *Contents;

			return T();
		}

		Deferred& operator = (const Deferred& rhs)
		{
			if(this != &rhs)
				Contents = rhs.Contents;
			return *this;
		}

		struct SafeContentAccess
		{
			T& operator * () const
			{
				if(!Owner->Contents)
					Owner->Contents.reset(new (Allocate<T>()) T());

				return *(Owner->Contents);
			}

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

	template <typename T, typename PointerType = boost::shared_ptr<T> >
	struct DeferredContainer
	{
		typedef typename T::ContainedT::value_type value_type;
		typedef typename T::ContainedT::iterator iterator;

		DeferredContainer()
			: Contents(reinterpret_cast<T*>(NULL))
		{
			Content.Owner = this;
		}

		DeferredContainer(const T& expr)
			: Contents(new (Allocate<T>()) T(expr))
		{
			Content.Owner = this;
		}

		DeferredContainer(const DeferredContainer& rhs)
			: Contents(rhs.Contents)
		{
			Content.Owner = this;
		}

		template <typename ContentOfSomeKind>
		DeferredContainer(const ContentOfSomeKind& a, const ContentOfSomeKind& b)
			: Contents(new (Allocate<T>()) T())
		{
			Content.Owner = this;
			Contents->Container.push_back(value_type(a, b));
		}

		template <typename VariantContentT>
		DeferredContainer(const VariantContentT& content)
			: Contents(new (Allocate<T>()) T(content))
		{
			Content.Owner = this;
		}

		operator T ()
		{
			if(Contents)
				return *Contents;

			return T();
		}

		operator T () const
		{
			if(Contents)
				return *Contents;

			return T();
		}

		DeferredContainer& operator = (const DeferredContainer& rhs)
		{
			if(this != &rhs)
				Contents = rhs.Contents;
			return *this;
		}

		void insert(const iterator& pos, const value_type& value)
		{
			Content->Container.insert(pos, value);
		}

		iterator begin()
		{
			return Content->Container.begin();
		}

		iterator end()
		{
			return Content->Container.end();
		}

		bool empty() const
		{
			return Content->Container.empty();
		}

		struct SafeContentAccess
		{
			T& operator * () const
			{
				if(!Owner->Contents)
					Owner->Contents.reset(new (Allocate<T>()) T());

				return *(Owner->Contents);
			}

			T* operator -> () const
			{
				if(!Owner->Contents)
					Owner->Contents.reset(new (Allocate<T>()) T());

				return Owner->Contents.get();
			}

			DeferredContainer* Owner;
		} Content;

	protected:
		mutable PointerType Contents;
	};


}

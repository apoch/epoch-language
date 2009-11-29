#pragma once


// Forward declarations
namespace Validator { class ValidationTraverser; }
namespace Serialization { class SerializationTraverser; }


namespace VM
{

	class SelfAwareBase
	{
	public:
		virtual ~SelfAwareBase()
		{ }

		virtual void Traverse(Validator::ValidationTraverser& traverser) = 0;
		virtual void Traverse(Serialization::SerializationTraverser& traverser) = 0;

		virtual const std::wstring& GetToken() const = 0;
	};

	template <class SelfType>
	class SelfAware : public SelfAwareBase
	{
	public:
		virtual void Traverse(Validator::ValidationTraverser& traverser);
		virtual void Traverse(Serialization::SerializationTraverser& traverser);

		virtual const std::wstring& GetToken() const;
	};

}


//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper classes and type definitions for working with r-values.
//

#pragma once

// Dependencies
#include "Utility/Types/EpochTypeIDs.h"
#include "Virtual Machine/Core Entities/Types/Tuple.h"
#include "Virtual Machine/Core Entities/Types/Structure.h"
#include "Virtual Machine/Types Management/Typecasts.h"
#include "Virtual Machine/VMExceptions.h"


namespace VM
{
	// Forward declarations
	class ScopeDescription;
	class ActivatedScope;
	class FunctionBase;

	//
	// Base class for all r-value types
	//
	class RValue
	{
	// Construction and destruction
	public:
		explicit RValue(EpochVariableTypeID type)
			: Type(type)
		{ }

		virtual ~RValue()
		{ }

	// Type interface
	public:
		EpochVariableTypeID GetType() const
		{ return Type; }

	// Copy interface
	public:
		virtual RValue* Clone() const = 0;

	// Comparison interface
	public:
		bool operator == (const RValue& rhs) const
		{
			if(Type == EpochVariableType_Null || rhs.Type == EpochVariableType_Null)
			{
				// Null is always equal to another null, but never equal to anything else
				return (Type == rhs.Type);
			}

			if(Type != rhs.Type)
				throw ExecutionException("Cannot compare variables of differing types; conversion required");

			return VirtualComparator(rhs);
		}

		bool operator != (const RValue& rhs) const
		{
			return !(rhs == *this);
		}

	// Comparison helper
	public:
		virtual bool VirtualComparator(const RValue& rhs) const = 0;

	// Casting interface
	public:
		template <class RValueClass>
		const RValueClass& CastTo() const
		{ return TypesManager::CastRValue<RValueClass>(*this); }

		template <class RValueClass>
		RValueClass& CastTo()
		{ return TypesManager::CastRValue<RValueClass>(*this); }

	// Internal tracking
	protected:
		EpochVariableTypeID Type;
	};


	// Handy type shortcut
	typedef std::auto_ptr<RValue> RValuePtr;


	//
	// Specific derived class for r-values of a particular type
	//
	template<typename BaseStorage, EpochVariableTypeID EpochTypeID>
	class TypedRValue : public RValue
	{
	// Handy type shortcut
	public:
		typedef BaseStorage BaseStorageType;

	// Construction
	public:
		TypedRValue()
			: RValue(EpochTypeID)
		{ }

	// R-Value interface
	public:
		virtual BaseStorage GetValue() const = 0;

		static EpochVariableTypeID GetType()
		{ return EpochTypeID; }
	};

	//
	// Specific derived class for r-value holders bound to a specific value
	//
	template<typename BaseStorage, EpochVariableTypeID EpochTypeID>
	class BoundRValue : public TypedRValue<BaseStorage, EpochTypeID>
	{
	// Construction
	public:
		BoundRValue()
			: Value()
		{ }

		explicit BoundRValue(const BaseStorage& value)
			: Value(value)
		{ }

	// Typed r-value interface
	public:
		BaseStorage GetValue() const
		{ return Value; }

	// R-value interface
	public:
		static EpochVariableTypeID GetType()
		{ return EpochTypeID; }

	// Copy interface
	public:
		virtual RValue* Clone() const
		{
			BoundRValue<BaseStorage, EpochTypeID>* copy = new BoundRValue<BaseStorage, EpochTypeID>(*this);
			return copy;
		}

	// Helper for comparison interface
	public:
		virtual bool VirtualComparator(const RValue& rhs) const
		{
			typedef BoundRValue<BaseStorage, EpochTypeID> thistype;
			const thistype& rhsref = dynamic_cast<const thistype&>(rhs);
			return Value == rhsref.Value;
		}

	// Internal storage
	protected:
		BaseStorage Value;
	};

	//
	// Special derived r-value class for working with tuples
	//
	class TupleRValue : public RValue
	{
	// Construction and destruction
	public:
		TupleRValue(const TupleType& tupletype, const ActivatedScope& scope, IDType tupletypeid);
		TupleRValue(const TupleType& tupletype, IDType tupletypeid);
		
		TupleRValue(const TupleRValue& rhs)
			: RValue(EpochVariableType_Tuple)
		{
			CopyFrom(rhs);
		}

		~TupleRValue();

	// Interface for working with the tuple's contents
	public:
		void AddMember(const std::wstring& membername, RValuePtr rvaltr);
		
		IDType GetTupleTypeID() const
		{ return TupleTypeID; }

	// Typed r-value interface
	public:
		RValuePtr GetValue(const std::wstring& membername) const;

	// R-value interface
	public:
		static EpochVariableTypeID GetType()
		{ return EpochVariableType_Tuple; }

	// Copy interface
	public:
		virtual RValue* Clone() const
		{
			TupleRValue *copy = new TupleRValue(*this);
			return copy;
		}

		TupleRValue& operator = (const TupleRValue& rhs)
		{
			if(this != &rhs) { Clean(); CopyFrom(rhs); }
			return *this;
		}

	// Internal helpers for copying
	protected:
		void Clean();
		void CopyFrom(const TupleRValue& rhs);

	// Helper for comparison interface
	public:
		virtual bool VirtualComparator(const RValue& rhs) const;

	// Internal tracking
	protected:
		std::map<std::wstring, RValue*> MemberData;
		IDType TupleTypeID;
	};

	//
	// Special derived r-value class for working with structures
	//
	class StructureRValue : public RValue
	{
	// Construction and destruction
	public:
		StructureRValue(const StructureType& structuretype, IDType structuretypeid);
		StructureRValue(const StructureRValue& rval);

		~StructureRValue();

	// Interface for working with the structure's contents
	public:
		void AddMember(const std::wstring& membername, RValuePtr rvalptr);
		
		IDType GetStructureTypeID() const
		{ return StructureTypeID; }

	// Typed r-value interface
	public:
		RValuePtr GetValue(const std::wstring& membername) const;

	// R-value interface
	public:
		static EpochVariableTypeID GetType()
		{ return EpochVariableType_Structure; }

	// Copy interface
	public:
		virtual RValue* Clone() const
		{
			StructureRValue *copy = new StructureRValue(*this);
			return copy;
		}

		StructureRValue& operator = (const StructureRValue& rhs)
		{
			if(this != &rhs) { Clean(); CopyFrom(rhs); }
			return *this;
		}

	protected:
		void Clean();
		void CopyFrom(const StructureRValue& rhs);

	// Helper for comparison interface
	public:
		virtual bool VirtualComparator(const RValue& rhs) const;

	// Internal tracking
	protected:
		std::map<std::wstring, RValue*> MemberData;
		IDType StructureTypeID;
	};


	//
	// Special derived type for array rvalues
	//
	class ArrayRValue : public RValue
	{
	// Construction and destruction
	public:
		explicit ArrayRValue(EpochVariableTypeID elementtype)
			: RValue(EpochVariableType_Array),
			  ElementType(elementtype)
		{ }

		ArrayRValue(const ArrayRValue& rhs)
			: RValue(EpochVariableType_Array),
			  ElementType(rhs.ElementType)
		{ CopyFrom(rhs); }

		~ArrayRValue();

	// Access to array elements
	public:
		void AddElement(RValue* element);

		size_t GetElementCount() const
		{ return Elements.size(); }

		EpochVariableTypeID GetElementType() const
		{ return ElementType; }

		const std::vector<RValue*>& GetElements() const
		{ return Elements; }

	// R-value interface
	public:
		static EpochVariableTypeID GetType()
		{ return EpochVariableType_Array; }

	// Copy interface
	public:
		virtual RValue* Clone() const
		{
			ArrayRValue *copy = new ArrayRValue(*this);
			return copy;
		}

		ArrayRValue& operator = (const ArrayRValue& rhs)
		{
			if(this != &rhs) { Clean(); CopyFrom(rhs); }
			return *this;
		}

	protected:
		void Clean();
		void CopyFrom(const ArrayRValue& rhs);

	// Helper for comparison interface
	public:
		virtual bool VirtualComparator(const RValue& rhs) const;

	// Internal tracking
	protected:
		std::vector<RValue*> Elements;
		EpochVariableTypeID ElementType;
	};


	//
	// Special derived type for buffer rvalues
	//
	class BufferRValue : public RValue
	{
	// Construction and destruction
	public:
		explicit BufferRValue(HandleType bufferhandle)
			: RValue(EpochVariableType_Buffer),
			  BufferHandle(bufferhandle)
		{ }

		BufferRValue(const ArrayRValue& rhs)
			: RValue(EpochVariableType_Buffer)
		{ CopyFrom(rhs); }

		~BufferRValue();

	// R-value interface
	public:
		static EpochVariableTypeID GetType()
		{ return EpochVariableType_Buffer; }

	// Copy interface
	public:
		virtual RValue* Clone() const
		{
			BufferRValue *copy = new BufferRValue(*this);
			return copy;
		}

		BufferRValue& operator = (const BufferRValue& rhs)
		{
			if(this != &rhs) { Clean(); CopyFrom(rhs); }
			return *this;
		}

	protected:
		void Clean();
		void CopyFrom(const BufferRValue& rhs);

	// Handle retrieval interface
	public:
		HandleType GetOriginHandle() const
		{ return BufferHandle; }

	// Helper for comparison interface
	public:
		virtual bool VirtualComparator(const RValue& rhs) const;

	// Internal tracking
	protected:
		HandleType BufferHandle;
	};

	
	// Handy type shortcuts
	typedef BoundRValue<Integer32, EpochVariableType_Null> NullRValue;
	typedef BoundRValue<Integer32, EpochVariableType_Integer> IntegerRValue;
	typedef BoundRValue<Integer16, EpochVariableType_Integer16> Integer16RValue;
	typedef BoundRValue<Real, EpochVariableType_Real> RealRValue;
	typedef BoundRValue<bool, EpochVariableType_Boolean> BooleanRValue;
	typedef BoundRValue<std::wstring, EpochVariableType_String> StringRValue;

	typedef BoundRValue<FunctionBase*, EpochVariableType_Function> FunctionRValue;
	typedef BoundRValue<void*, EpochVariableType_Address> AddressRValue;
	typedef BoundRValue<TaskHandle, EpochVariableType_TaskHandle> TaskHandleRValue;
}


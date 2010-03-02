//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Declarations of runtime variable typecast operations for the VM.
//

#pragma once

#include "Virtual Machine/Core Entities/RValue.h"
#include "Virtual Machine/Core Entities/Operation.h"
#include "Virtual Machine/Core Entities/Variables/BufferVariable.h"

#include "Utility/Strings.h"


namespace VM
{

	namespace Operations
	{

		template<typename OriginVarTypeInfo, typename DestinationVarTypeInfo>
		class TypeCast : public Operation, public SelfAware<TypeCast<OriginVarTypeInfo, DestinationVarTypeInfo> >
		{
		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context)
			{
				OriginVarTypeInfo::VariableType var(context.Stack.GetCurrentTopOfStack());
				context.Stack.Pop(var.GetStorageSize());
			}

			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context)
			{
				OriginVarTypeInfo::VariableType var(context.Stack.GetCurrentTopOfStack());
				DestinationVarTypeInfo::VariableType::BaseStorage retval;

				std::wstringstream convert;
				convert << var.GetValue();
				if(!(convert >> retval))
					throw ExecutionException("Failed to cast value; possible causes are overflow or malformed data");

				context.Stack.Pop(var.GetStorageSize());
				DestinationVarTypeInfo::VariableType returnvariable(&retval);
				return returnvariable.GetAsRValue();
			}

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return GetDestinationType(); }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 1; }

		// Additional queries
		public:
			EpochVariableTypeID GetOriginalType() const
			{ return OriginVarTypeInfo::VariableType::GetStaticType(); }

			// This may look redundant (given the presence of GetType()) but
			// in fact it is very handy to be able to look up the operation's
			// type without a valid scope description object, e.g. when the
			// serializer is running.
			EpochVariableTypeID GetDestinationType() const
			{ return DestinationVarTypeInfo::VariableType::GetStaticType(); }
		};


		template<typename OriginVarTypeInfo>
		class TypeCastToString : public Operation, public SelfAware<TypeCastToString<OriginVarTypeInfo> >
		{
		// Operation interface
		public:
			virtual void ExecuteFast(ExecutionContext& context)
			{
				OriginVarTypeInfo::VariableType var(context.Stack.GetCurrentTopOfStack());
				context.Stack.Pop(var.GetStorageSize());
			}

			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context)
			{
				OriginVarTypeInfo::VariableType var(context.Stack.GetCurrentTopOfStack());
				std::wstring retval;

				std::wstringstream convert;
				convert << var.GetValue();
				if(!(convert >> retval))
					throw ExecutionException("Failed to cast value; possible causes are overflow or malformed data");

				context.Stack.Pop(var.GetStorageSize());
				return RValuePtr(new StringRValue(retval));
			}

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return VM::EpochVariableType_String; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 1; }

		// Additional queries
		public:
			EpochVariableTypeID GetOriginalType() const
			{ return OriginVarTypeInfo::VariableType::GetStaticType(); }

			EpochVariableTypeID GetDestinationType() const
			{ return VM::EpochVariableType_String; }
		};

		class TypeCastBooleanToString : public Operation, public SelfAware<TypeCastBooleanToString>
		{
			virtual void ExecuteFast(ExecutionContext& context)
			{
				BooleanVariable var(context.Stack.GetCurrentTopOfStack());
				context.Stack.Pop(var.GetStorageSize());
			}

			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context)
			{
				BooleanVariable var(context.Stack.GetCurrentTopOfStack());
				std::wstring retval = (var.GetValue() ? Keywords::True : Keywords::False);
				context.Stack.Pop(var.GetStorageSize());
				return RValuePtr(new StringRValue(retval));
			}

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return VM::EpochVariableType_String; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 1; }


		// Additional queries
		public:
			EpochVariableTypeID GetOriginalType() const
			{ return VM::EpochVariableType_Boolean; }

			EpochVariableTypeID GetDestinationType() const
			{ return VM::EpochVariableType_String; }
		};

		class TypeCastBufferToString : public Operation, public SelfAware<TypeCastBufferToString>
		{
			virtual void ExecuteFast(ExecutionContext& context)
			{
				BufferVariable var(context.Stack.GetCurrentTopOfStack());
				context.Stack.Pop(var.GetStorageSize());
			}

			virtual RValuePtr ExecuteAndStoreRValue(ExecutionContext& context)
			{
				BufferVariable var(context.Stack.GetCurrentTopOfStack());
				std::wstring retval = reinterpret_cast<const wchar_t*>(var.GetValue());
				context.Stack.Pop(var.GetStorageSize());
				return RValuePtr(new StringRValue(retval));
			}

			virtual EpochVariableTypeID GetType(const ScopeDescription& scope) const
			{ return VM::EpochVariableType_String; }

			virtual size_t GetNumParameters(const VM::ScopeDescription& scope) const
			{ return 1; }

		// Additional queries
		public:
			EpochVariableTypeID GetOriginalType() const
			{ return VM::EpochVariableType_Buffer; }

			EpochVariableTypeID GetDestinationType() const
			{ return VM::EpochVariableType_String; }
		};

	}

}


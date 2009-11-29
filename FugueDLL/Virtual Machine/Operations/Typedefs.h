#pragma once


// Dependencies
#include "Virtual Machine/Types Management/RuntimeCasts.h"
#include "Virtual Machine/Types Management/TypeInfo.h"


namespace VM
{

	namespace Operations
	{

		typedef VM::Operations::TypeCast<TypeInfo::StringT, TypeInfo::IntegerT> TypeCastStringToInteger;
		typedef VM::Operations::TypeCast<TypeInfo::RealT, TypeInfo::IntegerT> TypeCastRealToInteger;
		typedef VM::Operations::TypeCast<TypeInfo::Integer16T, TypeInfo::IntegerT> TypeCastInteger16ToInteger;
		typedef VM::Operations::TypeCast<TypeInfo::BooleanT, TypeInfo::IntegerT> TypeCastBooleanToInteger;

		typedef VM::Operations::TypeCast<TypeInfo::StringT, TypeInfo::Integer16T> TypeCastStringToInteger16;
		typedef VM::Operations::TypeCast<TypeInfo::RealT, TypeInfo::Integer16T> TypeCastRealToInteger16;
		typedef VM::Operations::TypeCast<TypeInfo::IntegerT, TypeInfo::Integer16T> TypeCastIntegerToInteger16;
		typedef VM::Operations::TypeCast<TypeInfo::BooleanT, TypeInfo::Integer16T> TypeCastBooleanToInteger16;

		typedef VM::Operations::TypeCast<TypeInfo::StringT, TypeInfo::RealT> TypeCastStringToReal;
		typedef VM::Operations::TypeCast<TypeInfo::Integer16T, TypeInfo::RealT> TypeCastInteger16ToReal;
		typedef VM::Operations::TypeCast<TypeInfo::IntegerT, TypeInfo::RealT> TypeCastIntegerToReal;
		typedef VM::Operations::TypeCast<TypeInfo::BooleanT, TypeInfo::RealT> TypeCastBooleanToReal;

		typedef VM::Operations::TypeCastToString<TypeInfo::RealT> TypeCastRealToString;
		typedef VM::Operations::TypeCastToString<TypeInfo::Integer16T> TypeCastInteger16ToString;
		typedef VM::Operations::TypeCastToString<TypeInfo::IntegerT> TypeCastIntegerToString;

	}

}
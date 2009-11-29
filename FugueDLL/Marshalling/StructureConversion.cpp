//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Routines for converting Epoch structures into C-compatible structures
//
// WARNING - all marshalling code is platform-specific!
//

#include "pch.h"

#include "Marshalling/StructureConversion.h"
#include "Marshalling/Callback.h"

#include "Virtual Machine/Core Entities/Variables/StructureVariable.h"
#include "Virtual Machine/Types Management/Typecasts.h"
#include "Virtual Machine/Core Entities/Function.h"
#include "Virtual Machine/Types Management/TypeInfo.h"
#include "Virtual Machine/VMExceptions.h"

using namespace VM;


namespace 
{

	//
	// Helper for writing Epoch structure members that have been updated during a DLL call.
	// This is mainly used to simplify the code needed for handling nested structures.
	//
	void CopyStructMember(StructureVariable& var, const std::wstring& membername, EpochVariableTypeID type, void* target)
	{
		switch(type)
		{
		case EpochVariableType_Integer:
			var.WriteMember<TypeInfo::IntegerT>(membername, target, false);
			break;
		case EpochVariableType_Integer16:
			var.WriteMember<TypeInfo::Integer16T>(membername, target, false);
			break;
		case EpochVariableType_Real:
			var.WriteMember<TypeInfo::RealT>(membername, target, false);
			break;
		case EpochVariableType_Boolean:
			var.WriteMember<TypeInfo::BooleanT>(membername, target, false);
			break;
		case EpochVariableType_String:
			var.WriteMember(membername, RValuePtr(new StringRValue(*reinterpret_cast<const wchar_t**>(target))), false);
			break;
		case EpochVariableType_Structure:
			{
				const StructureType& t = StructureTrackerClass::GetOwnerOfStructureType(var.GetValue())->GetStructureType(var.GetValue());
				IDType structid = t.GetMemberTypeHint(membername);
				const StructureType& nestedt = StructureTrackerClass::GetOwnerOfStructureType(structid)->GetStructureType(structid);
				StructureVariable nestedvar(reinterpret_cast<Byte*>(var.GetStorage()) + t.GetMemberOffset(membername));
				const std::vector<std::wstring>& members = nestedt.GetMemberOrder();
				for(std::vector<std::wstring>::const_iterator iter = members.begin(); iter != members.end(); ++iter)
				{
					ptrdiff_t memberoffset = nestedt.GetMemberOffset(*iter) - nestedvar.GetBaseStorageSize();
					void* newtarget = reinterpret_cast<Byte*>(target) + memberoffset;
					CopyStructMember(nestedvar, *iter, nestedt.GetMemberType(*iter), newtarget);
				}
			}
			break;
		case EpochVariableType_Function:
			// Just a sanity check; we don't actually support having function addresses modified by the external code
			{
				Function* func = dynamic_cast<Function*>(var.ReadMember(membername)->CastTo<FunctionRValue>().GetValue());
				if(!func)
					throw ExecutionException("Function variable does not point to a user-defined function");

				void* oldvalue = Marshalling::RequestMarshalledCallback(func);
				void* newvalue = *reinterpret_cast<FunctionRValue::BaseStorageType*>(target);
				if(oldvalue != newvalue)
					throw NotImplementedException("External code has modified a function pointer; there is currently no established response to this behavior");
			}
			break;
		default:
			throw NotImplementedException("Cannot marshal structure member of this type from external DLL function to Epoch format");
		}
	}

}


//
// Convert structures back from C form to Epoch form
//
void Marshalling::CStructToEpoch(const std::vector<Byte*>& buffers, const std::vector<StructureVariable*> variables)
{
	if(buffers.size() != variables.size())
		throw InternalFailureException("Failed to marshal data back to Epoch format - mismatched buffer/variable records");

	for(UInteger32 i = 0; i < buffers.size(); ++i)
	{
		StructureVariable& var = *variables[i];
		const StructureType& t = StructureTrackerClass::GetOwnerOfStructureType(var.GetValue())->GetStructureType(var.GetValue());
		const std::vector<std::wstring>& members = t.GetMemberOrder();
		for(std::vector<std::wstring>::const_iterator iter = members.begin(); iter != members.end(); ++iter)
		{
			ptrdiff_t memberoffset = t.GetMemberOffset(*iter) - var.GetBaseStorageSize();
			void* target = &buffers[i][0] + memberoffset;
			CopyStructMember(var, *iter, t.GetMemberType(*iter), target);
		}
	}
}


//
// Convert an Epoch structure into C form
//
void* Marshalling::EpochToCStruct(const StructureVariable& structvar, ActivatedScope& params)
{
	size_t size = structvar.GetStorageSize() - structvar.GetBaseStorageSize();
	Byte* buffer = new Byte[size];

	try
	{
		const StructureType& structtype = StructureTrackerClass::GetOwnerOfStructureType(structvar.GetValue())->GetStructureType(structvar.GetValue());
		const std::vector<std::wstring>& structmembers = structtype.GetMemberOrder();

		for(std::vector<std::wstring>::const_iterator smiter = structmembers.begin(); smiter != structmembers.end(); ++smiter)
		{
			ptrdiff_t memberoffset = structtype.GetMemberOffset(*smiter) - structvar.GetBaseStorageSize();
			void* target = &buffer[0] + memberoffset;
			switch(structtype.GetMemberType(*smiter))
			{
			case EpochVariableType_Integer:
				*reinterpret_cast<IntegerRValue::BaseStorageType*>(target) = structvar.ReadMember(*smiter)->CastTo<IntegerRValue>().GetValue();
				break;
			case EpochVariableType_Integer16:
				*reinterpret_cast<Integer16RValue::BaseStorageType*>(target) = structvar.ReadMember(*smiter)->CastTo<Integer16RValue>().GetValue();
				break;
			case EpochVariableType_Real:
				*reinterpret_cast<RealRValue::BaseStorageType*>(target) = structvar.ReadMember(*smiter)->CastTo<RealRValue>().GetValue();
				break;
			case EpochVariableType_Boolean:
				*reinterpret_cast<BooleanRValue::BaseStorageType*>(target) = structvar.ReadMember(*smiter)->CastTo<BooleanRValue>().GetValue();
				break;
			case EpochVariableType_String:
				*reinterpret_cast<const wchar_t**>(target) = StringVariable::GetByHandle(*reinterpret_cast<StringHandle*>(reinterpret_cast<Byte*>(structvar.GetStorage()) + memberoffset + structvar.GetBaseStorageSize())).c_str();
				break;
			case EpochVariableType_Structure:
				{
					Byte* sourceptr = reinterpret_cast<Byte*>(structvar.GetStorage()) + structtype.GetMemberOffset(*smiter);
					IDType id = structvar.ReadMember(*smiter)->CastTo<StructureRValue>().GetStructureTypeID();
					size_t sizetocopy = StructureTrackerClass::GetOwnerOfStructureType(id)->GetStructureType(id).GetMemberStorageSize();
					StructureVariable var(sourceptr);
					Byte* tempbuffer = reinterpret_cast<Byte*>(EpochToCStruct(var, params));
					memcpy(target, tempbuffer, sizetocopy);
					delete [] tempbuffer;
				}
				break;
			case EpochVariableType_Function:
				{
					Function* func = dynamic_cast<Function*>(structvar.ReadMember(*smiter)->CastTo<FunctionRValue>().GetValue());
					if(!func)
						throw ExecutionException("Only user defined functions can be passed to external DLL functions");

					*reinterpret_cast<FunctionRValue::BaseStorageType*>(target) = reinterpret_cast<FunctionRValue::BaseStorageType>(RequestMarshalledCallback(func));
				}
				break;
			default:
				throw NotImplementedException("Cannot pass structure member of this type to external DLL function");
			}
		}
	}
	catch(...)
	{
		delete [] buffer;
		throw;
	}

	return buffer;
}

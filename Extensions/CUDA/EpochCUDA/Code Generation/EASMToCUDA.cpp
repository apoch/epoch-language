//
// The Epoch Language Project
// CUDA Interoperability Library
//
// EpochASM-to-CUDA compiler logic
//

#include "pch.h"

#include "Code Generation/EASMToCUDA.h"
#include "CUDA Wrapper/Naming.h"
#include "Utility/Strings.h"
#include "Serialization/SerializationTokens.h"

#include <algorithm>


using namespace Compiler;
using namespace Extensions;


//
// Construct and initialize a wrapper for a compile session
//
CompilationSession::CompilationSession(TemporaryFileWriter& codefile, std::list<Traverser::ScopeContents>& registeredvariables, Extensions::OriginalCodeHandle originalcode)
	: OriginalCode(originalcode),
	  TabDepth(0),
	  RegisteredVariables(registeredvariables),
	  TemporaryCodeFile(codefile)
{
}


//
// Callback: register the contents of a lexical scope with the compiler
//
void CompilationSession::RegisterScope(bool toplevel, size_t numcontents, const Traverser::ScopeContents* contents)
{
	WriteScopeContents(toplevel, numcontents, contents);
}

void CompilationSession::WriteScopeContents(bool toplevel, size_t numcontents, const Traverser::ScopeContents* contents)
{
	if(!numcontents)
		return;

	PadTabs();
	TemporaryCodeFile.OutputStream << L"// Define variables in the current scope\n";

	for(size_t i = 0; i < numcontents; ++i)
	{
		bool appendsemicolon = true;

		if(toplevel)
			RegisteredVariables.push_back(contents[i]);

		PadTabs();
		switch(contents[i].Type)
		{
		case VM::EpochVariableType_Integer:
			TemporaryCodeFile.OutputStream << L"int " << contents[i].Identifier;
			if(toplevel)
				TemporaryCodeFile.OutputStream << L" = __marshal_input_ints[__marshal_int_index++]";
			break;

		case VM::EpochVariableType_Real:
			TemporaryCodeFile.OutputStream << L"float " << contents[i].Identifier;
			if(toplevel)
				TemporaryCodeFile.OutputStream << L" = __marshal_input_floats[__marshal_float_index++]";
			break;

		case VM::EpochVariableType_Array:
			{
				VM::EpochVariableTypeID arraytype = contents[i].ContainedType;
				switch(arraytype)
				{
				case VM::EpochVariableType_Integer:
					TemporaryCodeFile.OutputStream << L"int ";
					break;

				case VM::EpochVariableType_Real:
					TemporaryCodeFile.OutputStream << L"float ";
					break;

				default:
					throw std::exception("Cannot pass arrays of this type");
				}

				TemporaryCodeFile.OutputStream << contents[i].Identifier << L"[" << contents[i].ContainedSize << L"];\n";
				ArraySizeCache[contents[i].Identifier] = contents[i].ContainedSize;

				PadTabs();
				TemporaryCodeFile.OutputStream << "for(unsigned __marshal_array_counter = 0; __marshal_array_counter < " << contents[i].ContainedSize << "; ++__marshal_array_counter)\n";
				++TabDepth;
				PadTabs();
				TemporaryCodeFile.OutputStream << contents[i].Identifier << L"[__marshal_array_counter] = ";
				
				switch(arraytype)
				{
				case VM::EpochVariableType_Integer:
					TemporaryCodeFile.OutputStream << L"__marshal_input_int_arrays[__marshal_int_array_index++];\n";
					break;

				case VM::EpochVariableType_Real:
					TemporaryCodeFile.OutputStream << L"__marshal_input_float_arrays[__marshal_float_array_index++];\n";
					break;

				default:
					throw std::exception("Cannot pass arrays of this type");
				}				
				
				--TabDepth;
				appendsemicolon = false;
			}
			break;

		default:
			throw std::exception("Scope contains a variable of an unrecognized type; cannot generate declaration/initialization code");
		}

		if(appendsemicolon)
			TemporaryCodeFile.OutputStream << L";\n";
	}

	TemporaryCodeFile.OutputStream << L"\n";
}



//
// Callback: register a leaf (AKA an EpochASM instruction) with the compiler
//
void CompilationSession::RegisterLeaf(const wchar_t* token, const Traverser::Payload* payload)
{
	LeafStack.top().push_back(Leaf(token, payload));
}


//
// Callback: enter a lexical scope/nested block
//
void CompilationSession::EnterNode()
{
	if(!LeafStack.empty())
	{
		WriteLeaves(LeafStack.top());
		LeafStack.top().clear();
	}

	LeafStack.push(LeafList());

	PadTabs();
	TemporaryCodeFile.OutputStream << L"{\n";
	++TabDepth;
}

//
// Callback: exit a lexical scope/nested block
//
void CompilationSession::ExitNode()
{
	WriteLeaves(LeafStack.top());

	--TabDepth;
	PadTabs();
	TemporaryCodeFile.OutputStream << L"}\n";

	LeafStack.pop();
}


//
// Helper routine for converting traversal payload data to code
//
// Note that we do not use an overloaded << operator for outputting payload structs
// to std streams; this is because we use the EpochVariableType_Error type as a
// sentinel to indicate that the payload should output the contents of a temporary
// stream buffer used to assemble infix expressions. Since this is fairly special
// behaviour and only makes sense within the context of our compiler, it is cleaner
// to use this output wrapper than try to generalize << operator support accordingly.
//
void CompilationSession::OutputPayload(const Traverser::Payload& payload, std::wostream& stream)
{
	switch(payload.Type)
	{
	case VM::EpochVariableType_Error:			stream << ExpressionConstruction.str(); ExpressionConstruction.str(L"");	break;
	case VM::EpochVariableType_Integer:			stream << payload.Int32Value;												break;
	case VM::EpochVariableType_Real:			stream << payload.FloatValue;												break;
	case VM::EpochVariableType_String:
		if(payload.IsIdentifier)
			stream << payload.StringValue;
		else
			stream << "\"" << payload.StringValue << "\"";
		break;
	default:
		throw std::exception("Cannot emit contents of Payload structure - type is not supported or not recognized");
	}
}

//
// Helper function for making code look all pretty and tabified
//
void CompilationSession::PadTabs()
{
	for(unsigned i = 0; i < TabDepth; ++i)
		TemporaryCodeFile.OutputStream << L"\t";
}

//
// Helper function for writing the beginning of a function definition
//
// This code includes preparatory steps for marshalling Epoch data into CUDA variables
//
void CompilationSession::FunctionPreamble(Extensions::OriginalCodeHandle handle)
{
	PadTabs();
	TemporaryCodeFile.OutputStream << L"extern \"C\" __global__ void " << widen(GenerateFunctionName(handle)) << L"(float* __marshal_input_floats, int* __marshal_input_ints, float* __marshal_input_float_arrays, int* __marshal_input_int_arrays)\n";
	PadTabs();
	TemporaryCodeFile.OutputStream << L"{\n";
	++TabDepth;
	PadTabs();
	TemporaryCodeFile.OutputStream << L"// Copy variable values from the host into local variables\n";
	PadTabs();
	TemporaryCodeFile.OutputStream << L"unsigned int __marshal_float_index = 0;\n";
	PadTabs();
	TemporaryCodeFile.OutputStream << L"unsigned int __marshal_int_index = 0;\n";
	PadTabs();
	TemporaryCodeFile.OutputStream << L"unsigned int __marshal_float_array_index = 0;\n";
	PadTabs();
	TemporaryCodeFile.OutputStream << L"unsigned int __marshal_int_array_index = 0;\n";
}

//
// Helper function for generating CUDA-to-Epoch variable marshalling code
//
// Marshalling code is responsible for ensuring that the CUDA environment
// returns data to the Epoch environment seamlessly, allowing users to
// modify variables in CUDA code and see the results reflected in the calling
// Epoch code.
//
void CompilationSession::MarshalOut()
{
	TemporaryCodeFile.OutputStream << L"\n";
	PadTabs();
	TemporaryCodeFile.OutputStream << L"// Copy final variable values back into the data buffer for retrieval by the host\n";
	PadTabs();
	TemporaryCodeFile.OutputStream << L"__marshal_float_index = 0;\n";
	PadTabs();
	TemporaryCodeFile.OutputStream << L"__marshal_int_index = 0;\n";
	PadTabs();
	TemporaryCodeFile.OutputStream << L"__marshal_float_array_index = 0;\n";
	PadTabs();
	TemporaryCodeFile.OutputStream << L"__marshal_int_array_index = 0;\n";

	for(std::list<Traverser::ScopeContents>::const_iterator iter = RegisteredVariables.begin(); iter != RegisteredVariables.end(); ++iter)
	{
		PadTabs();
		switch(iter->Type)
		{
		case VM::EpochVariableType_Integer:
			TemporaryCodeFile.OutputStream << L"__marshal_input_ints[__marshal_int_index++] = " << iter->Identifier << L";\n";
			break;

		case VM::EpochVariableType_Real:
			TemporaryCodeFile.OutputStream << L"__marshal_input_floats[__marshal_float_index++] = " << iter->Identifier << L";\n";
			break;

		case VM::EpochVariableType_Array:
			{
				switch(iter->ContainedType)
				{
				case VM::EpochVariableType_Integer:
					TemporaryCodeFile.OutputStream << "for(unsigned __marshal_array_counter = 0; __marshal_array_counter < " << GetArraySize(iter->Identifier) << "; ++__marshal_array_counter)\n";
					++TabDepth;
					PadTabs();
					TemporaryCodeFile.OutputStream << "__marshal_input_int_arrays[__marshal_int_array_index++] = " << iter->Identifier << "[__marshal_array_counter];\n";
					--TabDepth;
					break;

				case VM::EpochVariableType_Real:
					break;

				default:
					throw std::exception("Unsupported type - cannot generate code to transfer variable data from the CUDA device!");
				}
			}
			break;

		default:
			throw std::exception("Unsupported type - cannot generate code to transfer variable data from the CUDA device!");
		}
	}

	--TabDepth;
	PadTabs();
	TemporaryCodeFile.OutputStream << L"}\n\n";
}


void CompilationSession::WriteLeaves(LeafList& leaves)
{
	std::reverse(leaves.begin(), leaves.end());

	std::list<std::wstring> OutputLines;

	for(LeafList::const_iterator iter = leaves.begin(); iter != leaves.end(); ++iter)
	{
		if(iter->Token == Serialization::PushOperation)
			continue;

		OutputLines.push_front(GenerateLeafCode(leaves, iter));
	}

	for(std::list<std::wstring>::const_iterator iter = OutputLines.begin(); iter != OutputLines.end(); ++iter)
	{
		PadTabs();
		TemporaryCodeFile.OutputStream << (*iter) << L"\n";
	}
}


std::wstring CompilationSession::GenerateLeafCode(const LeafList& leaves, LeafList::const_iterator& iter)
{
	std::wostringstream out;

	if(iter->Token == Serialization::InitializeValue ||
	   iter->Token == Serialization::AssignValue)
	{
		OutputPayload(iter->Payload, out);
		out << L" = ";

		AdvanceLeafIterator(leaves, iter);
		out << GenerateLeafCode(leaves, iter);
		out << L";";
	
		return out.str();
	}
	else if(iter->Token == Serialization::PushIntegerLiteral ||
			iter->Token == Serialization::PushInteger16Literal ||
			iter->Token == Serialization::PushRealLiteral)
	{
		OutputPayload(iter->Payload, out);
		return out.str();
	}
	else if(iter->Token == Serialization::While)
	{
		return L"while(true)";
	}
	else if(iter->Token == Serialization::AddIntegers ||
			iter->Token == Serialization::AddInteger16s ||
			iter->Token == Serialization::AddReals)
	{
		AdvanceLeafIterator(leaves, iter);
		std::wstring operand = GenerateLeafCode(leaves, iter);
		AdvanceLeafIterator(leaves, iter);
		out << GenerateLeafCode(leaves, iter);
		out << L" + " << operand;
		return out.str();
	}
	else if(iter->Token == Serialization::GetValue)
	{
		return iter->Payload.StringValue;
	}
	else if(iter->Token == Serialization::WhileCondition)
	{
		AdvanceLeafIterator(leaves, iter);
		out << L"if(!(" << GenerateLeafCode(leaves, iter) << L")) break;";
		return out.str();
	}
	else if(iter->Token == Serialization::IsLesser)
	{
		AdvanceLeafIterator(leaves, iter);
		std::wstring operand = GenerateLeafCode(leaves, iter);
		AdvanceLeafIterator(leaves, iter);
		out << GenerateLeafCode(leaves, iter);
		out << L" < " << operand;
		return out.str();
	}
	else if(iter->Token == Serialization::ArrayLength)
	{
		out << GetArraySize(iter->Payload.StringValue);
		return out.str();
	}
	else if(iter->Token == Serialization::WriteArray)
	{
		std::wstring arrayidentifier = iter->Payload.StringValue;
		AdvanceLeafIterator(leaves, iter);
		std::wstring rhs = GenerateLeafCode(leaves, iter);
		AdvanceLeafIterator(leaves, iter);
		out << arrayidentifier << L"[";
		out << GenerateLeafCode(leaves, iter) << L"] = ";
		out << rhs << L";";
		return out.str();
	}
	else if(iter->Token == Serialization::ReadArray)
	{
		out << iter->Payload.StringValue << L"[";
		AdvanceLeafIterator(leaves, iter);
		out << GenerateLeafCode(leaves, iter) << L"]";
		return out.str();
	}
	else if(iter->Token == Serialization::MultiplyIntegers)
	{
		// TODO - factor this sludge out into a helper function
		AdvanceLeafIterator(leaves, iter);
		std::wstring operand = GenerateLeafCode(leaves, iter);
		AdvanceLeafIterator(leaves, iter);
		out << GenerateLeafCode(leaves, iter);
		out << L" * " << operand;
		return out.str();
	}
	
	throw std::exception("Cannot generate CUDA code for the given EASM instruction");
}


size_t CompilationSession::GetArraySize(const std::wstring& arrayname) const
{
	std::map<std::wstring, size_t>::const_iterator iter = ArraySizeCache.find(arrayname);
	if(iter != ArraySizeCache.end())
		return iter->second;

	throw std::exception("Cannot determine size of array variable - name not recognized");
}

void CompilationSession::AdvanceLeafIterator(const LeafList& leaves, LeafList::const_iterator& iter) const
{
	// TODO - since we don't use leaves anymore, remove the parameter
	do
	{
		++iter;
	} while(iter->Token == Serialization::PushOperation);
}



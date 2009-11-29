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
void CompilationSession::RegisterScope(size_t numcontents, const Traverser::ScopeContents* contents)
{
	for(size_t i = 0; i < numcontents; ++i)
	{
		RegisteredVariables.push_back(contents[i]);

		PadTabs();
		switch(contents[i].Type)
		{
		case VM::EpochVariableType_Integer:
			TemporaryCodeFile.OutputStream << L"int " << contents[i].Identifier << ";\n";
			break;

		case VM::EpochVariableType_Real:
			TemporaryCodeFile.OutputStream << L"float " << contents[i].Identifier << " = __marshal_input_floats[__marshal_float_index++];\n";
			break;

		case VM::EpochVariableType_String:
			TemporaryCodeFile.OutputStream << L"char* " << contents[i].Identifier << ";\n";
			break;

		default:
			throw std::exception("Scope contains a variable of an unrecognized type; cannot generate declaration/initialization code");
		}
	}
}


//
// Callback: register a leaf (AKA an EpochASM instruction) with the compiler
//
void CompilationSession::RegisterLeaf(const wchar_t* token, const Traverser::Payload* payload)
{
	if(token == Serialization::PushOperation)
	{
		// Do nothing yet; the nested operation will automatically be generated
	}
	else if(token == Serialization::PushIntegerLiteral
		 || token == Serialization::PushRealLiteral
		 || token == Serialization::GetValue)
	{
		Parameters.push(*payload);
	}
	else if(token == Serialization::AddReals
		 || token == Serialization::AddIntegers
		 || token == Serialization::AddInteger16s)
	{
		EmitInfixExpression(L"+");
	}
	else if(token == Serialization::SubtractReals
		 || token == Serialization::SubtractIntegers
		 || token == Serialization::SubtractInteger16s)
	{
		EmitInfixExpression(L"-");
	}
	else if(token == Serialization::MultiplyReals
		 || token == Serialization::MultiplyIntegers
		 || token == Serialization::MultiplyInteger16s)
	{
		EmitInfixExpression(L"*");
	}
	else if(token == Serialization::DivideReals
		 || token == Serialization::DivideIntegers
		 || token == Serialization::DivideInteger16s)
	{
		EmitInfixExpression(L"/");
	}
	else if(token == Serialization::AssignValue)
	{
		PadTabs();
		TemporaryCodeFile.OutputStream << payload->StringValue << L" = ";

		Traverser::Payload varpayload = Parameters.top();
		Parameters.pop();

		OutputPayload(varpayload, TemporaryCodeFile.OutputStream);
		TemporaryCodeFile.OutputStream << L";\n";
	}
	else
		throw std::exception("Unrecognized token passed to EpochASM-to-CUDA compiler; this may be an unsupported feature or a library version mismatch");
}


//
// Callback: enter a lexical scope/nested block
//
void CompilationSession::EnterNode()
{
	PadTabs();
	TemporaryCodeFile.OutputStream << L"{\n";
	++TabDepth;
}

//
// Callback: exit a lexical scope/nested block
//
void CompilationSession::ExitNode()
{
	--TabDepth;
	PadTabs();
	TemporaryCodeFile.OutputStream << L"}\n";
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
// Helper function for constructing and emitting infix operations
//
void CompilationSession::EmitInfixExpression(const std::wstring& operatorname)
{
	Traverser::Payload varpayload1 = Parameters.top();
	Parameters.pop();

	Traverser::Payload varpayload2 = Parameters.top();
	Parameters.pop();

	OutputPayload(varpayload2, ExpressionConstruction);
	ExpressionConstruction << L" " << operatorname << L" ";
	OutputPayload(varpayload1, ExpressionConstruction);

	Parameters.push(Traverser::Payload());
}

//
// Helper function for writing the beginning of a function definition
//
// This code includes preparatory steps for marshalling Epoch data into CUDA variables
//
void CompilationSession::FunctionPreamble(Extensions::OriginalCodeHandle handle)
{
	TemporaryCodeFile.OutputStream << L"extern \"C\" __global__ void " << widen(GenerateFunctionName(handle)) << L"(float* __marshal_input_floats)\n{\n";
	TemporaryCodeFile.OutputStream << L"\tunsigned int __marshal_float_index = 0;\n";
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
	TemporaryCodeFile.OutputStream << L"\t__marshal_float_index = 0;\n";

	for(std::list<Traverser::ScopeContents>::const_iterator iter = RegisteredVariables.begin(); iter != RegisteredVariables.end(); ++iter)
	{
		switch(iter->Type)
		{
		case VM::EpochVariableType_Real:
			TemporaryCodeFile.OutputStream << L"\t__marshal_input_floats[__marshal_float_index++] = " << iter->Identifier << L";\n";
			break;

		default:
			throw std::exception("Unsupported type - cannot generate code to transfer variable data from the CUDA device!");
		}
	}

	TemporaryCodeFile.OutputStream << L"}\n\n";
}


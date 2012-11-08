//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class containing a complete Epoch program
//

#include "pch.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Function.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Structure.h"
#include "Compiler/Intermediate Representations/Semantic Validation/CodeBlock.h"

#include "Compiler/Session.h"
#include "Compiler/Exceptions.h"

#include "Utility/StringPool.h"

#include <sstream>


using namespace IRSemantics;


//
// Construct and initialize a program
//
Program::Program(StringPoolManager& strings, CompileSession& session)
	: Strings(strings),
	  Session(session),
	  GlobalNamespace(IDSpace, strings, session)
{
}

//
// Add a string to the program's string pool
//
StringHandle Program::AddString(const std::wstring& str)
{
	StringHandle ret = IdentifierCache.Find(str);
	if(!ret)
	{
		ret = Strings.Pool(str);
		IdentifierCache.Add(str, ret);
	}
	
	return ret;
}

StringHandle Program::FindString(const std::wstring& str) const
{
	StringHandle cached = IdentifierCache.Find(str);
	if(cached)
		return cached;

	return Strings.Find(str);
}

const std::wstring& Program::GetString(StringHandle handle) const
{
	return Strings.GetPooledString(handle);
}

//
// Validate a program
//
bool Program::Validate(CompileErrors& errors) const
{
	return GlobalNamespace.Validate(errors);
}

bool Program::TypeInference(CompileErrors& errors)
{
	return GlobalNamespace.TypeInference(errors);
}

bool Program::CompileTimeCodeExecution(CompileErrors& errors)
{
	return GlobalNamespace.CompileTimeCodeExecution(errors);
}

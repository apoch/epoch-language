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
	// TODO - revisit the benefits of the identifier cache now that we have namespaces
	StringHandle ret = IdentifierCache.Find(str);
	if(!ret)
	{
		ret = Strings.Pool(str);
		IdentifierCache.Add(str, ret);
	}
	
	return ret;
}

//
// Find the handle of a string pooled in a program's string pool
//
StringHandle Program::FindString(const std::wstring& str) const
{
	StringHandle cached = IdentifierCache.Find(str);
	if(cached)
		return cached;

	return Strings.Find(str);
}

//
// Retrieve the content of a pooled string using its handle
//
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

//
// Perform type inference on a program
//
bool Program::TypeInference(CompileErrors& errors)
{
	return GlobalNamespace.TypeInference(errors);
}

//
// Perform compile-time code execution on a program
//
bool Program::CompileTimeCodeExecution(CompileErrors& errors)
{
	return GlobalNamespace.CompileTimeCodeExecution(errors);
}


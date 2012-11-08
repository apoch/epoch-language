//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// IR class containing a complete Epoch program
//

#pragma once


// Dependencies
#include "Utility/Types/IDTypes.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Helpers.h"
#include "Compiler/Intermediate Representations/Semantic Validation/Namespace.h"


// Forward declarations
class StringPoolManager;
class CompileSession;
class CompileErrors;


namespace IRSemantics
{

	// Forward declarations
	class Function;
	class Structure;
	class CodeBlock;

	//
	// Container for all contents of an Epoch program
	//
	class Program
	{
	// Construction
	public:
		Program(StringPoolManager& strings, CompileSession& session);

	// Non-copyable
	private:
		Program(const Program& other);
		Program& operator = (const Program& rhs);

	// String pooling
	public:
		StringHandle AddString(const std::wstring& str);
		StringHandle FindString(const std::wstring& str) const;
		const std::wstring& GetString(StringHandle handle) const;
		
		const StringPoolManager& GetStringPool() const
		{ return Strings; }

	// Compile-time code execution
	public:
		bool CompileTimeCodeExecution(CompileErrors& errors);

	// Type inference
	public:
		bool TypeInference(CompileErrors& errors);

	// Validation
	public:
		bool Validate(CompileErrors& errors) const;

	// Accessible state
	public:
		CompileSession& Session;
		Namespace GlobalNamespace;

	// Internal state
	private:
		StringPoolManager& Strings;
		GlobalIDSpace IDSpace;

	// String lookup caches
	private:
		impl::StringCache<std::wstring> IdentifierCache;
	};

}


//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Exception classes for the compiler system
//

#pragma once


//
// General failure of the compiler internally
//
// This typically indicates a logic bug in the compiler implemenation
// itself; even malformed code inputs should not trigger this exception
// type in general. All internal errors are considered fatal.
//
class InternalException : public FatalException
{
// Construction
public:
	explicit InternalException(const char* message) : FatalException(message) { }
	explicit InternalException(const std::string& message) : FatalException(message) { }
};

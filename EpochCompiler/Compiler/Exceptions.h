//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Exception classes for the compiler system
//

#pragma once


//
// General exception indicating that the programmer screwed up type safety somehow.
//
// Should almost always be fully recoverable, i.e. parsing/compilation should be allowed
// to continue even after one of these is thrown, so that the maximum amount of code is
// compiled and examined for errors on each pass.
//
class TypeMismatchException : public RecoverableException
{
// Construction
public:
	TypeMismatchException(const char* message) : RecoverableException(message) { }
	TypeMismatchException(const std::string& message) : RecoverableException(message) { }

// Helpers for making error reporting more friendly
public:
	virtual const char* GetErrorPrologue() const
	{
		return "The provided data type is not compatible with the expected data type.";
	}
};


//
// General exception indicating that the programmer screwed up some parameters somehow.
//
// Should almost always be fully recoverable, i.e. parsing/compilation should be allowed
// to continue even after one of these is thrown, so that the maximum amount of code is
// compiled and examined for errors on each pass.
//
class ParameterException : public RecoverableException
{
// Construction
public:
	ParameterException(const char* message) : RecoverableException(message) { }
	ParameterException(const std::string& message) : RecoverableException(message) { }

// Helpers for making error reporting more friendly
public:
	virtual const char* GetErrorPrologue() const
	{
		return "The function parameters provided do not match the expected parameters.";
	}
};


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
	explicit TypeMismatchException(const char* message) : RecoverableException(message) { }
	explicit TypeMismatchException(const std::string& message) : RecoverableException(message) { }
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
	explicit ParameterException(const char* message) : RecoverableException(message) { }
	explicit ParameterException(const std::string& message) : RecoverableException(message) { }
};


//
// Exception used to indicate a syntax error of some kind in a statement
//
class MalformedStatementException : public RecoverableException
{
// Construction
public:
	explicit MalformedStatementException(const char* message) : RecoverableException(message) { }
	explicit MalformedStatementException(const std::string& message) : RecoverableException(message) { }
};


//
// Exception used to indicate a recoverable failure to perform type inference
//
// The typical reaction to this exception is to make another compile pass to
// attempt to gather more type data. If a pass is completed without resolving
// any type information, leaving unfinished inferences, the compiler aborts.
//
class InferenceFailureException : public RecoverableException
{
// Construction
public:
	explicit InferenceFailureException(const char* message) : RecoverableException(message) { }
	explicit InferenceFailureException(const std::string& message) : RecoverableException(message) { }
};
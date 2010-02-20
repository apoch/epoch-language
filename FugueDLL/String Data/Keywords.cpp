//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Constant definitions for keywords used by the Epoch language
//

#include "pch.h"
#include "String Data/Keywords.h"
#include "Utility/Strings.h"


//-------------------------------------------------------------------------------
// Keyword constants
//-------------------------------------------------------------------------------

const wchar_t* Keywords::If = L"if";
const wchar_t* Keywords::ElseIf = L"elseif";
const wchar_t* Keywords::Else = L"else";

const wchar_t* Keywords::Do = L"do";
const wchar_t* Keywords::While = L"while";

const wchar_t* Keywords::Break = L"break";
const wchar_t* Keywords::Return = L"return";

const wchar_t* Keywords::Assign = L"assign";
const wchar_t* Keywords::ReadTuple = L"readtuple";
const wchar_t* Keywords::AssignTuple = L"assigntuple";
const wchar_t* Keywords::ReadStructure = L"readstructure";
const wchar_t* Keywords::AssignStructure = L"assignstructure";
const wchar_t* Keywords::Member = L"member";

const wchar_t* Keywords::Add = L"add";
const wchar_t* Keywords::Subtract = L"subtract";
const wchar_t* Keywords::Multiply = L"multiply";
const wchar_t* Keywords::Divide = L"divide";

const wchar_t* Keywords::Concat = L"concat";
const wchar_t* Keywords::Length = L"length";

const wchar_t* Keywords::Equal = L"equal";
const wchar_t* Keywords::NotEqual = L"notequal";
const wchar_t* Keywords::Less = L"less";
const wchar_t* Keywords::Greater = L"greater";
const wchar_t* Keywords::LessEqual = L"lessequal";
const wchar_t* Keywords::GreaterEqual = L"greaterequal";

const wchar_t* Keywords::DebugWrite = L"debugwritestring";
const wchar_t* Keywords::DebugRead = L"debugreadstring";

const wchar_t* Keywords::Map = L"map";
const wchar_t* Keywords::Reduce= L"reduce";

const wchar_t* Keywords::Cast = L"cast";

const wchar_t* Keywords::SizeOf = L"sizeof";

const wchar_t* Keywords::Or = L"or";
const wchar_t* Keywords::And = L"and";
const wchar_t* Keywords::Xor = L"xor";
const wchar_t* Keywords::Not = L"not";

const wchar_t* Keywords::Message = L"message";
const wchar_t* Keywords::AcceptMessage = L"acceptmsg";
const wchar_t* Keywords::ResponseMap = L"responsemap";
const wchar_t* Keywords::Caller = L"caller";
const wchar_t* Keywords::Sender = L"sender";

const wchar_t* Keywords::Var = L"var";
const wchar_t* Keywords::Integer = L"integer";
const wchar_t* Keywords::Integer16 = L"integer16";
const wchar_t* Keywords::String = L"string";
const wchar_t* Keywords::Boolean = L"boolean";
const wchar_t* Keywords::Real = L"real";
const wchar_t* Keywords::Tuple = L"tuple";
const wchar_t* Keywords::Structure = L"structure";
const wchar_t* Keywords::Array = L"array";
const wchar_t* Keywords::Buffer = L"buffer";

const wchar_t* Keywords::Reference = L"ref";
const wchar_t* Keywords::Constant = L"constant";

const wchar_t* Keywords::External = L"external";
const wchar_t* Keywords::Library = L"library";
const wchar_t* Keywords::Extension = L"extension";

const wchar_t* Keywords::Function = L"function";
const wchar_t* Keywords::Global = L"global";

const wchar_t* Keywords::Task = L"task";
const wchar_t* Keywords::Thread = L"thread";
const wchar_t* Keywords::ThreadPool = L"threadpool";
const wchar_t* Keywords::Future = L"future";

const wchar_t* Keywords::True = L"true";
const wchar_t* Keywords::False = L"false";

const wchar_t* Keywords::EntryPoint = L"entrypoint";

const wchar_t* Keywords::Infix = L"infix";
const wchar_t* Keywords::Alias = L"alias";

const wchar_t* Keywords::Comment = L"//";
const wchar_t* Keywords::FunctionArrow = L"->";
const wchar_t* Keywords::NullFunctionArrow = L"=>";
const wchar_t* Keywords::HexPrefix = L"0x";

const wchar_t* Keywords::DebugCrashVM = L"debug_crash_vm__";


//-------------------------------------------------------------------------------
// Additional utility functions
//-------------------------------------------------------------------------------

namespace
{
	std::map<const wchar_t*, std::string> KeywordCache;
	std::set<std::string> NarrowKeywordCache;
}

const char* Keywords::GetNarrowedKeyword(const wchar_t* keyword)
{
	std::map<const wchar_t*, std::string>::const_iterator iter = KeywordCache.find(keyword);
	if(iter != KeywordCache.end())
		return iter->second.c_str();

	return (KeywordCache[keyword] = narrow(keyword)).c_str();
}

const char* Keywords::GetNarrowedKeyword(const char* keyword)
{
	return NarrowKeywordCache.insert(std::string(keyword)).first->c_str();
}


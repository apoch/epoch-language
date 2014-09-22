//
// The Epoch Language Project
// Shared Library Code
//
// Common library functions for string manipulation
//

#include "pch.h"

#include "Utility/Strings.h"
#include "Utility/Types/IntegerTypes.h"

#include <iostream>
#include <vector>
#include <cstdlib>


//
// Convert a narrow string to a wide string
//
std::wstring widen(const std::string& str)
{
	const char* cstr = str.c_str();
	size_t len = str.length() + 1;
	size_t reqsize = 0;
#ifdef BOOST_WINDOWS
	if(mbstowcs_s(&reqsize, NULL, 0, cstr, len) != 0)
#else
    reqsize = mbstowcs(0, cstr, len);
    if(reqsize == -1)
#endif
		throw RecoverableException("Cannot widen string - invalid character detected");

	if(!reqsize)
		throw RecoverableException("Failed to widen string");

	std::vector<wchar_t> buffer(reqsize + 1, 0);
#ifdef BOOST_WINDOWS
	if(mbstowcs_s(NULL, &buffer[0], len, cstr, len) != 0)
#else
    if(mbstowcs(&buffer[0], cstr, len) == -1)
#endif
		throw RecoverableException("Cannot widen string - invalid character detected");

	return std::wstring(buffer.begin(), buffer.end() - 1);
}

//
// Widen an individual character
//
wchar_t widen(char c)
{
	wchar_t ret;
	mbtowc(&ret, &c, 1);
	return ret;
}


//
// Convert a wide string to a narrow string
//
std::string narrow(const std::wstring& str)
{
	const wchar_t* cstr = str.c_str();
	size_t len = str.length() + 1;
	size_t reqsize = 0;
#ifdef BOOST_WINDOWS
	if(wcstombs_s(&reqsize, NULL, 0, cstr, len) != 0)
#else
    reqsize = wcstombs(NULL, cstr, len);
    if(reqsize == (size_t)-1)
#endif
		throw RecoverableException("Cannot narrow string - invalid character detected");

	if(!reqsize)
		throw RecoverableException("Failed to narrow string");

	std::vector<Byte> buffer(reqsize, 0);
#ifdef BOOST_WINDOWS
	if(wcstombs_s(NULL, &buffer[0], len, cstr, len) != 0)
#else
    if(wcstombs(&buffer[0], cstr, len) != -1)
#endif
        throw RecoverableException("Cannot narrow string - invalid character detected");

	return std::string(buffer.begin(), buffer.end() - 1);
}

//
// Narrow an individual character
//
char narrow(wchar_t c)
{
	char ret;
#ifdef BOOST_WINDOWS
	wctomb_s(NULL, &ret, c, 1);
#else
    wctomb(&ret, c);
#endif
	return ret;
}


//
// Strip whitespace characters from the beginning and end of a string
//
std::wstring StripWhitespace(const std::wstring& str)
{
	const wchar_t* whitespacechars = L" \t\r\n";

	if(str.empty())
		return str;

	size_t pos = str.find_first_not_of(whitespacechars);
	size_t endpos = str.find_last_not_of(whitespacechars);

	if((pos == std::wstring::npos) || (endpos == std::wstring::npos))
		return L"";

	return str.substr(pos, endpos - pos + 1);
}

//
// Helper for removing quotes from around a string
//
std::wstring StripQuotes(const std::wstring& str)
{
	std::wstring ret(str);

	if(ret.empty())
		return ret;

	size_t beginpos = (ret[0] == L'\"') ? 1 : 0;
	size_t length = (*ret.rbegin() == L'\"') ? ret.length() - 1 : ret.length();

	return ret.substr(beginpos, length - beginpos);
}

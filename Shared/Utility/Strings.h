//
// The Epoch Language Project
// Shared Library Code
//
// Common library functions for string manipulation
//

#pragma once


std::wstring widen(const std::string& str);
wchar_t widen(char c);

std::string narrow(const std::wstring& str);
char narrow(wchar_t c);

std::wstring StripWhitespace(const std::wstring& str);

std::wstring StripQuotes(const std::wstring& str);


class Substring
{
// Construction
public:
	Substring(const std::wstring::const_iterator& begin, const std::wstring::const_iterator& end)
		: BeginIter(begin),
		  EndIter(end),
		  RBeginIter(begin == end ? end : end - 1)
	{ }

// std::wstring compatibility interface
public:
	size_t length() const
	{ return EndIter - BeginIter; }

	const std::wstring::const_iterator& begin() const
	{ return BeginIter; }

	const std::wstring::const_iterator& rbegin() const
	{ return RBeginIter; }

	size_t find(wchar_t c) const
	{
		size_t p = 0;
		for(std::wstring::const_iterator iter = BeginIter; iter != EndIter; ++iter)
		{
			if(*iter == c)
				return p;

			++p;
		}

		return std::wstring::npos;
	}

	operator std::wstring () const
	{ return std::wstring(BeginIter, EndIter); }

	bool operator == (const wchar_t* rhs) const
	{
		std::wstring::const_iterator iter = BeginIter;
		while(*rhs)
		{
			if(*rhs != *iter)
				return false;

			++rhs;
			++iter;

			if(iter == EndIter && *rhs)
				return false;
		}

		if(iter != EndIter)
			return false;

		return true;
	}

private:
	std::wstring::const_iterator BeginIter, EndIter;
	std::wstring::const_iterator RBeginIter;
};


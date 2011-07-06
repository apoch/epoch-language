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
//
// The Epoch Language Project
// Shared Library Code
//
// Common library functions for string manipulation
//

#pragma once


std::wstring widen(const std::string& str);
std::string narrow(const std::wstring& str);

std::wstring StripWhitespace(const std::wstring& str);


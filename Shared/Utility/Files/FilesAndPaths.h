//
// The Epoch Language Project
// Shared Library Code
//
// Common library functions for working with filenames and paths
//

#pragma once

std::wstring StripExtension(const std::wstring& filename);
std::wstring StripPath(const std::wstring& filename);
std::wstring StripFilename(const std::wstring& path);

std::wstring ShortenPathName(const std::wstring& path);


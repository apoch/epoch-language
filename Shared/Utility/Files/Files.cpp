//
// The Epoch Language Project
// Shared Library Code
//
// Utility code for reading and writing files
//

#include "pch.h"

#include "Utility/Files/Files.h"
#include "Utility/Files/FilesAndPaths.h"
#include "Utility/Strings.h"

#include <fstream>
#include <iterator>

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>


//
// Wrapper for loading a file's contents into a string
//
std::wstring Files::Load(const std::wstring& filename)
{
	std::wifstream infile(filename.c_str(), std::ios::binary);

    if(!infile)
		throw FileException("Could not open the input file " + narrow(filename));

	infile.unsetf(std::ios::skipws);

	return std::wstring(std::istream_iterator<wchar_t, wchar_t>(infile), std::istream_iterator<wchar_t, wchar_t>());
}

std::string Files::LoadNarrow(const std::string& filename)
{
	std::ifstream infile(filename.c_str(), std::ios::binary);

    if(!infile)
		throw FileException("Could not open the input file " + filename);

	infile.unsetf(std::ios::skipws);

	return std::string(std::istream_iterator<char, char>(infile), std::istream_iterator<char, char>());
}


//
// Wrapper for finding all files matching a specification (supports wildcards)
//
std::vector<std::wstring> Files::GetMatchingFiles(const std::wstring& filespec, bool recurse)
{
	using namespace boost::filesystem;
	std::vector<std::wstring> ret;

	path p = path(filespec);
	if(exists(p)) {
		if(is_regular_file(p)) {
			ret.push_back(p.generic_wstring());
		} else if(recurse && is_directory(p)) {
			directory_iterator endItor, itor(p);
			while(itor != endItor) {
				std::vector<std::wstring> contents = GetMatchingFiles(itor->path().generic_wstring(), true);
				std::copy(contents.begin(), contents.end(), std::back_inserter(ret));
				++itor;
			}
		}
	} else {
		const boost::wregex esc( _T("[\\^\\.\\$\\|\\(\\)\\[\\]\\+\\?\\/\\\\]") );
		const std::wstring rep( _T("\\\\\\1&") );
		const boost::wregex esc2(L"[\\*]");
		const std::wstring rep2(L".\\1&");
		const std::wstring tmp = boost::regex_replace(p.filename().generic_wstring(), esc, rep, boost::match_default | boost::format_sed);
		const std::wstring filterMatch = boost::regex_replace(tmp, esc2, rep2, boost::match_default | boost::format_sed);
		boost::wregex filter = boost::wregex(filterMatch);

		p.remove_filename();
		p = absolute(p);
		if(exists(p) && is_directory(p)) {
			directory_iterator endItor, itor(p);
			
			while(itor != endItor) {
				boost::wsmatch what;
				if(boost::regex_match(itor->path().generic_wstring(), what, filter))
					ret.push_back(itor->path().generic_wstring());
				++itor;
			}
		}
	}

	return ret;
}


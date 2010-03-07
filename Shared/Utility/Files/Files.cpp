//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Utility code for reading and writing files
//

#include "pch.h"

#include "Utility/Files/Files.h"
#include "Utility/Exception.h"

#include <fstream>
#include <boost/filesystem/operations.hpp>


//
// Wrapper for loading a file's contents directly into a memory buffer
//
void Files::Load(const char* filename, std::vector<Byte>& memory)
{
	std::ifstream infile(filename, std::ios::binary);

    if(!infile)
		throw FileException("Failed to load input file");

	infile.unsetf(std::ios::skipws);

	// Reserve memory for loading the file
	size_t size = GetFileSize(filename);
	memory.reserve(size + 10);	// Just some paranoia-padding

    std::copy(std::istream_iterator<Byte>(infile), std::istream_iterator<Byte>(), std::back_inserter(memory));
	memory.push_back(NULL);
}


size_t Files::GetFileSize(const char* filename)
{
	boost::intmax_t size = boost::filesystem::file_size(filename);
	if(size >= std::numeric_limits<size_t>::max())
		throw FileException("File too large to read into memory!");
	return static_cast<size_t>(size);
}


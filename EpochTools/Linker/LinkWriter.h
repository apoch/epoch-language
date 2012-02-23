//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Wrapper object for emitting the final linked binary file
//

#pragma once


// Dependencies
#include <fstream>


//
// Helper class with common functionality for outputting data
// to the destination executable file.
//
class LinkWriter
{
// Construction
public:
	explicit LinkWriter(std::ofstream& outputstream);

// Writing interface
public:
	void EmitByte(unsigned char out);
	void EmitWORD(WORD out);
	void EmitDWORD(DWORD out);
	void EmitNarrowString(const std::string& out);
	void EmitWideString(const std::wstring& out);
	void EmitBlob(void* data, size_t size);
	void Pad(std::streamsize size, unsigned char byte = 0x00);

// Status interface
public:
	DWORD GetOffset() const;

// Internal tracking
private:
	std::ofstream &OutputStream;
};



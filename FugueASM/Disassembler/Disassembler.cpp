//
// The Epoch Language Project
// FUGUE Bytecode assembler/disassembler
//
// Implementation of the disassembler portion of FugueASM
//

#include "pch.h"

#include "Disassembler.h"

#include "Translation Table/AssemblyTable.h"

#include "Bytecode/Bytecode.h"

#include "Serialization/SerializationTokens.h"

#include "Utility/Strings.h"

#include <fstream>
#include <iomanip>



// Prototypes
namespace { void Disassemble(std::ifstream& infile, std::ofstream& outfile); }

// Constants
enum DisassembleSingleResult
{
	CONTINUE_LOOP,			// Short-circuit the disassembly loop
	RECURSIVE_RETURN,		// Return to the next-highest block on the stack
	PROCEED					// Continue disassembling as normal
};


namespace
{

	//
	// Write a string to the output file
	//
	void WriteString(std::ofstream& outfile, const std::wstring& str)
	{
		std::string raw = narrow(str);
		raw = raw.substr(0, raw.find('\0'));
		outfile << raw;
	}

	void WriteString(std::ofstream& outfile, const std::string& str)
	{
		outfile.write(str.c_str(), static_cast<std::streamsize>(str.length()));
	}

	//
	// Write a hex-encoded number to the output file
	//
	void WriteHexNumber(std::ofstream& outfile, UINT_PTR value)
	{
		outfile << std::hex << std::uppercase << std::setfill('0') << std::setw(sizeof(value) * 2);
		outfile << value;
	}

	//
	// Write a number to the output file
	//
	void WriteNumber(std::ofstream& outfile, UINT_PTR value)
	{
		outfile << std::dec << std::setw(0);
		outfile << static_cast<Integer32>(value);
	}

	void WriteNumber(std::ofstream& outfile, Real value)
	{
		outfile << std::dec << std::setw(0);
		outfile << value;
	}

	//
	// Write a boolean constant to the output file
	//
	void WriteFlag(std::ofstream& outfile, bool value)
	{
		outfile << narrow(value ? Serialization::True : Serialization::False);
	}

	//
	// Write a newline to the output file
	//
	void WriteNewline(std::ofstream& outfile)
	{
		outfile << std::endl;		// This does double duty - we get the needed newline, plus a buffer flush
	}

	//
	// Write a space to the output file
	//
	void WriteSpace(std::ofstream& outfile)
	{
		outfile << " ";
	}


	//
	// Helper function for reading a multi-byte encoded number from the source file
	//
	UINT_PTR RetrieveNumber(std::ifstream& infile)
	{
		UINT_PTR ret;
		infile.read(reinterpret_cast<Byte*>(&ret), sizeof(ret));
		return ret;
	}

	//
	// Helper function for reading a float from the source file
	//
	Real RetrieveReal(std::ifstream& infile)
	{
		Real ret;
		infile.read(reinterpret_cast<Byte*>(&ret), sizeof(ret));
		return ret;
	}

	//
	// Helper function for reading a boolean flag
	//
	bool RetrieveFlag(std::ifstream& infile)
	{
		bool ret;
		infile.read(reinterpret_cast<Byte*>(&ret), 1);
		return ret;
	}

	//
	// Helper function for reading a null-terminated string
	//
	std::string RetrieveNullTerminatedString(std::ifstream& infile)
	{
		std::string str;
		while(true)
		{
			char c = 0;
			infile.read(&c, 1);
			if(infile.eof())
				throw Exception("Failed to find null terminator for string data!");
			if(!c)
				return str;

			str += c;
		}
	}

	//
	// Helper function for reading a string given its expected length
	//
	std::string RetrieveStringByLength(std::ifstream& infile, size_t len)
	{
		std::string str("\0", len);
		infile.read(&str[0], static_cast<std::streamsize>(len));
		return str;
	}

	//
	// Helper function for reading a single-byte instruction from the source file
	//
	unsigned char RetrieveInstruction(std::ifstream& infile)
	{
		char ret = 0;
		infile.read(&ret, 1);
		return ret;
	}

	//
	// Retrieve the next instruction from the input file, and ensure that
	// it matches our expectations. If not, an exception is thrown.
	//
	void ExpectInstruction(std::ifstream& infile, unsigned char instruction)
	{
		unsigned char retrieved = RetrieveInstruction(infile);
		if(retrieved != instruction)
		{
			std::ostringstream msg;
			msg << "Failed to read expected instruction!\nExpected 0x" << std::setw(2) << std::hex << std::setfill('0');
			msg << static_cast<Integer32>(instruction) << " retrieved 0x" << std::setw(2) << static_cast<Integer32>(retrieved) << " at location 0x" << std::setw(8) << infile.tellg();
			throw Exception(msg.str());
		}
	}


	//
	// Wrapper for disassembly process
	//
	// This function calls itself recursively to parse the inner contents
	// of certain code blocks. This is done so that the function can
	// effectively remain stateless, and only needs to do input-output
	// translation of instructions without "understanding" their context.
	//
	DisassembleSingleResult DisassembleSingle(unsigned char instruction, std::ifstream& infile, std::ofstream& outfile)
	{

	#define DEFINE_INSTRUCTION(bytecode, serializedtoken)	\
		if(instruction == bytecode)							\
		{													\
			WriteString(outfile, serializedtoken);			\

	#define DEFINE_ADDRESSED_INSTRUCTION(bytecode, serializedtoken) \
		if(instruction == bytecode)							\
		{													\
			WriteHexNumber(outfile, 0);						\
			WriteSpace(outfile);							\
			WriteString(outfile, serializedtoken);			\


	#define END_INSTRUCTION									\
			return CONTINUE_LOOP;							\
		}													\

	#define IF_ASSEMBLING									\
		if(false)											\
		{													\

	#define EXPECT(bytecode, serialized)					\
		ExpectInstruction(infile, bytecode);				\
		WriteString(outfile, serialized);					\

	#define COPY_INSTRUCTION								\
		DisassembleSingle(RetrieveInstruction(infile), infile, outfile); \

	#define COPY_UINT(paramname)							\
		UINT_PTR paramname = RetrieveNumber(infile);		\
		WriteNumber(outfile, paramname);					\

	#define COPY_BOOL(paramname)							\
		bool paramname = RetrieveFlag(infile);				\
		WriteFlag(outfile, paramname);						\

	#define COPY_STR(paramname)								\
		std::string paramname = RetrieveNullTerminatedString(infile);	\
		WriteString(outfile, paramname);					\

	#define COPY_REAL(paramname)							\
		Real paramname = RetrieveReal(infile);				\
		WriteNumber(outfile, paramname);					\

	#define COPY_HEX(paramname)								\
		UINT_PTR paramname = RetrieveNumber(infile);		\
		WriteHexNumber(outfile, paramname);					\

	#define COPY_RAW(length)								\
		WriteString(outfile, RetrieveStringByLength(infile, length));	\

	#define READ_NUMBER(paramname)							\
		UINT_PTR paramname = RetrieveNumber(infile);		\

	#define READ_STRING(paramname)							\
		std::string paramname = RetrieveNullTerminatedString(infile);	\

	#define READ_HEX(paramname)								\
		UINT_PTR paramname = RetrieveNumber(infile);		\

	#define READ_INSTRUCTION(paramname)						\
		unsigned char paramname = RetrieveInstruction(infile);	\

	#define WRITE(bytecode, serialized)						\
		WriteString(outfile, serialized);					\

	#define WRITE_STRING(paramname)							\
		WriteString(outfile, paramname);					\

	#define WRITE_NUMBER(paramname)							\
		WriteNumber(outfile, paramname);					\

	#define WRITE_HEX(paramname)							\
		WriteHexNumber(outfile, paramname);					\

	#define COPY_CLONEFLAG(paramname)						\
		READ_INSTRUCTION(paramname)							\
		WRITE_NUMBER(UINT_PTR(paramname ? 1 : 0))			\

	#define LOOP(counter)									\
		for(unsigned i = 0; i < counter; ++i)				\
		{													\

	#define ENDLOOP											\
		}													\

	#define IF_STR_MATCHES(paramname, bytecode, serialized)	\
		if(widen(paramname) == serialized)					\
		{													\

	#define IF_UINT_MATCHES(param1, param2)					\
		if(param1 == param2)								\
		{													\

	#define IF_BOOL(paramname)								\
		if(paramname)										\
		{													\

	#define IF_INSTRUCTION_MATCHES(paramname, bytecode, serialized)	\
		if(paramname == bytecode)							\
		{													\

	#define ELSE											\
		} else {											\

	#define END_IF											\
		}													\
		
	#define EXCEPTION(msg)									\
		throw Exception(msg);								\

	#define RECURSE											\
		Disassemble(infile, outfile);						\

	#define RETURN											\
		return RECURSIVE_RETURN;							\

	#define SKIPTOSTRING(length)

	#define SPACE											\
		WriteSpace(outfile);								\

	#define NEWLINE											\
		WriteNewline(outfile);								\


		ASSEMBLYTABLE

		return PROCEED;
	}


	//
	// Driver loop for the disassembler. This function takes care of
	// traversing the bytecode and converting operations back into
	// the original EpochASM instructions.
	//
	void Disassemble(std::ifstream& infile, std::ofstream& outfile)
	{
		// Disassemble instructions until we run out
		while(true)
		{
			unsigned char instruction = RetrieveInstruction(infile);

			if(infile.eof())
				break;

			DisassembleSingleResult result = DisassembleSingle(instruction, infile, outfile);
			if(result == CONTINUE_LOOP)
				continue;
			else if(result == RECURSIVE_RETURN)
				return;

			{
				std::ostringstream msg;
				msg << "Failed to disassemble instruction: 0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<Integer32>(instruction);
				msg << "\nRead location: 0x" << std::setw(8) << infile.tellg();
				throw Exception(msg.str());
			}
		}
	}

}


//
// Main function for disassembling a binary code file
//
bool Disassembler::DisassembleFile(const std::wstring& inputfile, const std::wstring& outputfile)
{
	std::wcout << L"Epoch Disassembler Utility" << std::endl;
	std::wcout << L"I: " << inputfile << std::endl;
	std::wcout << L"O: " << outputfile << std::endl;

	try
	{
		// We do not use wide streams here because we have to do a lot of single-byte operations
		std::ifstream infile(narrow(inputfile).c_str(), std::ios::binary);

		if(!infile)
			throw FileException("Could not open input file!");

		std::ofstream outfile(narrow(outputfile).c_str());

		if(!outfile)
			throw FileException("Could not open output file!");

		// Validate cookie
		char cookie[10] = {0,};
		if(strlen(Bytecode::HeaderCookie) >= sizeof(cookie))
			throw Exception("Not enough space to read header cookie!");
		infile.read(cookie, static_cast<std::streamsize>(strlen(Bytecode::HeaderCookie)));
		if(memcmp(cookie, Bytecode::HeaderCookie, strlen(Bytecode::HeaderCookie)) != 0)
			throw Exception("Input file is missing header cookie!");

		WriteHexNumber(outfile, RetrieveNumber(infile));
		WriteNewline(outfile);

		ExpectInstruction(infile, Bytecode::Scope);
		UINT_PTR scopeid = RetrieveNumber(infile);
		WriteHexNumber(outfile, scopeid);
		WriteSpace(outfile);
		WriteString(outfile, Serialization::Scope);
		WriteNewline(outfile);
		Disassemble(infile, outfile);
		
		if(!infile.eof())
			Disassemble(infile, outfile);

		std::wcout << L"Successfully disassembled.\n" << std::endl;
		return true;
	}
	catch(std::exception& err)
	{
		::DeleteFile(outputfile.c_str());

		std::wcout << L"ERROR - " << err.what() << std::endl;
		std::wcout << L"DISASSEMBLY FAILED!\n" << std::endl;
		return false;
	}
}


//
// The Epoch Language Project
// FUGUE Bytecode assembler/disassembler
//
// Implementation of the assembler portion of FugueASM
//
// Assembling a program is accomplished using a special table file
// (see AssemblyTable.h). This file consists of a small domain-specific
// language which can convert between the text and binary forms of
// EpochASM programs. We specify the functionality of the DSL itself
// using a series of macros; these macros automatically generate
// assembly/disassembly code using the table. This centralizes the
// knowledge of how to convert between formats, which is the primary
// goal of the FugueASM utility. Using this code-generation system
// minimizes the number of changes we have to make in order to
// introduce, remove, or modify any EpochASM instruction.
//

#include "pch.h"

#include "Assembler/Assembler.h"
#include "Translation Table/AssemblyTable.h"

#include "Utility/Strings.h"

#include "Utility/Types/IDTypes.h"
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/RealTypes.h"
#include "Utility/Types/EpochTypeIDs.h"

#include <fstream>
#include <iomanip>


// Prototypes
namespace { void Assemble(std::wifstream& infile, std::wofstream& outfile); }


// Constants
enum AssembleSingleResult
{
	CONTINUE_LOOP,			// Short-circuit the assembly loop
	RECURSIVE_RETURN,		// Return to the next-highest block on the stack
	PROCEED					// Continue assembling as normal
};


namespace
{

	//
	// Wrapper for writing an instruction to the binary output file
	//
	void WriteInstruction(std::wofstream& outfile, unsigned char instructionbyte)
	{
		outfile << static_cast<Byte>(instructionbyte);
	}

	//
	// Wrapper for writing integer literals to the binary output file
	//
	void WriteLiteral(std::wofstream& outfile, UINT_PTR value)
	{
		outfile << static_cast<Byte>(static_cast<unsigned char>(value & 0xff));
		outfile << static_cast<Byte>(static_cast<unsigned char>((value >> 8) & 0xff));
		outfile << static_cast<Byte>(static_cast<unsigned char>((value >> 16) & 0xff));
		outfile << static_cast<Byte>(static_cast<unsigned char>((value >> 24) & 0xff));
	}

	//
	// Wrapper for writing real literals to the binary output file
	//
	void WriteLiteral(std::wofstream& outfile, Real value)
	{
		Integer32 temp;
		temp = *reinterpret_cast<Integer32*>(&value);
		outfile << static_cast<Byte>(static_cast<unsigned char>(temp & 0xff));
		outfile << static_cast<Byte>(static_cast<unsigned char>((temp >> 8) & 0xff));
		outfile << static_cast<Byte>(static_cast<unsigned char>((temp >> 16) & 0xff));
		outfile << static_cast<Byte>(static_cast<unsigned char>((temp >> 24) & 0xff));
	}

	//
	// Wrapper for writing a boolean literal to the binary output file
	//
	void WriteLiteral(std::wofstream& outfile, bool value)
	{
		outfile << static_cast<Byte>(value ? 1 : 0);
	}

	//
	// Wrapper for writing a null-terminated string to the binary output file
	//
	void WriteTerminatedString(std::wofstream& outfile, const std::wstring& str)
	{
		outfile << str;
		if(str[str.length() - 1] != '\0')
			outfile << '\0';
	}

	//
	// Wrapper for writing a raw string to the binary output file
	//
	void WriteRawString(std::wofstream& outfile, const std::wstring& str)
	{
		outfile.write(str.c_str(), static_cast<std::streamsize>(str.length()));
	}


	//
	// Helper function for loading integers
	//
	UINT_PTR RetrieveNumber(std::wifstream& stream)
	{
		std::wstring opidstr;

		if(!(stream >> opidstr))
			throw Exception("Failed to extract number from assembly source!");

		if(opidstr == Serialization::True)
			return 1;
		else if(opidstr == Serialization::False)
			return 0;

		std::wstringstream convert;
		convert << opidstr;
		UINT_PTR opid;

		if(!(convert >> opid))
			throw Exception("Failed to convert input into a number!");

		return opid;
	}

	//
	// Helper function for loading reals
	//
	Real RetrieveFloat(std::wifstream& stream)
	{
		std::wstring opidstr;

		if(!(stream >> opidstr))
			throw Exception("Failed to extract real from assembly source!");

		std::wstringstream convert;
		convert << opidstr;
		Real opid;

		if(!(convert >> opid))
			throw Exception("Failed to convert input into a real!");

		return opid;
	}

	//
	// Helper function for loading booleans
	//
	bool RetrieveBoolean(std::wifstream& stream)
	{
		std::wstring str;
		if(!(stream >> str))
			throw Exception("Failed to extract boolean from assembly source!");

		if(str == Serialization::True || str == L"1")
			return true;

		return false;
	}

	//
	// Helper function for loading hex-encoded IDs
	//
	UINT_PTR RetrieveHexNumber(std::wifstream& stream)
	{
		std::streamsize pos = stream.tellg();

		std::wstring opidstr;

		if(!(stream >> opidstr))
			throw Exception("Failed to extract hex number from assembly source!");

		std::wstringstream convert;
		convert << std::hex << opidstr;
		UINT_PTR opid;

		if(!(convert >> opid))
		{
			std::ostringstream msg;
			msg << "Failed to convert input into a hex number! 0x" << std::hex << std::setw(8) << std::setfill('0') << pos;
			throw Exception(msg.str());
		}

		return opid;
	}

	//
	// Helper functions for loading assembly language strings
	//
	std::wstring RetrieveString(std::wifstream& stream)
	{
		std::wstring ret;
	
		if(!(stream >> ret))
			return L"";

		return ret;
	}

	std::wstring RetrieveStringWithLength(std::wifstream& stream, size_t len)
	{
		std::wstring ret(L"\0", len);
		stream.read(&ret[0], static_cast<std::streamsize>(len));
		return ret;
	}

	//
	// Helper function for confirming that expected tokens are present
	//
	void ExpectToken(std::wifstream& stream, const std::wstring& token)
	{
		std::streampos pos = stream.tellg();

		std::wstring str;
		if(!(stream >> str))
			throw Exception("Failed to read any token!");

		if(str != token)
		{
			std::ostringstream msg;
			msg << "Failed to read expected token! Expected " << narrow(token) << " at 0x" << std::hex << std::setw(8) << std::setfill('0') << pos;
			throw Exception(msg.str());
		}
	}

	//
	// Actual implementation of the assembler, using our macro-based table system
	//
	AssembleSingleResult AssembleSingle(const std::wstring& str, std::wifstream& infile, std::wofstream& outfile)
	{
	#define DEFINE_INSTRUCTION(bytecode, serializedtoken)	\
		if(str == serializedtoken)							\
		{													\
			WriteInstruction(outfile, bytecode);			\

	#define DEFINE_ADDRESSED_INSTRUCTION(bytecode, serializedtoken) \
		DEFINE_INSTRUCTION(bytecode, serializedtoken)		\

	#define END_INSTRUCTION									\
			return CONTINUE_LOOP;							\
		}													\

	#define IF_ASSEMBLING									\
		if(true)											\
		{													\

	#define EXPECT(bytecode, serialized)					\
		ExpectToken(infile, serialized);					\
		WriteInstruction(outfile, bytecode);				\

	#define COPY_INSTRUCTION								\
		AssembleSingle(RetrieveString(infile), infile, outfile); \

	#define COPY_UINT(paramname)							\
		UINT_PTR paramname = RetrieveNumber(infile);		\
		WriteLiteral(outfile, paramname);					\

	#define COPY_BOOL(paramname)							\
		bool paramname = RetrieveBoolean(infile);			\
		WriteLiteral(outfile, paramname);					\

	#define COPY_CLONEFLAG(paramname)						\
		COPY_BOOL(paramname)								\

	#define COPY_STR(paramname)								\
		std::wstring paramname = RetrieveString(infile);	\
		WriteTerminatedString(outfile, paramname);			\

	#define COPY_REAL(paramname)							\
		Real paramname = RetrieveFloat(infile);				\
		WriteLiteral(outfile, paramname);					\

	#define COPY_HEX(paramname)								\
		UINT_PTR paramname = RetrieveHexNumber(infile);		\
		WriteLiteral(outfile, paramname);					\

	#define COPY_RAW(length)								\
		WriteRawString(outfile, RetrieveStringWithLength(infile, length));	\

	#define READ_NUMBER(paramname)							\
		UINT_PTR paramname = RetrieveNumber(infile);		\

	#define READ_STRING(paramname)							\
		std::wstring paramname = RetrieveString(infile);	\

	#define READ_HEX(paramname)								\
		UINT_PTR paramname = RetrieveHexNumber(infile);		\

	#define READ_INSTRUCTION(paramname)						\
		READ_STRING(paramname)								\

	#define WRITE(bytecode, serialized)						\
		WriteInstruction(outfile, bytecode);				\

	#define WRITE_STRING(paramname)							\
		WriteTerminatedString(outfile, paramname);			\

	#define WRITE_NUMBER(paramname)							\
		WriteLiteral(outfile, paramname);					\

	#define WRITE_HEX(paramname)							\
		WRITE_NUMBER(paramname)								\

	#define LOOP(counter)									\
		for(unsigned i = 0; i < counter; ++i)				\
		{													\

	#define ENDLOOP											\
		}													\

	#define IF_STR_MATCHES(paramname, bytecode, serialized)	\
		if(paramname == serialized)							\
		{													\

	#define IF_UINT_MATCHES(param1, param2)					\
		if(param1 == param2)								\
		{													\

	#define IF_BOOL(paramname)								\
		if(paramname)										\
		{													\

	#define IF_INSTRUCTION_MATCHES(paramname, bytecode, serialized)	\
		IF_STR_MATCHES(paramname, bytecode, serialized)		\

	#define ELSE											\
		} else {											\

	#define END_IF											\
		}													\
		
	#define EXCEPTION(msg)									\
		throw Exception(msg);								\

	#define RECURSE											\
		Assemble(infile, outfile);							\

	#define RETURN											\
		return RECURSIVE_RETURN;							\

	#define SKIPTOSTRING(length)							\
		if(length) { infile.ignore(); }						\

	#define SPACE

	#define NEWLINE


		ASSEMBLYTABLE

		return PROCEED;
	}


	//
	// Driver loop that repeatedly reads and processes instructions to assemble
	//
	void Assemble(std::wifstream& infile, std::wofstream& outfile)
	{
		while(true)
		{
			std::wstring str = RetrieveString(infile);

			if(infile.eof())
				return;

			AssembleSingleResult result = AssembleSingle(str, infile, outfile);
			if(result == CONTINUE_LOOP)
				continue;
			else if(result == RECURSIVE_RETURN)
				return;

			// Discard unused hex addresses
			if(str.length() == 8 && str.find_first_not_of(L"0123456789ABCDEF") == std::string::npos)
				continue;

			throw Exception("Failed to assemble instruction: \"" + narrow(str) + "\"");
		}
	}

}


//
// Main function for assembling a code file
//
bool Assembler::AssembleFile(const std::wstring& inputfile, const std::wstring& outputfile)
{
	std::wcout << L"Epoch Assembler Utility" << std::endl;
	std::wcout << L"I: " << inputfile << std::endl;
	std::wcout << L"O: " << outputfile << std::endl;

	try
	{
		std::wifstream infile(narrow(inputfile).c_str(), std::ios::in);

		if(!infile)
			throw Exception("Failed to load source file!");

		std::wofstream outfile(narrow(outputfile).c_str(), std::ios::binary);

		if(!outfile)
			throw Exception("Failed to open destination file for writing!");

		outfile << Bytecode::HeaderCookie;
		UINT_PTR firstscopeid = RetrieveHexNumber(infile);		// Copy first scope's ID so it doesn't get discarded by the assembler table
		ExpectToken(infile, Serialization::Scope);
		WriteInstruction(outfile, Bytecode::Scope);
		WriteLiteral(outfile, firstscopeid);
		ExpectToken(infile, Serialization::ParentScope);
		WriteInstruction(outfile, Bytecode::ParentScope);
		UINT_PTR parentid = RetrieveHexNumber(infile);
		WriteLiteral(outfile, parentid);

		do
		{
			Assemble(infile, outfile);
		} while(!infile.eof());


		std::wcout << L"Successfully assembled.\n" << std::endl;
		return true;
	}
	catch(std::exception& err)
	{
		::DeleteFile(outputfile.c_str());

		std::wcout << L"ERROR - " << err.what() << std::endl;
		std::wcout << L"ASSEMBLY FAILED!\n" << std::endl;
		return false;
	}
}


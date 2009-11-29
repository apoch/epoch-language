//
// The Epoch Language Project
// FUGUE Bytecode assembler/disassembler
//
// Functions exported by the assembler/disassembler library
//

#include "pch.h"

#include "Assembler/Assembler.h"
#include "Disassembler/Disassembler.h"

#include "Utility/Strings.h"



//
// Assemble the file requested by the user
//
bool __stdcall DoAssemble(const char* inputfilename, const char* outputfilename)
{
	return Assembler::AssembleFile(widen(inputfilename), widen(outputfilename));
}

//
// Disassemble the file requested by the user
//
bool __stdcall DoDisassemble(const char* inputfilename, const char* outputfilename)
{
	return Disassembler::DisassembleFile(widen(inputfilename), widen(outputfilename));
}

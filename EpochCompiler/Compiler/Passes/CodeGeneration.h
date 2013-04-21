//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Pass for converting a semantic validation IR
// into output bytecode for the Epoch VM.
//

#pragma once


// Forward declarations
namespace IRSemantics { class Program; }
class BytecodeEmitterBase;


namespace CompilerPasses
{

	bool GenerateCode(const IRSemantics::Program& program, BytecodeEmitterBase& emitter);

}



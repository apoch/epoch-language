//
// Stub - shell out to self-hosting code generator
//

#pragma once


namespace IRSemantics { class Program; }


namespace CompilerPasses
{

	bool GenerateCodeSelfHosted(const IRSemantics::Program& program);

}


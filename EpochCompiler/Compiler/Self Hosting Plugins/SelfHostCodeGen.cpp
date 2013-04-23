//
// Stub - shell out to self-hosting code generator
//

#include "pch.h"

#include "Compiler/Self Hosting Plugins/SelfHostCodeGen.h"
#include "Compiler/Self Hosting Plugins/Plugin.h"

#include "Compiler/Intermediate Representations/Semantic Validation/Program.h"

#include "Utility/StringPool.h"



bool CompilerPasses::GenerateCodeSelfHosted(const IRSemantics::Program& program)
{
	const StringPoolManager& strings = program.GetStringPool();
	const boost::unordered_map<StringHandle, std::wstring>& stringpool = strings.GetInternalPool();

	for(boost::unordered_map<StringHandle, std::wstring>::const_iterator iter = stringpool.begin(); iter != stringpool.end(); ++iter)
		Plugins.InvokeVoidPluginFunction(L"PluginCodeGenRegisterString", iter->first, iter->second.c_str());


	// TODO - traverse entire program and register it with the plugin
	((void)(program));

	// TODO - should we care about potential failure status from the plugin?
	Plugins.InvokeVoidPluginFunction(L"PluginCodeGenProcessProgram");
	return true;
}



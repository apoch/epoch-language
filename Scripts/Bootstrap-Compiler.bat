@echo off
echo Begin bootstrap!
pushd "C:\Code\epoch-language\EpochDevTools"
"C:\Code\epoch-language\EpochDevTools32\Debug\x86\Compiler32.exe" /files Common\DataStructures.epoch;Common\DataStructures\BinaryTree.epoch;Common\DataStructures\LinkedList.epoch;Common\DataStructures\Trie.epoch;Common\Lexer.epoch;Common\Parser.epoch;Common\Project.epoch;Common\Strings.epoch;Common\StringTable.epoch;Common\Win32.epoch;Compiler\Assignments.epoch;Compiler\Builtin.epoch;Compiler\ByteStream.epoch;Compiler\CodeBlocks.epoch;Compiler\CodeGen.epoch;Compiler\Compiler.epoch;Compiler\Dump.epoch;Compiler\Entities.epoch;Compiler\Exe.epoch;Compiler\Expressions.epoch;Compiler\Functions.epoch;Compiler\Globals.epoch;Compiler\IR.epoch;Compiler\LLVM.epoch;Compiler\Overloads.epoch;Compiler\PatternMatching.epoch;Compiler\Resources.epoch;Compiler\Scopes.epoch;Compiler\Structures.epoch;Compiler\SumTypes.epoch;Compiler\Templates.epoch;Compiler\TypeChecking.epoch;Compiler\TypeInfo.epoch;Compiler\TypeMatching.epoch;PDB\PDB.epoch;PDB\Symbols.epoch /output c:\Code\epoch-language\bootstrap\CompilerTest.exe > c:\code\epoch-language\bootstrap\log.txt 2> C:\code\epoch-language\bootstrap\err.txt
popd
echo Bootstrap attempt complete.
pause

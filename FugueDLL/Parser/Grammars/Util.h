//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Utility macros used in the parser grammars
//


// No include guard because this file is used in multiple grammar
// header files, and needs to be reparsed for each inclusion to
// ensure the macros are available and correctly defined.

#define KEYWORD(k)	(Keywords::GetNarrowedKeyword(Keywords::k))
#define OPERATOR(o)	(Keywords::GetNarrowedKeyword(Operators::o))


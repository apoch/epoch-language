# Epoch Language Grammar #

_**NOTE:** This page is still work in progress. Some productions may be missing and precedences often do not reflect the actual parser's order of operations._


```

AlphaCharacter      = ? A-Z a-z ? ;

AlphaNumeric        = AlphaCharacter
                    | Digit
                    ;

Assignment          = AssignmentLHS, [Whitespace], "=", [Whitespace], AssignmentRHS ;

AssignmentLHS       = CompoundIdentifier
                    | Identifier
                    ;

AssignmentRHS       = AssignmentLHS
                    | Expression
                    ;

ChainEntity         = ChainName, [Whitespace], ["(", ExpressionList, ")"],
                      CodeBlock,
                      [ChainEntity]
                    ;

ChainName           = "elseif"
                    ;

ChainTermEntity     = ChainTermName,
                      CodeBlock
                    ;

ChainTermName       = "else"
                    ;

CodeBlock           = "{", { [Whitespace], CodeBlockContents, [Whitespace] }, "}" ;

CodeBlockContents   = Entity
                    | Preoperation
                    | Postoperation
                    | Statement
                    | Initialization
                    | Assignment
                    | CodeBlock
                    ;

CompoundIdentifier  = Identifier, { ".", Identifier } ;

Digit               = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;

Entity              = PrefixEntity
                    | PostfixEntity
                    ;

EntityName          = "if"
                    | "while"
                    ;

Expression          = ExpressionTerm, [ExpressionContinued] ;

ExpressionList      = { [Whitespace], [Expression], [Whitespace], "," },
                      [Whitespace], Expression, [Whitespace]

ExpressionTerm      = Parenthetical
                    | ([UnaryOperator], Operand)
                    | Preoperation
                    | Postoperation
                    | Statement
                    ;

FuncSigParams       = ReferenceTypeName, { [Whitespace], ",", [Whitespace], ReferenceTypeName } ;

FuncSigReturn       = [Whitespace], "->", [Whitespace], TypeName, [Whitespace], [TemplateArguments] ;

FunctionDefinition  = Identifier, [Whitespace],
                      [TemplateParameters], [Whitespace],
                      ":",
                      [FunctionParams],
                      [FunctionReturn],
                      [FunctionTags],
                      [CodeBlock]
                    ;

FunctionParams      = Param, { [Whitespace], ",", [Whitespace], Param } ;

FunctionSignature   = "(",
                      [Whitespace], Identifier, [Whitespace],
                      ":", [Whitespace],
                      [FuncSigParams],
                      [FuncSigReturn], [Whitespace],
                      ")"
                    ;

FunctionReturn      = [Whitespace], "->", [Whitespace], Expression ;

FunctionTag         = Identifier, [Whitespace], { "(", [Whitespace], LiteralString, [Whitespace], ")" } ;

FunctionTags        = "[", FunctionTag { [Whitespace], ",", [Whitespace], FunctionTag }, "]" ;

GlobalBlock         = "global", [Whitespace], "{", { [Whitespace], Initialization, [Whitespace] }, "}" ;

HexDigit            = Digit
                    | "a" | "A"
                    | "b" | "B"
                    | "c" | "C"
                    | "d" | "D"
                    | "e" | "E"
                    | "f" | "F"
                    ;

Identifier          = AlphaCharacter, { AlphaNumeric | Underscore } ;

Initialization      = TypeName, Whitespace,
                      Identifier, [Whitespace], "=", [Whitespace]
                      ExpressionList
                    ;

LiteralBoolean      = "true"
                    | "false"
                    ;

LiteralHex          = "0x", HexDigit, { HexDigit } ;

LiteralInteger      = Digit, { Digit } ;

LiteralReal         = Digit, { Digit }, ".", Digit, { Digit } ;

LiteralString       = '"', ? Any sequence of characters besides double-quotes ?, '"' ;

Operand             = LiteralBoolean
                    | LiteralReal
                    | LiteralInteger
                    | LiteralHex
                    | LiteralString
                    | Identifier
                    ;

Param               = ReferenceTypeName, Whitespace, Identifier ;

Parenthetical       = "(", Expression, ")" ;

PostfixEntity       = PostfixEntityName, [Whitespace],
                      CodeBlock,
                      PostfixEntityCloser,
                      "(", ExpressionList, ")"
                    ;

Postoperation       = [Identifier | CompoundIdentifier], Postoperator ;

Postoperator        = "++"
                    | "--"
                    ;

PrefixEntity        = EntityName, [Whitespace], "(", ExpressionList, ")",
                      CodeBlock,
                      [ChainEntity],
                      [ChainTermEntity]
                    ;

Preoperation        = Preoperator, [Identifier | CompoundIdentifier] ;

Preoperator         = "++"
                    | "--"
                    ;

Program             = { [Whitespace], TopLevelContent, [Whitespace] } ;

ReferenceTypeName   = TypeName, [Whitespace], [TemplateArguments], [(Whitespace, "ref")] ;

Statement           = Identifier, [Whitespace], [TemplateArguments],
                      "(", [ExpressionList], ")"
                    ;

StructureDefinition = "structure", Whitespace,
                      Identifier, [Whitespace],
                      [TemplateParameters], [Whitespace],
                      ":", [Whitespace],
                      StructureMember, { [Whitespace], ",", [Whitespace], StructureMember }
                    ;

StructureMember     = StructureMemberVar
                    | StructureMemberFunc
                    ;

StructureMemberFunc = FunctionSignature ;

StructureMemberVar  = ReferenceTypeName, [Whitespace], Identifier ;

SumTypeDefinition   = "type", [Whitespace],
                      [TemplateParameters],
                      ":", [Whitespace],
                      TypeName, [Whitespace],
                      "|", [Whitespace],
                      TypeName, { [Whitespace], "|", [Whitespace], TypeName }
                    ;

TemplateArguments   = "<", [Whitespace],
                      Identifier, { [Whitespace], ",", [Whitespace], Identifier }, [Whitespace],
                      ">"
                    ;

TemplateParam       = "type", Whitespace, Identifier ;

TemplateParameters  = "<", [Whitespace],
                      TemplateParam, { [Whitespace], ",", TemplateParam }, [Whitespace],
                      ">"
                    ;

TopLevelContent     = TypeAliasWeak
                    | TypeAliasStrong
                    | SumTypeDefinition
                    | StructureDefinition
                    | FunctionDefinition
                    | GlobalBlock
                    ;

TypeAliasStrong     = "type", Whitespace, Identifier, [Whitespace], ":", [Whitespace], TypeName ;

TypeAliasWeak       = "alias", Whitespace, Identifier, [Whitespace], "=", [Whitespace], TypeName ;

TypeName            = (Identifier, [Whitespace], TemplateArguments)
                    | "integer"
                    | "integer16"
                    | "boolean"
                    | "real"
                    | "string"
                    | "buffer"
                    | "nothing"
                    ;

UnaryOperator       = "!"
                    ;

Underscore          = "_" ;

Whitespace          = ? Any sequence of one or more spaces, tabs, newlines ? ;

```


See [EBNF on Wikipedia](http://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_Form) for details on the notation.
# Epoch Compilation Model #
Epoch makes use of a multiple pass compilation model. The first pass is used to accumulate simple metadata about the program code, including the locations of function definitions, structure type definitions, and so on. Subsequent passes then progressively elaborate the abstract syntax tree as necessary, filling in as much information as possible. In most programs only two additional passes may be necessary; however, in some programs, particularly where type inference rules are heavily utilized, it may be necessary to invoke additional passes to resolve various details of the program under compilation. A typical Epoch program undergoes a total of three passes of compilation before all type inference resolution is completed; some require four or more (including the prepass.)

## Metadata "Prepass" ##
The first pass over the program is used to build a table of some of the symbols used by the program, specifically function identifiers, structure type identifiers, and so on. Additionally, information about function signatures and overloads is collected. A catalog of pattern-matched functions is constructed as well, and a list of dispatchers needed for the pattern matching implementation is assembled. Structure type definitions are processed and cached for later passes. Certain function tags such as `external` are also processed, and used to generate additional metadata.

The purpose of this pass is to facilitate separate compilation and eliminate the need for declarations versus definitions in Epoch code. By performing a fairly naive pass over the code first, we can gather enough information about the functions, types, and so on provided by a program that we do not need to enforce rules about defining a function or type prior to its use. In this way, we can do things like have function A call function B which is defined in a different part of the code (even a different file) without having to explicitly declare B first.

This is in direct contrast to the C and C++ compilation model which enforces a distinction between declaration and definition; it is only possible to use a function, type, or variable which has been declared. In many cases it is only legal to use a type which has also been defined. Epoch eliminates this distinction and an entire class of annoyances during compilation; once something is defined in code, it is accessible everywhere (subject to namespace and scoping rules of course) without additional overhead.

The compiler also uses the metadata "prepass" to cache information which makes it easier to output error messages later on during the process.

> _**Implementation Note:** The existing compiler implementation uses the boolean flag `CompilationSemantics::IsPrepass` to indicate that the prepass is being performed._


## Second Pass ##
Once the prepass is complete, a second pass is performed, which actually begins examining the code for syntactical and semantic errors, as well as accumulating more metadata about the program. In particular, this pass looks at the _typing semantics_ of the program, noting situations where inference must be performed. When type inference can be accomplished immediately, this pass elaborates and decorates the abstract syntax tree accordingly; if additional metadata is required, a flag is set which causes the compiler to enqueue another pass, and the AST is marked as needing further elaboration.

The primary purpose of this pass is to determine where type inference rules must be applied; although some rules are applied immediately, this is simply an optimization. By nature the second code pass is intended to discover where additional compilation exploration is needed.

> _**Implementation Note:** The compiler implementation will detect the performance of elaboration passes by checking if the `IsPrepass` flag is `false` and the `IsInferenceComplete` flag is also `false`._


## Additional Elaboration Passes ##
Subject to the vagaries of separate compilation, interaction with external library code, and so on, the compiler may need to perform several elaboration passes before resolving all of the details of the program being compiled. These passes generally involve looking through the AST for marked areas that require elaboration, and attempting to elaborate them based on information discovered in prior passes. Once a pass is completed that leaves no areas of the AST marked in this way, the final pass is performed.

Note that due to the optimization of the second pass, it is fairly rare in current Epoch programs to need additional elaboration passes. In most cases the second pass can immediately elaborate the vast majority of the detail needed to complete compilation.


## Final Code Generation Pass ##
Once the elaboration passes are finished, the compiler performs one final run through the code, traversing the now-decorated AST and generating Epoch bytecode as it goes.

> _**Implementation Note:** The compiler checks for `IsPrepass` to be `false` and `IsInferenceComplete` to be `true` to detect when a final code emission pass is being performed._

> _**Optimization Note:** It should be possible to avoid executing the actual code parser during the final pass, and operate strictly on the generated AST. This would substantially reduce the execution time of the final pass itself. Analogous improvements could be made to the elaboration passes as well. However, the parser currently in use is desperately in need of replacement anyways, so these optimizations are considered low priority until such time as a faster base parser is available._


# Dynamic Parsing #
During the parsing passes, the compiler actually updates its own grammar to facilitate type checking and semantic error detection. This process is typically referred to as _dynamic parsing_. Because of the cost of updating the grammar and reparsing the code in each pass, this leads to potentially very long compile times. We are actively exploring improvements to this model.

Dynamic parsing is used in two situations:
  * Standard library entity registration pre-compilation
  * Type registration during compilation

### Standard library usage of dynamic grammar modifications ###
The standard library implementation modifies the base Epoch grammar at load time. This enables the grammar to remain generic and simple, while still providing strict compile-time safety checks to ensure syntactic and semantic errors are avoided as much as possible.

In addition to registering functions, the standard library registers [code entities](http://code.google.com/p/epoch-language/wiki/EntitySystem) such as flow control constructs. The grammar is modified to recognize these constructs and treat them with special semantic significance. This must be done prior to the metadata prepass to ensure that parsing can succeed in the presence of custom entities.

### Type registration during the prepass ###
When a structure definition is encountered, the name of the structure type being defined is added to the grammar. This enables the compiler to validate types during later passes using the parser rules instead of more complex semantic checks; code that uses an undefined type will literally fail to _parse_ rather than requiring that the compiler explicitly detect and throw a semantic error later.

> _**Optimization Note:** This process introduces some inconsistencies in the define-once model of Epoch compilation. For example, some structures must be defined "above" any usage of those structure types in the program, particularly where nested structures are involved. This inconsistency is regrettable and we are actively exploring ways to eliminate it._

## Implementation of Dynamic Parsing ##
In the current Epoch compiler (as of Release 11), parsing is provided by the Boost Spirit library (`boost::spirit::classic`). Spirit exposes a special grammar rule type known as `stored_rule<>` which has certain semantics that allow it to be modified on the fly at runtime.

As rules are added to the grammar, they are fed into these `stored_rule` objects to extend the parser's understanding of the code. An example can be found in `Grammars.h` in the `AddVariableType` function, which handles the case of structure type definitions described above:

```
void AddVariableType(const std::string& identifier)
{
	VariableType = VariableType.copy() | boost::spirit::classic::strlit<>(identifier.c_str());
}
```

## Limitations of Dynamic Parsing ##
Currently, extensions to the grammar are limited to two mechanisms: standard library implementations, and structure type definitions. As of Release 11 there is no means by which the grammar can be modified from within Epoch code directly; standard library code (typically implemented in C++) can utilize this functionality, but Epoch programs themselves cannot mutate the grammar.

We are planning on extending this in the future so that Epoch programs can extend their own syntax. One proposed example for how this could operate looks something like this:

```
//
// Epoch meta code demonstration
//
entity forrange :
  (variablescope(scope), codeblock(code), identifier(iterator), integer(start), integer(end))
    ->
  ()
{
    scope.AddVariable(integer, iterator)
    scope.SetValueByName(iterator, start)
    while(scope.GetValueByName(iterator) < end)
    {
        executeblock(code)
        scope.SetValueByName(iterator, scope.GetValueByName(iterator) + 1)
    }
}

entrypoint : () -> ()
{
    forrange(i, 0, 10)
    {
        debugwritestring("Counting!" ; cast(string, i))
    }
}
```

This would obviously require extensions to the language including an object model, homoiconic code representations, scope reflection, and so on. Clearly this is an extensive project and will require substantial additional investment in the language before it is viable, which is why the current limitations on syntax extensions stand, as of Release 11.
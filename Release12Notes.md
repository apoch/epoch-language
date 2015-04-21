# Release 12 #
This release of the Epoch language marks a substantial update to the compilation model as well as a significant overhaul of core syntactical features.

On the compilation side, a proper treatment of the new compiler is provided on the page [Compilation Model Release 12](http://code.google.com/p/epoch-language/wiki/CompilationModelRelease12).

For syntax, the most noteworthy changes are as follows:
  * Function syntax is now greatly cleaned up
  * Structure definition syntax is similarly simplified
  * Variable definitions now follow a new, clearer form

As an example of the new syntactical forms, Release 12 Epoch looks like this:

```
fib : 0 -> 1
fib : 1 -> 1
fib : integer n -> integer f = fib(n - 1) + fib(n - 2)

deep_thought : -> integer answer = 0
{
    integer a = fib(5)
    integer b = fib(8)
    answer = a + b
}

entrypoint :
{
    integer the_answer = deep_thought()
    print(cast(string, the_answer))
}
```

This release focuses primarily on the compiler rewrite and optimizations; no significant language features have been added or extended in this release.

## Compiler Test Suite ##
A complete compiler test suite for this release is included in the source package. This suite is designed both to showcase various language features and to act as a regression test for compiler changes; by running the programs in this suite after all modifications to the compiler, we can ensure that changes do not cause breakage of unrelated features. This marks the first release in which Epoch has been fully testable - and passes all of its tests.

## LLVM Integration ##
Release 12 also includes a very limited prototype of LLVM integration for JIT-compiling Epoch programs. Please note that this is still highly experimental and supports only a very small subset of the language. An example can be found [here](http://code.google.com/p/epoch-language/source/browse/JIT%20Tests/jit.epoch?repo=examples). Further work on JIT-compilation to native x86 code is planned for subsequent releases.
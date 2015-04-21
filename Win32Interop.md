# Win32 Interop and Epoch #

Epoch provides rich support for interacting with Win32 APIs, and indeed any C-style API using the `cdecl` or `stdcall` calling conventions over standard DLL exports in Windows.

This includes the ability to call out to external functions, as well as provide callbacks which external code can use to invoke Epoch functions.


## Externals ##

External functions are accessed in Epoch using _function tagging_. A function tag is enclosed in square brackets and follows the function parameter and return value definitions, but precedes the (optional) code body.

```
MessageBox : integer hwnd, string message, string caption, integer style -> integer ret = 0
[external("User32.dll", "MessageBoxW", "stdcall")]
```

Here we define a function `MessageBox` which accepts several parameters and returns an `integer`. The critical element is the function tag, which specifies that the function's code body lives in `"User32.dll"` under the name `"MessageBoxW"`, to be invoked using the `stdcall` calling convention.

The Epoch runtime uses this tag to re-route calls to `MessageBox` into a _marshaling system_, which translates variables and data in Epoch format into a form that is understood by the underlying external API.

Note that it is possible to attach your own code to a function which is tagged as `external`. In this case, your code will run _after_ the invocation of the external, allowing for wrapping of error-handling and other special logic around external APIs without requiring multiple functions.


## Callbacks ##
When a function reference is passed to an external function, either as a direct parameter or as a member of a structure, Epoch automatically generates code which allows the external API to call back into the referenced Epoch function.

An example of this is the common enumeration mechanism found in the Windows API:
```
//
// CALLBACK.EPOCH
//
// Compiler test for marshaling between external functions and Epoch callbacks
//


EnumDesktopWindows : integer desktop, (callback : integer, integer -> boolean), integer lparam -> boolean ret = false
[external("User32.dll", "EnumDesktopWindows", "stdcall")]


entrypoint :
{
    EnumDesktopWindows(0, enumerator, 0)
}


enumerator : integer hwnd, integer lparam -> boolean ret = false
{
    passtest()
}
```

Standard function reference syntax is used here to indicate that `EnumDesktopWindows` accepts two parameters: one function reference (which is a function mapping two `integer`s onto a single return `boolean`) and one extra parameter which is handed off to the callbacks as user data.

When the Epoch system executes this code, it first generates a translation layer for _marshaling_ the data in and out of Epoch format. It then provides a C-style function pointer to this translation layer to the external function.

A similar process is used for function references which are members of a structure, allowing the use of standard Win32 structures which contain callback pointers, such as those used for defining window classes.


## Guidelines ##

A few common guidelines will make interaction with external APIs much simpler:

  * External APIs must conform to the `stdcall` or `cdecl` calling conventions
  * Specify `stdcall` as the third function tag parameter unless `cdecl` is in use (in which case the parameter may be omitted)
  * C++ style name mangling is not supported, so use `extern "C"` as appropriate when writing code that must be called from Epoch
  * Opaque pointer types such as `HWND` should be treated as Epoch `integer`s wherever possible, or `integer16`
  * Use `string` types for immutable strings, such as LPCTSTR
  * Use `buffer` types for mutable strings, such as LPTSTR or WCHAR[.md](.md)
  * Epoch is UTF-16 little-endian by default; as such, when calling applicable Windows APIs, call the W version of the function, such as MessageBoxW
  * Epoch function names do not have to correspond to the external function's name, provided that the external function's name is specified correctly in the function tag. This allows the omission of the trailing W specifier for Unicode-based Windows APIs, for example, which can lead to slightly more readable code without relying on overloads or preprocessor macros
  * Epoch does not use escape characters in strings by default. To use special escape sequences like `\n` or `\0` in a string, pass the string to the library function `unescape` prior to passing the string into an external API
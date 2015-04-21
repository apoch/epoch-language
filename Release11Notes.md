# Release 11 Notes #

**Major fixes and improvements in Release 11, in no particular order**
  * Garbage collector implemented
  * Win32 function calls reimplemented
  * Win32 callbacks into Epoch code reimplemented
  * Structure support (aggregate types) reimplemented
  * Buffers now have value semantics (versus old reference semantics)
  * Reference parameters to functions implemented
  * Structures now have value semantics (versus old reference semantics)
  * Handle recycling improves longevity of Epoch programs
  * Function labels (e.g. "external") are now saved to bytecode correctly
  * Structures may now contain function references
  * sizeof() function implemented
  * integer16 support reimplemented
  * Externally marshaled functions may use return variables with any name
  * Structure/buffer allocation and access is now thread-safe
  * Anonymous temporary objects can now be constructed
  * Constructors for structures may now be overloaded
  * Improved parser exception handling
  * Added support for hex (base 16) literals
  * Added direct returns of arbitrary expressions (anonymous returns)
  * Numerous fixes to .EXE generation
  * Function references as structure members are validated at compile time
  * Fixed semantics of nested structures
  * Reimplemented higher order functions
  * Reimplemented buffer data type
  * Improved status messages from EpochTools during compilation
  * Bugfixes to pattern matching
  * Implemented Scintilla lexer support for Epoch code
  * Improved exception handling overall (better exception types)
  * Improved error recovery in the compiler
  * Improved consistency of relative path handling in Epoch project files
  * Fixed a bug involving parenthetical expressions
  * Major work on the Era IDE implementation
  * Assorted code cleanup and documentation improvements

As work continues apace on the Epoch language, it is often difficult to maintain an exact catalog of all the changes, fixes, and improvements made; therefore, the above list may not be comprehensive. In particular many interim changes between Release 10 and Release 11 may not be noted, including fixes to transitional code between versions. As such, any defects discovered during use of Release 10 or subsequent development preview versions from the Mercurial repository may have been addressed, and should be checked again in Release 11.

Please feel free to file issues against Release 11 as they are discovered.
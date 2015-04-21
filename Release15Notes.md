# Epoch Language - Release 15 #
Published 2014-10-06, Release 15 of the Epoch programming language is the first fully self-hosted implementation of the language. The release includes the Era IDE prototype.

  * [Download the Installer](https://docs.google.com/uc?export=download&id=0BxOmwjGzQxEWMW1fdE1pZVZkZGM)


---


  * [Introduction to Epoch](https://code.google.com/p/epoch-language/wiki/Introduction)
  * [Newcomer's Guide](https://code.google.com/p/epoch-language/wiki/NewcomersGuide)
  * [Programming Guide](https://code.google.com/p/epoch-language/wiki/ProgrammingGuide)


## Highlights ##

  * Self-hosted compiler written entirely in Epoch
  * Countless bugfixes and language feature additions/improvements
  * Preview edition of the development environment (Era)
  * Includes 90 compiler tests and several example programs
  * Full source for Era and the compiler are included
  * Runtime library (implemented in C++) and source included

## Inside the Package ##

The installer will deploy the Epoch binaries to a location of your choosing. Optionally, the installer can deploy the compiler test suite, source code for the compiler and Era, and source code for the runtime library/platform.

### Files ###

  * `EpochCompiler.exe` - the self-hosting Epoch compiler
  * `EpochLexer.dll` - syntax highlighting linkage for the Scintilla editor
  * `EpochLibrary.dll` - runtime library implementation
  * `EpochRuntime.dll` - runtime platform (JIT) implementation
  * `Era.exe` - preview edition of the Era IDE
  * `License.txt` - Epoch language license terms
  * `LLVM.txt` - LLVM license terms
  * `Readme.txt` - link to this site
  * `SciLexer.dll` - Scintilla editing widget
  * `Scintilla.txt` - Scintilla license terms

### Optional Folders ###

  * `Epoch Programs` - compiler and Era source, plus the test suite
  * `Runtime Platform Source` - source code for the runtime DLLs for Epoch

## Building the Compiler ##

The easiest way to build the compiler is to locate the file `Compiler.eprj` in `Epoch Programs\Projects\Development Tools\Compiler` and open it (it should be associated with Era by default after running the installer). From the Project menu in Era, select Build Project.

## Building the Runtime Platform Code ##

The runtime platform code is a bit tricky. It is currently targeted at Visual Studio 2010. Additionally, there are some dependencies on external code, namely the LLVM project.

Release 15 is dependent on LLVM 3.3. To build the runtime platform from source, install LLVM 3.3. Before building LLVM, apply the patch located in `Runtime Platform Source` (just copying the files into your LLVM source location should suffice).

After building LLVM, rename the `.lib` files it generates to the pattern `DebugFoo.lib` and/or `ReleaseFoo.lib` depending on the configuration you built. These library files will need to be reachable for the runtime platform code to build.

Open `Epoch2010.sln` in Visual Studio, select the appropriate build configuration, and build the solution. If all goes well, this should produce working binaries for the runtime.

Please note that Debug builds are _incredibly_ slow. We strongly recommend building Release unless you really need to use Debug.
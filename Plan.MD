# Epoch Language Plans
This file is a running stream-of-consciousness log of plans and thoughts surrounding the Epoch programming language. It is not intended to be an authoritative or exhaustive resource; it exists primarily for the curious and for recording various ideas.

### MikeL 2016-08-20
Debug info generation has reached a level where the Epoch compiler is directly emitting stub `PDB` files for any built binary. This is a rough but working proof-of-concept. A vast amount of work still needs to go into making the debug files do the right things, with the right data from the compiler front-end; but the foundations are down and the rest is just iteration and polish. The heavy R&D is mostly over - type metadata being the main exception.

In the process of converting LLVM's emitted CodeView buffers to data suitable for embedding in a PDB, I realized that there's basically no good byte-wise extraction mechanism in Epoch right now. That is to say, given a `buffer` of N bytes, I can't easily turn, say, bytes 30-34 into an `integer` and operate on it.

I'm loath to do genuine pointer arithmetic muppetry in Epoch, at least right now; but the other extreme end (writing a support DLL that has an API for extracting data from `buffer`s) isn't all that appealing either. So I'm going to sit down and think about array support and how to make it integrate elegantly with byte-stream value extraction.

That'll be my new "weekend project" I suspect!


### MikeL 2016-07-15
After much tinkering I did manage to coax WinDbg into listing sane callstacks, but Visual Studio remains elusive. I've determined that the main problem I have is the lack of control over how `link.exe` emits PE sections. Since I can't coerce the linker into emitting the same section layout as `EpochNativeBin.exe` does when it links an `.exe` file, the debug data naturally mismatches pretty badly.

In lieu of mucking with `link` much more, I've started looking at a project called `cv2pdb` which originated with the D language toolchain. It uses an intriguing pattern of calling directly into `MSPDB140.dll` (or the appropriate version for some given install of Visual Studio) to emit a PDB file. Most of the logic looks unnecessary to me but I can extract the basic interfacing with the PDB writer DLL to generate my own PDBs without having to binary-create them from scratch.

So far I have the PDB containing nothing, but it at least loads and validates in WinDbg and `DBH.exe` which is a great start. I'll likely sink much of the upcoming weekend into making a more useful implementation of this functionality.
  
  
### MikeL 2016-07-11
Recently I've been putting most of my effort into GC implementation overall, although in the small I've been distracted by a few interesting issues related to debugging. I'm growing really painfully tired of not having a good debug experience for Epoch. With the emission of exception handling metadata in 64-bit executables, we're finally one step closer to having debugger support. Callstacks are now correct and readable... assuming you know the emitted addresses of every function in the program.

Obviously that's a non-starter, so I've been looking into hacky ways to generate PDB symbols for emitted Epoch binaries so that I can use WinDbg or even Visual Studio to debug Epoch code.

My current approach is to emit a COFF object file alongside the generated EXE that contains the CodeView debug metadata emitted by LLVM, along with a symbol and string table. I then run this through `link.exe` to generate the PDB, and discard the binary that `link` produces, in favor of the Epoch binary. This is *sort of* functional at this point; the PDB contains enough data for debuggers to recognize the loaded image and that it contains code, but there's some kind of mismatched data in the linker because the PDB doesn't contain any definitive symbols.

I think what I might try next is emitting a COFF object with the full contents of the Epoch .EXE and trying to get a runnable binary from `link`. If *that* binary and PDB are fully debuggable, then I have a starting point for whittling down the process and getting a minimal set of data to feed `link` to convince it to give me a usable PDB.

On a tangential note, it's a profoundly weird experience to have your debugger (WinDbg) crash inside another debugger (VS) while you're trying to write a debugger (Epoch)!

Of course this would be much simpler if the PDB format was documented or at least accessible through a documented API... alas. Someday I'll try and get debug output without reliance on `link.exe` but that's a ways from now. I'm fine with running a manual, VS-dependent step to generate debug info (for the moment).
  
  
### MikeL 2016-07-06
I'd like to quickly address the general question of overall plan for the language. I see the project progressing in roughly the following order (more or less) over the next few months/years/etc.:

 * Finish GC implementation for strings on Windows x64
 * Assess what's left for getting self-hosting on x64
 * Build out functionality needed to self-host
 * Flesh out test suites as necessary
 * Self-host on x64
 * Take time to analyze state of the project
 * Probably tackle arrays around this timeframe
 * Massive cleanup, documentation, and test-hardening passes
 * Introduction of tasks
 * Explore BGFX integration further during this time
 * Refactor Era to use new language features, x64 port
 * Continuing development on Era
 * Debugging infrastructure work
 * Version 0.1?

Each bullet point represents anywhere from a few weeks to a couple of months worth of work. Obviously this is not slowing down any time soon, and there's plenty left to be done. However, I'm fairly confident that this is the right direction for things and that with sufficient investment the final candidate for 0.1 will land gracefully.
  
  
### MikeL 2016-07-05
Garbage collection of strings is now loosely prototyped and running on Windows x64 binaries. There are still many challenges to solve, such as marking intermediate values in expressions as GC roots, and interacting with callstacks that contain external/non-Epoch code. However, given that I had reliable solutions for these in previous compilers on 32-bit Windows, I'm confident that I can rebuild the GC implementation fairly soon.

There is a strong chance that I will be exploring more of the resource allocation mechanisms of Epoch during this time. I've gained a renewed interest in supporting optional GC as a first-class principle of the language - i.e. it should be possible to totally opt-out of the GC and write manually resource-controlled programs without much extra pain. I also want to have first-class support for things like stack allocators, pool allocators, freelists, etc. etc. although most of that will have to wait until after array support lands.

Speaking of arrays... I need to think up a good syntactical approach to expressing "array of type T". I also have to decide if I want to build fixed-size arrays or go hog wild and support resizing in native array objects. It's looking like the former for simplicity and elegance; we can always build standard library containers atop fixed arrays that support growth patterns. It might even be nice to allow programmers to supply growth policies for their "vector" containers.


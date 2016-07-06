# Epoch Language Plans
This file is a running stream-of-consciousness log of plans and thoughts surrounding the Epoch programming language. It is not intended to be an authoritative or exhaustive resource; it exists primarily for the curious and for recording various ideas.

## MikeL 2016-07-05
Garbage collection of strings is now loosely prototyped and running on Windows x64 binaries. There are still many challenges to solve, such as marking intermediate values in expressions as GC roots, and interacting with callstacks that contain external/non-Epoch code. However, given that I had reliable solutions for these in previous compilers on 32-bit Windows, I'm confident that I can rebuild the GC implementation fairly soon.

There is a strong chance that I will be exploring more of the resource allocation mechanisms of Epoch during this time. I've gained a renewed interest in supporting optional GC as a first-class principle of the language - i.e. it should be possible to totally opt-out of the GC and write manually resource-controlled programs without much extra pain. I also want to have first-class support for things like stack allocators, pool allocators, freelists, etc. etc. although most of that will have to wait until after array support lands.

Speaking of arrays... I need to think up a good syntactical approach to expressing "array of type T". I also have to decide if I want to build fixed-size arrays or go hog wild and support resizing in native array objects. It's looking like the former for simplicity and elegance; we can always build standard library containers atop fixed arrays that support growth patterns. It might even be nice to allow programmers to supply growth policies for their "vector" containers.


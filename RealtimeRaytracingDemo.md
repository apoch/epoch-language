# Background #

As part of the ongoing work on Release 14, we have been optimizing the Epoch language runtime model in a number of ways:

  * Instruction set revisions to take advantage of static data (minimizing runtime computations for things like stack operations, structure accesses, etc.)
  * Improved memory allocation model for virtual machine stack frames and metadata
  * Minimization of dynamic memory allocations during type decomposition and pattern matching
  * Transitioning away from exclusive virtual machine execution towards native code generation

While the VM optimizations have provided substantial runtime performance gains, the most promising avenue has been moving towards native code where possible.

As of this writing, a large subset of the Epoch language can be Just In Time compiled to native machine code. This process is currently performed by the VM framework when an Epoch image is loaded from disk, just prior to execution. JIT compilation is performed on the VM bytecode and assisted by the embedded metadata produced by the Epoch compiler.

We have selected [LLVM](http://llvm.org/) as our JIT/native code framework of choice. LLVM has proven to be sufficiently mature to act as the foundation of the language runtime and offers a rich feature set that will enable the full range of planned functionality for Epoch going forward.

Our current plan is to phase out the Epoch VM over time, beginning in Release 14 by heavily moving towards JITted native code wherever possible.


# Measuring Progress #

As with any performance-related work, measuring is central to success. We have elected to build a simple raytracer as our demonstration app for the Release 14 optimizations.

This decision was motivated by several factors:

  * Raytracing is fairly arithmetic-intensive and provides a good benchmark for the raw computational power of the language
  * Building an elegant raytracer capitalizes on a number of Epoch's language features
  * A very significant subset of the language and execution model can be tested
  * Since so much of the language is exercised by the demo, it provides a good target for considering the JIT native code generator mostly feature-complete
  * And of course a visual demo is much more compelling than an abstract list of performance numbers

On our reference hardware, the first pass of the raytracer demo generated a 300x300 pixel image of a single sphere lit by a lone point light source. This image averaged roughly **8 seconds** to generate.


# Virtual Machine Optimizations #

Heavy work on the VM yielded a roughly six-fold performance increase on the reference project. However, this was still considered far too slow for practical applications. Based on the investment required to continue improving the VM versus transitioning to native code, it was clear that the VM would eventually need to be phased out entirely in favor of native JIT compilation.


# Native Code Generation with LLVM #

Release 12 already featured a rudimentary integration with LLVM that supported a trivial subset of the Epoch language. For Release 14, we have generalized and expanded this integration substantially. As of this writing, roughly 85% of the Epoch language feature set is supported by the new JIT implementation.

A large amount of work remains before the JIT implementation is considered "clean" and up to the Epoch language project code standards; however, the initial results have been remarkable.


# Realtime Raytracing in Epoch #

With the native code generation process largely complete, we decided to publish the raytracing tech demo as a commemorative milestone of our current progress.

The final demo can be found [on our Downloads section](https://code.google.com/p/epoch-language/downloads/detail?name=RealtimeRaytracer.zip). This is a complete .ZIP archive containing the demo program and the current trunk build of the VM support infrastructure (Windows only).

The demo features a blue sphere, illuminated by a point light source that moves along with the mouse cursor on the screen. On the reference hardware, this demo runs at roughly 17 frames per second. Keep in mind the initial render time for a single frame is 8 seconds using the Release 13 runtime model.


# Future Work #

A number of major features must still be transitioned into the native execution model. Most notably, garbage collection is still managed entirely by the VM framework. This leads to visible pauses in the raytracer demo as the garbage collector runs, for example.

In general, memory allocation and management in the current trunk is bound heavily to the VM. We intend to move towards a more efficient approach while retaining the implementation of precise garbage collection. Research into options for accomplishing this is ongoing.

Besides garbage collector pauses, a major implication of the current dynamic memory model is that all dynamic memory accesses require extra layers of indirection through the VM. This slows down the execution of programs which rely heavily on dynamically allocated data, such as the raytracer demo. This accounts for virtually all of the remaining performance gap between the pre-Release 14 runtime and other native languages.

Due to the scope and magnitude of work involved in removing the VM overhead from the Epoch runtime model, it is unlikely that this will be fully complete for Release 14. However, as noted above, we intend to continue with this transition over time, and ideally within a few releases the VM will be all but gone.


# Conclusion #

The final frontier of heavy feature development for Epoch remains parallel processing. We are planning on a shared-nothing green-threads approach; this will allow us to continue with the current JIT model while minimizing inter-thread communication overheads. (For example, in the current VM model, removing thread synchronization from memory accesses can literally double runtime performance.)

As development progresses, we will continue to release these demos and updates. Thanks for following along, and stay tuned for more exciting progress from the Epoch Language Project!
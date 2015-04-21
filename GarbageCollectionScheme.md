# Garbage Collection on LLVM #

As of Release 14 of the Epoch language, we have converted to a native machine code execution model, enabled by JIT compilation through [LLVM](http://llvm.org/).

One of the principal challenges involved in running Epoch as native code centers around garbage collection. Epoch offers precise garbage collection for strings, byte buffers, and aggregate-typed objects (structures). This page provides an in-depth review of the techniques used to accomplish precise GC on top of LLVM.

## Strategy ##

Our approach to garbage collection in Epoch involves a relatively simple mark/sweep collector which operates in the context of individual _tasks_ (aka "green threads"). Since tasks cannot share state, this radically limits the amount of heap space (and stack roots) that must be traversed during the mark phase. This also eliminates the need for a concurrent collector since each green thread can collect its own garbage independent of any other task.

As of Release 14 and subsequent work on the garbage collector, full support for tasks is not yet implemented; however, the GC is completely operational for single-threaded programs at this time.


## LLVM Integration ##

A crucial limitation of LLVM's GC support at this time is that values stored in machine registers cannot be marked as potential GC roots. Epoch uses a mark/sweep collector, and knowing the complete set of live roots is essential.

Two options exist for dealing with this limitation: shadow stacks, and register spilling. We opted to use register spilling as it involves significantly less runtime overhead than shadow stack maintenance, and runtime performance is an important goal for the language.

LLVM does support generating stack maps for GC roots, so it is entirely possible to implement a fully functional GC using stack maps via LLVM. However, accessing the stack maps can be difficult in a JIT environment. As of LLVM 3.3, the only proper support for reading stack maps requires emitting assembly language dumps instead of machine code. This represents an unacceptable overhead for our purposes.

To solve this problem, we implemented a custom `GCStrategy` with custom root handling.

The crucial element is a variable known as the `FunctionList` container, which is a `std::vector<GCFunctionInfo*>` that holds references to all garbage-collection-enabled functions in the emitted machine code. `GCFunctionInfo` encapsulates the stack map and "safe point" details for each function. Since we do JIT compilation at load time of the Epoch program, this container can be handed off to the runtime environment for garbage collection purposes.

Once the stack map is accessible to the garbage collector, we can crawl the machine stack for live roots. This is a relatively simple process with one nasty wrinkle: the stack map we have access to does not contain the actual in-memory locations of the safe points we emitted during JIT compilation.

The upshot of this problem is that it is impossible to know (using vanilla LLVM) _which_ stack map data to look at, because we don't know which safe point we are at when the collector is invoked. To solve this, we introduced a surgical change to LLVM.

Specifically, we modified the `ExecutionEngine` interface to provide access to the `JITCodeEmitter` which knows the location of `MCSymbol` labels emitted during machine code generation. This is a single-function modification and is relatively hackish; we are actively considering submitting a patch to the LLVM project with a more robust approach to solving this visibility issue.


## Pitfalls ##

There are several areas where implementing a robust GC on top of LLVM can be tricky. One such issue is correlating stack map data to safe points in JITted code, covered above. Other pitfalls encountered during the project are enumerated below for the benefit of anyone else who wishes to tackle GC on LLVM.

### Stack Map Offsets ###

The offsets provided in the stack map have two different meanings depending on the sign of the offset (specifically referencing `llvm::GCRoot` and the `StackOffset` member). If the offset is _positive_, the actual stack location of the root must be calculated as follows:

```
callee frame pointer + size of return address pointer + size of frame pointer + StackOffset
```

However, if the offset is _negative_, it is relative to the _other_ end of the stack frame, and can simply be added to the current frame's frame pointer to obtain the correct memory location.

### Frame Pointer Omission ###

This common compiler optimization makes stack walking very difficult. There are two basic approaches to precise GC in the presence of the FPO optimization. One involves maintaining metadata which allows the runtime environment to track stack roots without the use of explicit frame pointers and stack maps; this is impractical for Epoch's purposes. It also is not resilient in the face of interop with external libraries which may be compiled with FPO.

We opted therefore to use a different approach. Essentially, we do not use FPO for any Epoch JITted code. However, when invoking external functions, we leave a "bookmark" in a special heap container. If the external function calls back into Epoch code, and the garbage collector is invoked during the callback, we can use the bookmark to retrieve the location of the last valid Epoch stack frame. This allows the runtime to recover roots that might otherwise have been missed due to the use of FPO.

### LLVM lifetime intrinsics ###

LLVM includes two intrinsicts, llvm.lifetime.start and llvm.lifetime.end, which are used for (among potentially other things) stack coloration. This optimization involves noting when two variables cannot possibly be in use at the same time, and collapsing them into a single memory location on the machine stack.

Unfortunately, this optimization has some issues in the presence of GC roots. It is possible, for instance, for a GC root to be merged into a non-GC-related variable's stack slot. If the GC is invoked while this situation is active, the GC will trace garbage as if it were a valid stack root.

The interim solution to this involves a custom lowering pass in the `GCStrategy` used by the Epoch runtime; this lowering pass eliminates all calls to the lifetime intrinsics. This costs a small amount of extra stack space, but we feel that this is a reasonable tradeoff for getting a working garbage collector!

An [LLVM bug](http://llvm.org/bugs/show_bug.cgi?id=16778) has been filed detailing this situation and is pending investigation.

## Optimizations ##

There are several improvements that can be made to the existing implementation of the Epoch garbage collector.

Most notably:

  * Improve heap tracing algorithm for performance (current traversal is highly naive)
  * Cache a lookup of return addresses to safe point metadata to avoid an expensive search of every function and every safe point for every stack frame.
  * Provide tuning controls to programmers to regulate how often GC passes are run
  * Implement optional GC on a per-object basis, i.e. allow programmers to "opt out" of garbage collection for specific objects

## Complete Implementation ##

The current implementation of the garbage collector infrastructure can be found [here](https://code.google.com/p/epoch-language/source/browse/EpochRuntime/JIT/GarbageCollection.cpp).
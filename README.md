# The Epoch Programming Language
Welcome to the home of **Epoch**, an opinionated programming language designed to balance speed, control, and productivity.

Epoch places a strong emphasis on certain ways of thinking about programming, at both large and small scales. The decisions behind the language will undoubtedly not please everyone - but if you're a like-minded programmer, we hope Epoch scratches your itch as well as it has scratched our own.

## Where It Stands
 * Procedural, structured foundations with some generic programming support
 * Strong, statically-checked type system
   * Built-in integral, floating-point, Boolean, and string types
   * Product types (Structures)
   * Algebraic sum types (Discriminated Unions)
   * Type aliases with control over conversion rules
   * Zero implicit type conversions, ever
   * No nullable types! Prefer sum types with `nothing` as a member
   * Value-semantics by default, can opt-in to reference semantics
   * Static function dispatch by argument type (overloads)
   * Dynamic function dispatch by type or value (virtual calls, pattern matching)
 * Ahead-of-time compilation to native machine code (Work In Progress)
   * Uses LLVM to gain access to a huge, mature suite of optimizations
   * Carefully written Epoch is competitive with C++ in synthetic benchmarks
 * Optional garbage collection for memory management
 * Native 64-bit support on Windows (Work In Progress)

## What's Coming
 * First-class lexical closures
 * Protocol-based polymorphic dispatch
 * Tasks model for enabling parallelism and distributed processing
 * Deterministic resource control model (think RAII)
 * Fully native integration with garbage collector engine
 * Broader platform support
 * Full custom IDE with highlighting, syntax completion, integrated debugger, profiler, and more


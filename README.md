
> Please, give us tools that are a pleasure to use, especially for our routine assignments, instead of providing something we have to fight with. Please, give us tools that encourage us to write better programs, by enhancing our pleasure when we do so.
>
>_-- Donald Knuth, Communications of the ACM, December 1974_


# The Epoch Programming Language
Welcome to the home of **Epoch**, an opinionated programming language designed to balance speed, control, and productivity.

Epoch places a strong emphasis on certain ways of thinking about programming, at both large and small scales. The decisions behind the language will undoubtedly not please everyone - but if you're a like-minded programmer, we hope Epoch scratches your itch as well as it has scratched our own.

## Hello World
Here's a simple example of what Epoch programs look like. This is a slightly over-engineered version of Hello World that illustrates some common type system idioms from Epoch:

    //
    // A simple Epoch program
    //
    
    // Declare an algebraic sum type
    type OptionalString : string | nothing
    
    // Define a function
    Display : string optstr
    {
        print(optstr)
    }
    
    // Overload the function
    Display : nothing
    {
        print("End of line.")
    }
    
    // Entry point function
    entrypoint :
    {
        OptionalString hello = "Hello, world!"
        OptionalString blank = nothing
        
        Display(hello)
        Display(blank)
    }

   
## What It Looks Like
See the [Example Epoch Snippets](https://github.com/apoch/epoch-language/wiki/Example-Snippets) page for a showcase of small Epoch programs doing routine things.

## What's Different
* Leave behind nullable types - and gain freedom from null pointer/null reference problems
* Leave behind uninitialized state - and gain assurance that *all* data is always initialized *by the programmer* to sane values
* Leave behind implicit type conversions - and gain peace of mind that a `real` is always `real`. Really
* Leave behind archaic parser/grammar minefields - and gain the knowledge that `++i++` will fail to compile instead of invoking undefined behavior at runtime
* Leave behind terminating semicolons - and spare your pinky finger all that RSI
* Leave behind convoluted syntax rules - and gain a little bit more precious sanity when you never have to deal with a "most vexing parse" again
* Leave behind primitive build models - and gain a linker-free, single-pass compilation model with full program optimization by default
* Leave behind rigid class hierarchies - and gain flexible, minimally coupled, and easily reusable architectures using composition of task pipelines
* Leave behind inheritance - and gain better composition of your reusable components. If your task conforms to a protocol, anything speaking that protocol can interact with that task. Compose multiple protocols to build arbitrarily rich tasks
* Leave behind explicit threading - and gain green-thread task-model programming that can scale arbitrarily, even across multiple host machines

## What's The Same
* Don't lose the tools you know - Visual Studio 2015 integration is Work In Progress
* Don't lose your APIs - Epoch supports invoking any C-ABI functions exported from DLLs
* Don't lose your target platform - Windows 64-bit is natively supported by Epoch
* Don't lose performance - Epoch uses cutting-edge LLVM optimization passes to ensure maximal speed
* Don't lose your memories - garbage collection can make many pieces of software simpler and easier to write
* Don't lose control - garbage collection is fully optional per-task and can be tuned (or shut off) for best performance

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
* Visual Studio 2015 integration (Work In Progress)
   * Create and edit Epoch code with syntax highlighting
   * Debug and profile Epoch programs (Work In Progress)

## What's Coming
* First-class lexical closures
* Protocol-based polymorphic dispatch
* Tasks model for enabling parallelism and distributed processing
* Deterministic resource control model (think RAII)
* Fully native integration with garbage collector engine
* Broader platform support
* Exporting Epoch code from DLLs


# The Wiki
More documentation is available on [the Wiki](https://github.com/apoch/epoch-language/wiki).

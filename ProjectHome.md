# Epoch Language Project - Programming for the Next Era #
Every seasoned programmer knows that building great software involves making some compromises.<br />
_Choosing a language doesn't need to be one of them._

### Epoch in a Nutshell ###
  * Strongly, statically typed with pervasive type inference for succinctness
  * Compiles to native code for maximum performance
  * Garbage collection when you want it, manual memory management when you need it
  * Green threads/fibers with rich built-in message passing model for parallel processing

### Why Another Language? ###
Put simply, Epoch is very opinionated. The language is designed from the ground up with an emphasis on certain mindsets and approaches to building software systems and applications. These principles may not appeal to everyone, but if you're a like-minded developer, we hope Epoch will scratch your itch as well as it scratches our own.

### The Philosophy ###
  * Programming shouldn't suck. This underlies all other principles of the language.
  * Make it easy to do the right thing, and hard to do the wrong thing.
  * Get out of the programmer's way as much as possible.
  * Repeating yourself and stating the obvious are both equally grave sins.
  * Conventional modes of thought only deserve to live insofar as they enhance productivity.
  * Provide escape hatches to permit things that are gross but necessary.

[See the Introduction for more on the language philosophy and opinions.](https://code.google.com/p/epoch-language/wiki/Introduction)


### What Does It Look Like? ###
```
// A simple program in Epoch

structure DeepThought :        // Declare an aggregate type
    string Question,
    integer Answer

type Operation :
    DeepThought | nothing      // Declare an algebraic sum type (discriminated union)


Process : Operation ref op     // Implement a method with no return type
{
    if(Test(op))
    {
        print("Complete!")
    }
    else
    {
        print("Please wait 7 million years!")
    }
}

// Implement a function which maps a parameter on to a return value
Test : DeepThought ref op -> boolean success = true
{
    print("Given the question: " ; op.Question)
    print("We obtain the answer: " ; cast(string, op.Answer))
}

// Overload the function to handle all options of the sum type
Test : nothing -> false


// Specify the start of execution for the program
entrypoint :
{
    // Define some variables
    DeepThought dt = "Life? The universe? Everything?", 42
    Operation failure = nothing
    Operation success = dt

    // Call some functions
    Process(failure)
    Process(operation)
}
```

![![](http://wiki.epoch-language.googlecode.com/hg/images/EraIDESept2014Small.png)](http://wiki.epoch-language.googlecode.com/hg/images/EraIDESept2014.png)

### How Ready Is It? ###
Epoch is still undergoing heavy development. The core compiler achieved a successful self-hosting in December 2013. Having reached this critical milestone, we plan to sink considerable effort into developing an IDE and powerful tool chain to go along with the compiler.

Currently native code compilation is working (as of Release 14) and a significant number of language features are in place. Crucial remaining elements include parallelization support and implementation of a more robust standard library. Additionally there are numerous quality-of-life improvements planned for the future of the language.

In short: Epoch is not a production-ready language, but we're moving in that direction as rapidly as possible. We welcome feedback and suggestions on the course of the language, as well as new contributors to the project.


## Current Events ##

  * Release 15 is now available - the first fully self-hosted version of Epoch! [Download the installer](https://docs.google.com/uc?export=download&id=0BxOmwjGzQxEWMW1fdE1pZVZkZGM) or view the Release15Notes.

  * Self-hosting was achieved on the evening of December 15, 2013!

  * Follow [@ApochPiQ on Twitter](https://twitter.com/ApochPiQ) to get more language updates!

  * Release 14 is now available, featuring 100% native code generation and numerous bug fixes. [Download the installer](https://code.google.com/p/epoch-language/downloads/detail?name=EpochRelease14.exe&can=2&q=) or check out the [release notes](http://code.google.com/p/epoch-language/wiki/Release14Notes).

  * Preview the performance gains coming in Release 14 with our [Realtime Raytracing Demo](http://code.google.com/p/epoch-language/downloads/detail?name=RealtimeRaytracer.zip)!

  * Release 13 is officially available! Play with templates, a richer type system, and more!


## Epoch Language Releases ##

#### Latest Stable Release ####
Release 15 is the current stable release ([installer](https://docs.google.com/uc?export=download&id=0BxOmwjGzQxEWMW1fdE1pZVZkZGM)).

#### Development Previews ####
The current development fork of the Epoch project can be accessed via our [Mercurial repository](http://code.google.com/p/epoch-language/source/checkout). Please note that _all_ development preview code is subject to change, and is _not_ considered stable. Explore at your own risk!
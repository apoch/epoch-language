# Self-Hosting for Epoch #

Beginning with Release 15, Epoch is increasingly moving away from a C++ codebase and investing heavily in writing the language tools in Epoch itself. With the shipping of Release 14, the compiler and runtime were deemed fast and feature-rich enough to explore the possibilities of self-hosting Epoch completely.

Self-hosting was achieved on December 15, 2013. The Epoch compiler successfully built itself, and the resulting compiler was used to build a "second-order" copy of the compiler program; both first and second order self-hosted compilers fully passed the compiler test suite (with a couple of minor exceptions, which none of the compiler implementations at the time could pass cleanly).

The remainder of this page is a historical look at how self-hosting was accomplished for the Epoch language.


# Strategy #

A typical approach to self-hosting might look much like writing a new compiler from scratch: begin with lexical analysis, move on to parsing, construct an abstract syntax tree, decorate and transform the AST over one or more passes, then finally implement a back-end which emits some kind of executable code.

We have elected not to follow this pattern, for one primary reason: rewriting the compiler from scratch in Epoch leaves us at serious risk for omitting features for a potentially long while during the rewrite process. Instead, we aim to implement self-hosting by moving _backwards_, starting from the back-end of the compiler and moving towards the lexer/parser over time.

This ensures that at all times the language remains as feature-rich in the Epoch implementation as in the existing C++ implementation. It also allows us to utilize the existing compiler test suite to verify the self-hosted compiler at all phases of the conversion project.


# Milestones #

The conversion process is divided into several major milestones:

  * Bytecode stream generation _(completed April 2013)_
  * Bytecode emitter from the back-end _(completed April 2013)_
  * Code generation from IR _(completed July 2013)_
  * IR optimization and decoration passes _(completed December 2013)_
  * Semantic analysis, converting AST to the IR _(completed December 2013)_
  * AST generation from the parser _(completed December 2013)_
  * Parsing of code _(completed December 2013)_
  * Lexical analysis of code _(completed December 2013)_
  * Final bug fixing and self-hosting _(completed December 2013)_


# Comments #

Each milestone denotes a significant amount of implementation effort, although they are not necessarily evenly divided in terms of time required for completion (see Strategy section). More in-depth analysis of each milestone is offered below.


## Bytecode Stream Generation/Back-end Emission ##

> _**Note:** this milestone is complete. Comments are provided for retrospective information._

Required elements:
  * Byte buffer manipulation system (accomplished via `writebuffer` library API functions)
  * Plugin infrastructure for calling Epoch code from the compiler
  * Minor library support for bitwise operations and some other tidbits

Implementation challenges:
  * Buffers are not resizable in Epoch
  * Type system mismatches between C++ and Epoch can cause issues


## Code Generation from IR ##

> _**Note:** this milestone is complete. Comments are provided for retrospective information._

This is the first large-scale milestone in the conversion process. The lack of data structures built into Epoch will make this painful in all likelihood.

It is highly probable that many data structures will have to be implemented from scratch in the compiler in order to reasonably accomplish this milestone.

The principal challenge for this stage will be converting IR data structures in the C++ form into something Epoch can process. There are two main options for doing this:

  1. Implement an IR traversal system similar to that used by the AST which can shell out to the Epoch back-end
  1. Implement a conversion layer that transforms C++ IR into Epoch IR

The former requires expanding rather than contracting the size of the C++ codebase, and temporarily at that; once the remaining self-hosting milestones are hit, the traversal mechanism will be redundant. On the plus side, however, it may inform the design of the Epoch compiler somewhat.

The second option means building a lot of data structure support into the Epoch compiler first, and then installing hooks in the IR handling code to invoke the conversion layer. This may not be a problem, though, since we'll need those data structures for handling other parts of compilation in all probability.


### Update Apr. 27 2013 ###
We've elected to pipe a limited flavor of the IR into Epoch - only the IR metadata necessary for sufficiently generating code. The first trivial test program passed today under the new code generation framework. There is still a vast amount of work to be done to get code generation fully implemented in Epoch, but we're making noteworthy headway, and fixing a lot of bugs in the C++ implementation of the compiler along the way.


### Update May 5 2013 ###
Nearly one third of the compiler test suite is now passing using the code generation layer written in Epoch. As it turns out, a fair bit of the semantic IR is being reimplemented in Epoch-flavored data structures along the way; the upshot of this is that work on code generation is a little slower but semantic validation of the IR and type inference should be mostly algorithmic implementations rather than data structure translation.

The bulk of remaining work for code generation lies in extending the existing IR support in the Epoch side, particularly for structure, template, and complex expression features.


### Update Jul. 14 2013 ###
Code generation is working well enough that the compiler has successfully generated a binary version of itself based on the IR constructed in C++. The next major phase of development will be to move IR construction and decoration into Epoch code.

As expected, the majority of work in this phase will be reimplementations of the type system algorithms and various other IR transformations/decorations. The plan is to use an AST traversal pass in the C++ code which calls into the Epoch compiler plugin to register each node of IR as the AST is walked. Once this IR is sufficiently represented, effort will shift towards decorating and transforming the IR in preparation for feeding it to the already-completed code generation layer.


### Update Sep. 18 2013 ###
All but nine of the compiler tests now pass with the end-to-end Epoch implementation of the compiler. Value-based pattern matching and templates are the major features left to be implemented, along with a few minor parser improvements surrounding preincrement-style operations and other syntactic sugar elements.

Once these aspects are completed, we will attempt a first pass at compiling the compiler with itself. Assuming all goes well, we aim to implement executable binary emission as a final step, and generate a working compiler binary. This should allow us to feed the compiler source through the compiler several times to validate its behavior.

Pattern matching is estimated to be a substantial feature but imminently solvable; template implementation may require quite a bit more time and effort since very little consideration for templates has been made in the existing compiler implementation.

Allowing time for refactoring and code cleanup to support pattern matching and templates, we still aim to have self-hosting achieved by the end of 2013.


### Update Oct. 12 2013 ###
Only three tests remain! All three are related to templatized data structures. Templatized functions are now largely working and require only minor additions and refinements to pass self-hosting requirements.

As might be expected, there have been many deficiencies in the code uncovered by all the testing and feature work, and indeed a number of "TO DO" comments are sprinkled throughout the compiler at the moment. We intend to develop additional tests to cover those weak points (and fix the compiler deficiencies, of course) prior to attempting the self-hosting process, as many of the gaps are related to functionality that will be stressed during large-scale compilations.

Ideally, structure templates should be completed in the next few weeks, and remaining cleanup and minor tasks can be tackled after that. Once the compiler code is passing the entire expanded test suite, we'll work on binary executable image emission as a last step, which should go quickly. Once stand-alone executables can be generated by the compiler, the first attempts at self-hosting will begin, and any remaining issues addressed. Everything seems on track to hit our goal of finishing this by the end of 2013.


### Update Dec. 15 2013 ###
The final test of self-hosting passed today! We are tremendously excited to have completed this portion of the project. It's been a long road but well worth the effort.
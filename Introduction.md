# Introduction #
Epoch has opinions. Very strong opinions. It's entirely possible that these opinions will conflict with your own. However, the philosophy behind Epoch is born of many years of exploring other languages and _their_ philosophies, and is ultimately the distillation of what we as the language creators feel _makes sense_.

The goal of Epoch is to make programming smooth, enjoyable, and efficient. Epoch's primary niche is native applications, but there's enough power under the hood to write systems-level code as well if you're sufficiently motivated and brave.


## Motivation ##
Over time, Epoch has claimed to have a few different goals. Upon reflection, however, it turns out that most of these are not really central to the _raison d'etre_ of the language.

**Epoch exists to scratch an itch.**

The itch in question derives from the fact that many existing languages just don't feel great. There are plenty of excellent languages out there, and plenty of reasons to choose them for specific tasks; but ultimately, there's always a mental impedance mismatch between the opinions of those languages and our own.

Epoch was created to offer an alternative for programmers who feel fundamentally dissatisfied with the available language offerings. You don't have to be a connoisseur of languages to appreciate it; we're not founding a culture of exclusivity or superiority.

Put simply, Epoch is for people who might like Epoch. That's nicely self-referential and circular and recursive - just like the language itself.


## Defining Features ##
**Type System**
  * Strongly, statically typed
  * Type inference allows for succinctness
  * Compile time templates for generic programming
  * Value-typed by default, reference-typed by opt-in

**Memory Management**
  * Garbage collection when you want it
  * Manual resource management when you don't
  * Optional deterministic destruction

**Execution Model**
  * Generates native code supported by a thin runtime library
  * Green threads/fibers as first-class entities
  * Rich built-in message passing system for parallelism

**Interoperability**
  * Supports calls to C-ABI functions
  * Can export C-ABI compatible callbacks


## Opinions ##
**General Philosophy**
  * First and foremost, programming should not suck.
  * The path of least resistance should lead to the correct solution.
  * A language which obstructs the programmer is a bad language.
  * Thou shalt not repeat thyself.
  * Also, stating the obvious is a sin.
  * Sometimes the Gross Thing is also the Right Thing.
  * Eat Your Own Dog Food. Epoch and its toolset are developed using Epoch.

**Language Rules**
  * Syntax should be consistent. No optional braces, for example.
  * Implicit conversions are bad. Epoch has none.
  * Composition over inheritance. In fact, Epoch has no inheritance.
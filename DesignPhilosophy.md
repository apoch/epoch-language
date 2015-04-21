# Epoch's Design Philosophy #

Without careful guidance and a sharp focus, it is all too easy for a programming language to  become bloated, inconsistent, inefficient, and even incorrect. To aid in our development of the Epoch language, we have selected a set of operating principles that help avoid problems and provide us with direction for future progress on the language.


## Pragmatism ##

The primary principle of Epoch's design is pragmatism. We specifically aim to create a language that is applicable for real world usage _right now_. As such, we practice continuous integration and make frequent releases, to ensure that the latest capabilities of Epoch are ready for programmers to use immediately.

Pragmatism also guides the scope and depth of Epoch's feature set. Wherever we encounter a modern piece of programming technology, we consider carefully if it will fit within the design of Epoch, and if so, how we can best integrate it to provide seamless, natural, and robust support from within the language.

A major consequence of this philosophy is that we have primarily targeted Windows as our launch platform. Applications development for Windows represents one of the largest collectives of programmers, and as such, we seek to entice that collective with powerful and rich tools for Windows development.

This does not preclude porting Epoch to other platforms; indeed, targeting the Linux family as well as the BSD family of operating systems is high on our priority list. Naturally we also plan to release support for Apple's Mac OS X at some point in the future.


## Multi-paradigm Development ##

We believe that in a modern programming language, restricting the programmer to only a specific coding paradigm is unacceptable. Epoch is designed to allow easy development in procedural, functional, and object-oriented paradigms. Moreover, Epoch supports clean interaction between code written in all three paradigms.

It is absolutely essential for a language to not get in a programmer's way. Instead, a good language supports many approaches to a given problem, in order to allow the programmer to choose the best alternative for a given situation. Freedom of design is an important part of Epoch's philosophy, and we believe that providing maximum freedom allows  - even encourages - the developer to create optimal solutions.


## Eyes on the Future ##

As Epoch grows from a prototype language into a full-scale development toolset, many of the problems Epoch aims to solve have yet to become serious issues within the software development realm. For example, massive scaling across large numbers of CPU cores is not necessary in the majority of contemporary software. Commodity PCs do not yet contain large numbers of cores; and aside from GPUs, auxiliary processing units are fairly rare.

So why do we emphasize parallel development so heavily? The answer lies in projecting the current trends in computing technology a few years into the future. We believe that eventually the majority of deployed computers will embrace _asymmetric multiprocessing_, wherein a piece of code can be directed towards a piece of processing hardware that is best suited for carrying out that code's task.

For example, consider a machine with two GPUs and four CPU cores. We have a large data set that would be best processed using GPGPU techniques. We also wish to simultaneously present this data set in different formats, using the CPU cores to collate and display the results from the GPUs. In Epoch, this scenario is trivial to accomplish, using built-in parallel features.

As this trend continues and diversity in processing hardware continues to grow, it becomes increasingly important to have a programming platform that can stay abreast of the hardware technology. Epoch plans to be ready when the hardware arrives - providing developers with a considerable edge on taking advantage of new technologies.


## Planning Ahead for the Epoch Programmer ##

Our aim is to provide a rich toolbox for programmers to use in developing a wide variety of software solutions in a myriad of problem domains. Because we target a highly diverse audience, we must always be mindful of the programmer who will use the language.

  * We assume the programmer knows his problem domain better than we do
  * We assume the programmer is fallible, and seek to help prevent mistakes, and recover quickly when mistakes do occur
  * We always emphasize programmer productivity, even if it means more work for us in implementing the language

One of the most effective ways to ensure that the language is suitable for real-world use is to develop Epoch's tools in Epoch itself. This "bootstrapping" process helps ensure that our most important goal is met: real applications can be delivered using Epoch as a foundation.


## Other Languages are not the Enemy ##

We do not intend to overthrow any existing dominant language, in any given problem domain. Instead, we seek to supplement existing tools, and provide smooth interoperability with other language technologies. We recognize that discarding a large body of existing code in order to adopt a new language like Epoch is simply not possible or practical for the vast majority of our users. As such, we wish to work alongside existing code as effectively as we can, and provide a seamless transition towards Epoch over time.


## Extensibility ##

Epoch's execution model is designed to allow easy support for new types of computing hardware, as they become available. Extending Epoch to run on new hardware should be straightforward, convenient, and seamless. We also aim to provide rich introspection and metaprogramming capabilities, so that new software technologies can easily be retrofitted into Epoch programs and libraries.

In general, we recognize that we cannot create a language core that solves all problems for all people. Instead, we supply a set of tools for creating the solutions _you_ need, fitting the requirements and constraints that you know best.
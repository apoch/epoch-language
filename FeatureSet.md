# Epoch Feature Set #
As part of developing Epoch, we are always on the lookout for clean abstractions and powerful building blocks that make sense in a modern programming language. Many such tools already exist in academic and research languages, but there is often a lag between the development of new features and their implementation in popular programming languages. Our goal is to deliver these features as first-class citizens of the Epoch language, so that development in Epoch can take advantage of recent advances in programming technology.


## Currently Implemented Features ##
Following is the current feature set of the Epoch language, as of Release 14.

### Types and Data ###
  * Static, strong typing
  * Lexical scoping
  * Type inference in several common situations
  * Aggregate types (structures) and nested structures
  * Algebraic sum types
  * Weak type aliases (similar to C/C++ typedef)
  * Strong type aliases (non-convertible typedefs)
  * `nothing` type (useful for optional types)
  * Templatized (type-generic) data structures

### Functions ###
  * Zero or more parameters mapped to zero or one return values
  * Recursive functions
  * Pass by value semantics
  * Optional pass by reference semantics
  * Function overloading
  * Overloads which differ only by return type
  * Pattern matching
  * Higher order functions
  * Templatized (type-generic) functions


### Variables ###
  * 32-bit integer data type
  * 16-bit integer data type
  * 32-bit IEEE 754 floating point data type
  * String data type (immutable, garbage collected)
  * Character buffer data type (mutable, garbage collected)
  * Boolean data type
  * Meta-type for identifiers/type names
  * Typecast operators
  * Aggregate data types (structures)
  * Global variables


### Flow Control ###
  * If/elseif/else conditionals
  * Standard while and do-while loops


### Standard Operations ###
  * Integer and floating point arithmetic
  * Integer and boolean comparisons
  * Infix operator syntax


### Data Lifetime ###
  * Full, automatic garbage collection


### Language Interoperability ###
  * Marshaling to allow calling C APIs from Epoch code
  * Marshaling to allow C APIs to callback into Epoch code


### Deployment ###
  * Execution of raw source via `EpochTools`
  * Packaging of Epoch programs into `.EXE` files suitable for execution on Windows platforms


## Planned Features for the Future ##
In addition to the language features in place, we plan to roll out several additional features in future releases. The highest priority/highest-impact features are listed below.

### Types and Data ###
  * Type constraints such as range limits
  * Enumerations
  * Type "units" and dimensional analysis support
  * Optional automatic type conversions to mimic dynamic typing

### Functions ###
  * Full first-class functions
  * Partial function application
  * Closures
  * Lambdas
  * Generators

### Variables ###
  * Byte (8 bit) and 64 bit data types, including float64

### Flow Control ###
  * Iteration over a value range or container (e.g. for loops)
  * Exceptions

### Data Lifetime ###
  * Optional deterministic destruction of objects

### Standard Library ###
  * String slices
  * Array/other container manipulations
  * Additional mathematical operations and library functions
  * Standard library containers

### Parallelism and Concurrency ###
  * Task (green thread) model
  * Distributed computation support (e.g. over a network interface)
  * Distributed versions of built-in operations such as map/reduce

### Code Organization ###
  * Support for multiple source files in a single program
  * Namespace system

### Deployment ###
  * Support for deploying Epoch programs on Linux, BSD, and Mac OS X - likely via ports of the Epoch VM
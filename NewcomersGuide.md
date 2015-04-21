# The Newcomer's Guide to Epoch #
Hi, and welcome to the Epoch language! This article will take you on a brief tour of using Epoch to write software. Please note that some familiarity with programming practices and terminology is assumed.


## Part 1: Hello World! ##
All programs in Epoch begin with a specific function, called `entrypoint`. This function is invoked when the program begins execution. You can think of this function as the first thing that your program will do (although there are some exceptions to this rule, but they aren't important just yet).

The basic syntax of a function in Epoch is simple:
> `[function name] : [parameters] [-> return expression] { [code statements] }`

Function names may be any combination of letters, numbers, and underscores, but must begin with a letter. All names in Epoch are case-sensitive.

The `entrypoint` function is defined to have no parameters, and no return values. Therefore, the complete definition of the function looks like this:

```
entrypoint :
{
   [code statements]
}
```

### Console output ###
Epoch provides a simple facility for emitting debug output, called `print`. By default, this function directs output to a console window.

`print` returns no values, and accepts one parameter of type `string`. As is traditional, our first minimal-but-legal program will use `print` to display the message "Hello World!" to the user.

```
entrypoint :
{
   print("Hello World!")
}
```

Note that we use the common curly-braces format for denoting a block of code. Also, note that Epoch does not use a semicolon to terminate a statement; further, Epoch does not use semantic whitespace. (Contrast this with Python, where indentations denote code blocks, or Visual Basic, where carriage returns specify the end of a statement.)

Once you have written this simple program using your favorite text editor, you can then execute it using the EpochTools program from the Epoch SDK. (If you have not yet installed the tools, see the InstallationGuide for detailed instructions on how to set up the SDK and run Epoch programs.)

### Comments ###
Epoch comments are denoted by two slash characters, `//` Anything appearing after the comment marker is ignored by the compiler, up until the end of the line.

```
//
// A simple, friendly message
//
entrypoint :
{
   print("Hello World!")
}
```


## Part 2: Variables ##
Epoch currently supports variables of the following concrete data types:

  * Integer (32 bit) - `integer`
  * Integer (16 bit) - `integer16`
  * Floating point (32 bit IEEE 754) - `real`
  * Boolean - `boolean`
  * String - `string`
  * Raw byte buffer - `buffer`

There are additional types available, but we will focus on these for now.

Defining a variable in Epoch uses the following syntax:

> `type identifier = initial value`

For instance, you can define an integer and initialize it to 42 with the following code:

> `integer answer = 42`

Note that in Epoch all variables _must_ be explicitly initialized when they are declared.

Conversion between types is made possible using the `cast` function. `cast` accepts two parameters: the target type of the cast, and the value to cast. For example, the following code will convert the given number into string format, suitable for output via `print`:

```
integer answer = 42
print(cast(string, answer))
```

Epoch is _strongly typed_ and does not permit implicit conversions between variable types. For this reason, it is important to denote the difference between an integer and a real literal, for instance. Integers are written in the base-10 standard notation (or base-16 notation, such as `0xfabcd`) whereas reals are always written with at least one value after the decimal place, such as `10.42` or `0.0`.

Be sure to provide the correct type of literal for expressions that expect a certain numeric type, or the compiler will emit a type-clash error message.

String literals in Epoch are enclosed in double quotation marks:

`string message = "Hello World"`

Strings in Epoch are native UTF-16, little endian. Please note that despite this runtime requirement there may be limitations in the current compiler infrastructure which prevent literals from being expressed in true UTF-16; if you encounter such bugs, please feel free to report them in the [issue tracker](http://code.google.com/p/epoch-language/issues/list).

The last type of variable we will consider here is the `buffer` type, which corresponds to a raw sequence of uninterpreted byte values. This is mainly useful for intercommunicating with C-style APIs and legacy code that does not understand UTF-16.

Unlike most variables, buffers are not initialized with a literal, but rather a _size_. This describes how many bytes must be able to fit into the buffer. To allocate 512 bytes worth of buffer space, we might use the following variable definition:

`buffer space = 512`

Note that, like all other variables in Epoch, buffers have, by default, _value semantics_. This means that a buffer is copied every time you assign one into a variable, or pass one into a function, unless a reference is explicitly requested using the `ref` keyword in a function parameter list. This can have significant runtime cost, so use buffers sparingly, and prefer strings instead unless interacting with external code.


### Operations ###
Epoch supports the standard arithmetic operators common to most languages (addition, subtraction, multiplication, and division). You may also use the operate/assign form of each operator, such as `a += b`, as well as the standard preincrement/decrement and postincrement/decrement operators, such as `++c`.

Comparison operators are also provided; as with arithmetic operators, they are the same as in many languages, such as C, C++, C#, Java, PHP, and so on.

Epoch provides a single operator for strings, the concatenation operator:

> `stringa = stringb ; stringc`

This code will result in `stringa` containing the value of `stringb`, immediately followed by the value of `stringc`. The concatenate-and-assign operator `;=` is also provided.


## Part 3: Flow Control ##
Epoch provides `if`/`elseif`/`else` flow control constructs, as well as `while` and `do-while` looping constructs.

All code blocks assigned to a conditional or loop must be fully wrapped with curly braces `{ }` - unlike many C-family languages, the braces are not optional.

Here is a simple example of using `if`/`elseif` statements, which behaves as you would expect:

```
entrypoint :
{
	print("Enter a number:")
	integer foo = cast(integer, read()))

	if(foo == 0)
	{
		print("You entered zero")
	}
	elseif(foo == 1)
	{
		print("You entered one")
	}
	elseif(foo == 2)
	{
		print("You entered two")
	}
	else
	{
		print("You entered something else")
	}
}
```

`while` loops operate in a manner similar to most languages. The loop control parameter is a single boolean value, and may be provided either by a variable, or by any expression or function call which evaluates to a boolean.

```
entrypoint :
{
	integer beer = 5

	while(beer > 0)
	{
		integer test = 1

		print("Beer supply: " ; cast(string, beer))
		print("Take " ; cast(string, test) ; " down, pass it around...")

		beer -= test
	}

	print("Bummer, we're out of beer!")
}
```

Lastly, Epoch provides `do`/`while` loops for cases where you would like the loop body to execute at least once before checking the loop condition. As you may expect, these constructs can be nested and otherwise combined for complex flow control logic:

```
pi : -> real retval = 0.0
{
	real denominator = 1.0
	boolean isplus = true

	do
	{
		real div = 4.0 / denominator

		if(isplus)
		{
			retval += div
		}
		else
		{
			retval -= div
		}

		isplus = !isplus
		denominator += 2.0

	} while(denominator < 10000.0)
}
```


### Functions ###
Epoch supports standard function return values; the values returned by a function are specified in the function definition. As with variables, function return values must be initialized immediately.

```
sum : integer foo, integer bar -> integer retvalue = 0
{
   retvalue = foo + bar
}
```

This code defines a function `sum` which takes two integer parameters (`foo` and `bar`) and returns a single integer value, `retvalue`. Note that `retvalue` is initialized in the function definition. Parameters do not need to be explicitly initialized, because their initial values will be passed in by the calling code.

Note that unlike many programming languages, `sum` does not use the `return()` function to return a value. Instead, when the function exits, the current value of `retvalue` is used as the function's return value. This means that the `return()` function never needs any parameters; when invoked, `return()` will simply return the current value(s) of the corresponding named return variables.

### Simpler Function Notation ###
Epoch supports _type inference_ for automatically determining the return type of many functions. In some cases it is not necessary to define a return variable at all, and the code body of the function can even be omitted. For example, we can state the `sum` function from above equivalently as the following:

```
sum : integer foo, integer bar -> foo + bar
```

This can be very useful for functions with relatively simple implementations, where the effect of the function can be accomplished in a single expression rather than requiring an entire code body worth of statements.


### Free Blocks/Artificial Scopes ###
Artificial lexical scopes can be introduced at any time by surrounding a set of code statements with curly braces `{ }`. These free blocks work much the same as in other C-family languages:

```
entrypoint :
{
	integer outer = 40

	{
		integer inner = 2
		print(cast(string, inner + outer)) // Prints 42
		outer = 666
	}

	//inner = 666   --- Compile error, can't access inner from this scope!
	print(cast(string, outer)) // Prints 666
}
```


> _**Note:** Artificial scopes are currently primarily useful for organization purposes, but later on in the development of the Epoch language, we plan to introduce constructs that allow for deterministic destruction of objects at the end of a given scope, much like RAII in C++. This idiom will allow artificial scopes to control lifetime of objects explicitly, which will significantly improve their usefulness._


## Part 4: References ##
As mentioned above, all data in Epoch conforms to a _value model of variables_. That is, a variable can be thought of like a "bucket" in which a value always resides. Assigning one variable to another copies the value from one bucket into the second; each bucket retains a unique (but identical) copy of that value.

In many cases it is useful to be able to refer to variables as _references_ instead. This is accomplished in Epoch via the `ref` keyword. Currently only function parameters may be references, although support for more general references is on its way.

```
mutate : integer ref variable
{
	++variable
}

entrypoint : () -> ()
{
	integer test = 41
	mutate(test)
	print(cast(string, test))  // Prints 42
}
```

## Part 5: Structures ##
Aggregate types are available in Epoch under the name _structures_. A structure definition looks similar to a function definition, but does not include any return types or code:

```
structure Person :
   string FirstName,
   string LastName,
   integer Age
```

Structures must be initialized on creation, just like every other variable type; this is done (by default) by providing a value for each field. Note that this behaviour can be customized, although the process of _overloading constructors_ is beyond the scope of this introduction.

`Person fred = "Fred", "Jones", 42`

Structure members can be read and written using the `.` operator:

```
fred.LastName = "Smith"
print(cast(string, fred.Age))
```


## Part 6: Garbage Collection ##
Epoch is a fully garbage-collected language. This means that you do not have to worry about the lifetime of your objects; they will automatically be reclaimed by the runtime when they are no longer in use.

This applies to all built-in resources in Epoch: strings, buffers, structures, and so on. For example, the following program will run indefinitely and never run out of memory, because the garbage collector will continually reclaim strings that are allocated but then discarded:

```
entrypoint :
{
	while(true)
	{
		integer counter = 0
		while(counter < 100000)
		{
			foo(counter)
		}
	}
}


foo : integer ref counter -> string ret = ""
{
	string alpha = "a"
	string beta = "b"

	ret = alpha ; beta ; cast(string, (++counter))
}
```


## Additional Examples ##
Don't forget to browse around the [Examples Repository](http://code.google.com/p/epoch-language/source/browse/?repo=examples) for more Epoch programming information. With these basic rules in mind, understanding the examples should be quite straightforward.
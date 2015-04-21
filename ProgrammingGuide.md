# Programming Guide #

## Additional Resources ##

Some topics are large enough to merit their own pages; a list of such topics which have been split into separate areas follows:

  * Win32Interop


## Function Overloading ##

Epoch allows the same function name to be used for various configurations of parameters and return types, commonly referred to as _overloading_ the function.

Overloading permits us to write code like the following:
```
overloaded : integer input -> "Integer overload"
overloaded : string input -> "String overload"

entrypoint :
{
    print(overloaded(42))
    print(overloaded("Test"))
}
```

Because the first call to `overloaded` passes an `integer`, the first overload of the function is selected (at compile time) and therefore the first log line will display `"Integer overload"`. Similarly, the second call passes a `string`, so the second log line will display `"String overload"`.

In Epoch, unlike many languages, it is permissible to overload a function based only on its return type:
```
converter : integer input -> integer output = input
converter : integer input -> string output = cast(string, input))
```

When this occurs, Epoch uses the context of the function call to determine which overload to invoke. For instance, if we passed `converter(42)` into a function expecting an `integer`, the first overload would be called. By contrast, if we passed the same expression into a function expecting a `string`, the second overload would be called.

This allows for easy creation of type-conversion wrappers which can be used to transparently cast a source type into many other types more or less implicitly. It also allows for context-sensitive code that performs different logic (perhaps optimizing a certain code flow) depending on the expected type of the result.

Note that if Epoch cannot safely determine which overload to call based on context, a compile-time error will be produced.


## Pattern Matching ##

In many traditional imperative languages, function overloads are only selected based on the _types_ of the parameters. Functions in Epoch support _pattern matching_, which provides a means to select a function overload based on the _values_ of the parameters. This enables the simple and elegant definition of rich, recursive processes.

Consider the classic Fibonacci sequence. We can define this using Epoch pattern matching as follows:
```
fib : 0 -> 1
fib : 1 -> 1
fib : integer n -> integer ret = fib(n - 1) + fib(n - 2)
```

Note that we can use _type inference_ and _anonymous return values_ to simplify the definitions of the `fib` function for the base cases of 0 and 1. Due to type inference, Epoch realizes that `fib` returns an `integer` for both the 0 and 1 base cases, and so we do not need to explicitly provide a return type. We can also eliminate the name of the return variable.

However, in the general case, it is not possible to perform type inference for the return value of `fib`. This is because the `fib(n)` overload relies on the value of other calls to the `fib(n)` overload itself. If no return type was specified, the overload would have a circular reference to itself, preventing the type inference engine from determining the return type.

Unfortunately, this problem is not solvable in the general case, because many such situations could reduce to analogues of the Halting Problem. Therefore, we opt for the simplest approach, which is that recursive functions using pattern matching for base conditions must explicitly specify their return types for the general-case overloads.


## Global variables ##
Epoch supports global variables using the `global` entity:

```
global
{
    integer counter = 0
}

entrypoint :
{
    ++counter
    print(cast(string, counter))
}
```

As is generally the case, global mutable state is inadvisable and is provided solely for convenience in producing small programs. Larger software should avoid using global variables.

However, global constants may still be quite useful.


## Type Aliases ##
Epoch supports both _weak_ and _strong_ type aliases. A weak type alias creates a secondary name for a type, which is considered equivalent to the original type:

```
alias Meters = integer
alias Height = Meters
alias Width = Meters

SquareArea : Height h, Width w -> h * w
```

In this code, `Meters` is a type which is interchangeable with `integer`. The `Height` and `Width` types are in turn interchangeable with `Meters` and therefore by extension with `integer`.

_Strong_ aliases, by contrast, are not convertible to their internal type. This is useful for creating types which are structurally equivalent to other types, but should not be considered interchangeable:

```
type Meters : integer
type Feet : integer

RectArea : Meters w, Meters h -> Meters ret = w * h

// This code would cause a type error:
typeerror :
{
    Feet w = 12
    Feet h = 6

    RectArea(w, h)
}
```

Since `RectArea` is defined to take `Meters`, it will not accept `Feet` as input even though both types are based on `integer`.

Type aliases are resolved entirely at compile time and carry zero runtime overhead.


## Algebraic Sum Types ##
Epoch provides a facility for defining _algebraic sum types_, also referred to as _discriminated unions_. A sum typed variable may contain a value of any of several different _base types_.

```
type intorstring : integer | string
```

Sum typed variables cannot be examined directly; they must be _decomposed_ via pattern matching or function overload resolution to be used:

```
check : integer param, integer expected
{
    assert(param == expected)
}

check : string param, string expected
{
    assert(param == expected)
}

entrypoint :
{
    intorstring foo = 42
    intorstring bar = "test"

    check(foo, 42)
    check(bar, "test")
}
```


## The `nothing` Type ##
Epoch includes a special type named `nothing` which represents the absence of any meaningful value. It is distinct from `null` _values_ in other languages, in that it is a type unto itself, and cannot be stored in variables of other types. Specifically, it is not permissible for references in Epoch to be `null`.

`nothing` by itself may not seem terribly useful, but in combination with algebraic sum types, it permits programs to have _optional types_ which provide statically verifiable checking for omitted values. This turns "null pointer exception" bugs into compile-time errors.

```
type optional : integer | nothing

test : integer i
{
    assert(i == 42)
}

test : nothing
{
    // Do nothing!
}

entrypoint :
{
    optional foo = 42
    optional bar = nothing

    test(foo)
    test(bar)
}
```


## Recursive Types ##
Types are permitted to be recursive, which is useful for constructing certain data structures. Combined with optional types, we can write a simple linked list:

```
type listnode : list | nothing

structure list :
    integer value,
    listnode next

walklist : list ref thelist
{
    print(cast(string, thelist.value))

    listnode nn = thelist.next
    walklist(nn)
}

walklist : nothing
{
    print("End of list")
}
```

A complete example can be found [here](https://code.google.com/p/epoch-language/source/browse/Compiler%20Test%20Suite/Type%20System/listofintegers.epoch?repo=examples).


## Template Structures ##
Structures can be _templated_ based on a given type. This allows a structure to be reused with different member types based on the parameterized type passed:

```
structure wrapper<type T> :
    T contents,
    string tag
```

The `wrapper` type here is a _template_ and not directly useful; it must be _instantiated_ in order to be used:

```
entrypoint :
{
    testwrapper<integer> intwrap = 42, "number"
    testwrapper<string> strwrap = "test", "text"
    testwrapper<boolean> boolwrap = true, "flag"

    assert(intwrap.contents == 42)
    assert(intwrap.tag == "number")

    assert(strwrap.contents == "test")
    assert(strwrap.tag == "text")

    assert(boolwrap.contents)
    assert(boolwrap.tag == "flag")
}
```


## Template Functions ##
Just like structures, functions can be templated:

```
add<type T> : T a, T b -> T sum = a + b

entrypoint :
{
    integer foo = add<integer>(40, 2)
    assert(foo == 42)
}
```

Note that we plan on removing the necessity of explicitly parameterizing function templates in the future, based on type inference.
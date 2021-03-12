# A Guide to bread

A guide to the `bread` programming language
A guide through the syntax, features, and semantics of the `bread` programming
languages.

# Types

`bread` is a dynamically typed language, with a small number of distinct types.

## number

All numbers in `bread` are floating point numbers. Internally the `long double`
C type is used for numbers, so integer arithmetic is still (reasonably) accurate.
Numerical literals are defined by whatever the `strtold` function in the C standard
library accepts, with the caveat that numbers cannot begin with a decimal point
(i.e `.5` is not a valid expression, but `0.5`, `5e-1` and `5E-1` are).

When coerced to a string, numbers become the string representation of the number
(using the `%Lg` formatter). All non-zero numbers are truthy.

## string

Strings in `bread` are immutable ASCII strings. Theoretically a string
can contain any UTF-8 sequence, but indexing operations will behave in undesirable
ways if this is the case. String literals are enclosed in double quotes, with
the standard escape characters for newlines, tabs, and double quotes.

When coerced to a number, `strtold` is used to parse the string as a number.
All non-empty strings are truthy.

## boolean

Boolean values are either `true` or `false`.

When coerced to a number, `true` becomes 1 and `false` becomes 0. When coerced
to a string, `true` and `false` become `"true"` and `"false"`, respectively.

## unit

A `unit` contains no information other than it's own existence.

When coerced to a number, a unit becomes 0.
When coerced to a string, a unit becomes the string `"unit"`.
Unit values are not truthy.

## builtin

`bread` provides several builtin functions for basic IO and other operations.
The identifiers of all builtin functions begin with the `@` symbol.
Bread provides the following builtin functions:

* `@write(args...)` prints the arguments to stdout.
* `@writeln(args...)` prints the arguments to stdout with an additional newline.
* `@readln()` takes no arguments and reads a line from stdin (if stdin is closed, `unit` is returned)
* `@length(arg)` reports the length of a string, list, or dict
* `@typeof(arg)` reports the type of its argument
* `@system(args...)` runs a shell command, and returns the exit code of the command
* `@push(list, arg)` pushes a value onto the end of a list
* `@insert(list, arg, idx)` inserts a value into a list at a given index

Builtins cannot be coerced into a number. When coerced into a string,
a builtin becomes the name of the builtin (including the "@"). All builtins
are truthy.

## list

A list is in fact an array of non-homogeneous elements. List literals
are comma separated expressions surrounded by a pair of square brackets.

Lists cannot be coerced into a number. When coerced into a string, a list
becomes a string representation of the list. All non-empty lists are truthy.

## closure

`bread` has first class functions in the form of closures which capture
their environment when initialized. Closures are immutable, and are initialized
with the function definition syntax discussed below. If `foo` is a closure,
then `foo` can be called with the syntax `foo(args...)`.

Closures cannot be coerced into a number. When coerced into a string, a closure
becomes the string `"closure"`. All closures are truthy.

## class

Classes are first class values in `bread`. The only class which `bread` initially
provides is the `@Object` class, and all other classes must be defined as a subclass
of `@Object`, using subclass definition syntax discussed below.

Classes cannot be coerced into a number. When coerced into a string, a class
becomes the string `"class"`. All classes are truthy.

## object

Objects are instances of a class and have fields and methods. All fields
of an object are mutable, while the methods are immutable (since classes are
immutable). if `Foo` is a class, then an object of the class `Foo` can
be instantiated with `Foo(args...)`, and these arguments are given to the constructor
of `Foo`. While all objects are of the same type, it is possible to check what class
an object is an instance of (the syntax for this will be shown later).

Objects cannot be coerced into a number. When coerced into a string, an object
becomes the string `"object"`. All objects are truthy.

## dict

Dictionaries map strings to values.
Dictionary literals are comma separated key-value pairs surrounded by a pair
of curly braces, where the key is a string literal and the key and value are separated
by a colon (not dissimilar to JSON).
An empty dictionary may also be initialized with the `@dict` builtin.
Note that keys whose value is `unit` in the dictionary do not count towards the
length of the dict.

Dictionaries cannot be coerced into a number. When coerced into a string,
a dictionary becomes the string `"dict"`. All dictionaries are truthy.

## method

Methods are closures which also carry a reference to some object. They behave
the same way as closures aside from their type.

Methods cannot be coerced into a number. When coerced into a string, a method
becomes the string `"method"`. All methods are truthy.

Write something about how variables work.

# Expressions

Here we discuss the syntax and semantics of some expressions not defined above.
As `bread` is an expression based language, more things are expressions in
`bread` than in many other imperative programming languages. All expressions
are evaluated from left to right.
Single line comments in bread begin with the `#` symbol, and there are no
multi line comments.

## Arithmetic Operators

When using an arithmetic operator, all operands are coerced into numbers before
performing the arithmetic.
`bread` has the regular arithmetic operators `+, -, *, /, %` with the (hopefully)
expected semantics, associativity, and precedence. Additionally, `bread`
has the integer division operator `//` and the exponentiation operator `^`.
The syntax of `//` is the same as `/`. Exponentiation is right associative
and binds tighter than every other binary operator.

## Boolean Operators

The three boolean operations in `bread` are `and`, `or`, and `not`. If the first
operand of `and` is truthy, then the second operand is evaluated and returned.
Otherwise the first operand is returned. Is the first operand of `or` is truthy,
then the first operand is returned. Otherwise the second operand is evaluated
and returned. If the operand of `not` is truthy, `false` is returned, otherwise
`true` is returned.

## Comparison Operators

The comparison operators in `bread` are `<, <=, =, !=, >, >=`. 
Equality for strings, numbers, booleans, lists, and units works as expected
(there is no epsilon threshold or comparing numbers, so floating point errors
are possible here). Closures, objects, and classes are checked for equality
by reference. Two methods are equal if they contain a reference to the same
object and the same closure. As for checking values of different type for equality,
the following rule set is used:

1. If one of the operands is a number, then try and coerce the other operands to
a number. If it can, check for equality, If it cannot, return `false`.
2. If one of the operands is a string and the other is a boolean, string, or unit,
coerce it to a string and check for equality.
3. If neither of the above hold, return `false`.

As for comparison, for strings and numbers comparison works as expected.
For other types, comparison operators will return false. For operands of different
types, the same rule set as above is used.

## Concatenation Operator

The `..` operator is used to concatenate strings and lists. If both operands
are lists, the concatenation of the lists is returned (neither of the operands
are mutated). If the left operand is a list and the right operand is not a list,
then the result of appending the right onto the left is returned (again, neither
operand is mutated). Otherwise, both operands are coerced into strings and the
concatenation is returned. Concatenation is left associative, and every other
binary operator binds tighter than concatenation.

## Indexing

Lists and strings can be indexed with square brackets. The value inside the brackets
will be coerced into a number.  If the number has a decimal part, it will be truncated
before indexing. If the index is greater than the length of the string/list, indexing
is performed modulo the length. If the index is less than 0, then indexing is performed
off of the end of the string/list. In this way, `"Hello"[4]`, `"Hello"[9]`,
`"Hello"[-1]`, and `"Hello"[-6]` all return the string `"o"`. If the string/list
has length 0, then any indexing will return `unit`.

Dictionaries can also be indexed with square brackets. The value inside
the brackets must be a string. This string will be looked up in the dictionary.
If it is indeed a key of the dictionary, then the corresponding value is returned.
Otherwise, `unit` is returned.

## Object Syntax

Objects have two important operations---accessing fields and accessing methods.
Fields can be accessed and set with the dot operator.

```
set obj = @Object()
set obj.num = 17
@writeln(obj.num)
```

If the accessed field does not exist, `unit` is returned. Methods of an object
can be accessed with the `::` operator. These are immutable, since classes
are immutable. Moreover, there are two special words that can be placed
on the right hand side of `::`. If `x` is an object, then `x::class` is the
class of which `x` is an instance, and `x::super` is `x` as an instance of
the superclass.

## Set Expressions

Set expressions are used to set some value. Set expressions take the form
`set LVALUE = EXPRESSION`, where `LVALUE` is the name of a variable, an index
into an lvalue, or accessing the field of an lvalue. Indexing is valid on lists
and dictionaries, and accessing fields is only valid on objects.
The value of a set expression is the value of the expression on the right hand side
of the equals sign.

## Begin/End expressions

Begin/End expressions allow you to group several statements into a single expression.
The value of a Begin/End expression is the value of the last statement.
For example the following expression:

```
begin
  @writeln("Hello, World!")
  5
end + 10
```

will print `Hello, World!` and evaluate to `15`. If there is only a single statement,
then the expression can be written in a single line: `begin 10 end` has value 10.
This isn't particularly useful for begin/end expressions, but the 3 following
expressions work in a similar way.

## If Expressions

Conditionals in `bread` take the form of if expressions. Here is an example
of an if expression:

```
if true then
  @writeln("true!")
  17
elif false then
  @writeln("false!")
  "nineteen"
else
  @writeln("neither true nor false??")
  false
end
```

The value of an if expression is the value of the last statement in whatever branch
is taken. The previous example would have value `17`. If there is no `else` clause
and no branch is taken, then the value of the expression is `unit`. If expressions
can be written in a single; the expression `if 3 < 2 then "a" else "b" end`
evaluates to the string `"b"`.

## For/While Expressions

`bread` has two types of loops: for expressions and while expressions.
The value of a loop is a list where each entry is the value of the body on the loop
on each iteration. For example, the following loop:

```
for i = 0,5 do
  @writeln(i)
  i < 3
end
```

would evaluate to the list `[true, true, true, false, false]`, and print
numbers 0 through 4. Loops can also be written inline:

```
set i = 0
set list = while i < 5 do set i = i + 1 end
```

In that example, the variable `list` would have value `[1,2,3,4,5]`.
If the list is not needed, the `for` or `while` can be appended with an asterisk,
in which case the value of the loop is `unit`.

## Closure Definitions

Closures are defined with the `func` keyword. The value returned by a closure
is the value of the last statement in the function body (there is no `return`
keyword). Closures capture the environment in which they are defined.
The following program:

```
set add = func(a)
  func(b)
    a + b
  end
end

@writeln(add(14)(6))
```

will print the number `20`. Use the identifier `self` to define a recursive closure.

## Subclass Definitions

When defining a subclass, one provides a superclass, a constructor, and any number
(possibly zero) of method definitions. In the constructor and the bodies of methods,
the variable `this` will refer to either the object being constructed or the
object which called the method. `bread` provides the initial class `@Object`,
and every class will be a subclass of this class.

```
set Greeter = subclass(@Object)
  constructor(message)
    set this.message = message
    this
  end

  set greet = func()
    @writeln("Hello, ", this.message)
  end
end

set greeter = Greeter("Dave")
greeter::greet() # prints: "Hello, Dave"
```

Note how the constructor returns the `this` object --- this is necessary, and I
haven't decided whether this was a good design decision or not.
Unlike functions, conditionals, and loops, subclass definitions cannot be written
in a single line.

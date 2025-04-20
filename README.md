# PetWay library

## Warning

Everything is work in progress and subject to change.

## Building

The following environment variables are honoured by cmake:

* `DEBUG`: debug build (XXX not fully implementeded in cmake yet)
* `PW_WITHOUT_ICU`: if defined (the value does not matter), build without ICU dependency

## PW values

PetWay is primarily targeted to 64-bit systems. All values are 128 bit wide.

The basic PW values do not require memory allocation.
Functions never return pointers to PW value, they always return the whole 128 bit structure.
It's the caller's responsibility to destroy values returned by functions.
The simplest and natural way to do that is assigning them to automatically cleaned
local variables declared with `PwValue`.

Generally speaking, variables should always be declared using `PwValue`
type specifier and initialization clause.
Such variables are automatically destroyed on scope exit thanks to `gnu::cleanup` attribute.
Yes, it's a `GNU` extension, but this is supported by `Clang` too.
Sorry, msvc users.
```c
{
    PwValue myvar1 = PwUnsigned(123);
    PwValue myvar2 = PwNull();

    // do the job
}
```

Note, if `PwValue` is used in a loop body, always use nested scope (double curly brackets)
to run destructors on each iteration:
```c
for (unsigned i = 0, n = pw_array_length(my_array); i < n; i++) {{
    PwValue value = pw_array_item(my_array, i);

    // process item

    // without nested scope we'd have to explicitly
    // call pw_destroy(&item)
}}
```

Functions should use `PwResult` for the type of return value.
Upon exit the value should be returned with `pw_move` that prevents automatic destruction:
```c
PwResult foo()
{
    PwValue result = PwBool(false);

    // do the job

    return pw_move(&result);
}
```

Values are passed to functions by reference to make things a bit more efficient.
Functions do not and should not modify arguments, they treat them as immutable.

There are a few exceptions that accept PW values by value: `PwArray()`, `PwMap()`,
`pw_strcat()`, and `pw_path()`. All they are variadic functions, and they destroy
their arguments upon return. This is convenient to construct objects
using function calls only, i.e.
```
PwValue my_map = PwMap(
    PwCharPtr("foo"), get_answer(),
    PwCharPtr("bar"), PwSigned(42)
)
```

but extra care should be taken when passing local variables:
always use `pw_clone()` for this purpose.


## Type system

The first member of PW value structure is type identifier.
It takes 16 bits. Other bits in the first 64-bit half can be used for various purposes.
The second 64-bit half contains either a value of specific type or a pointer.

The type of value can be obtained with `pw_typeof` macro that return a pointer to `PwType` structure.
That structure contains `id`, `name`, `ancestor_id`, basic interface, and other fields.

As you might already guess, `ancestor_id` field implies type hierarchy.

### Basic PW type hierarchy

```
                                PwType
                                  |
   +------+-----+-------+---------+---------+----------+--------+
   |      |     |       |         |         |          |        |
PwNull PwBool PwInt  PwFloat PwDateTime,   PwPtr,   PwString PwStruct
                |            PwTimestamp  PwCharPtr             |
                |                                               |
           +----+----+                 +----------+---------+---+-----+
           |         |                 |          |         |         |
       PwSigned PwUnsigned          PwStatus  PwStringIO  PwFile  PwCompound
                                                                      |
                                                               +------+
                                                               |      |
                                                            PwArray  PwMap
```

Integral types `PwNull`, `PwBool`, `PwInt`, `PwFloat`, `PwPtr`, and `PwCharPtr`
are self-contained and do not contain pointers to allocated memory blocks.
Actually, `PwPtr` and `PwCharPtr` may do, but it's entirely up to the user
to manage that.

`PwString` may allocate memory if string does not fit into 128-bit structure.
For single-byte character this limit is 12 which is more than double of average
length of English word.

`PwCharPtr` facilitates work with null-terminated C strings.
Its `clone` method converts C strings to `PwString` and other types, such as
array and map, take advantage of that:
```c
PwValue my_map = PwMap(
    PwCharPtr(u8"สวัสดี"), PwChar32Ptr(U"สบาย"),
    PwCharPtr("let's"),  PwCharPtr("go!")
)
// now my_map contains PW strings only
```

`char*` and `char8_t*` are treated equally, as UTF-8 strings.

`Struct` type is the basic type for structured and `Compound` types.
It handles data allocation and reference counting, although allocated data
is optional for `Status`.

The data attached to `Struct` may contain PW values, but it does not handle
circular references which may cause memory leak.

`Compound` type handles circular references. It's the base type for `PwArray`
and `PwMap` which may contain any items, including self.
No garbage collection is necessary.

`Status` type is used to check returned PW values for errors.
Because of this special purpose, `Status` values can't be added to arrays
and used in maps.

## Interfaces

`PwType` structure embeds two interfaces: `basic` and `struct`.
They are simply pointers to functions.

Additional interfaces are stored in a dynamically allocated array
and require linear lookup to get interface pointer by id.
This is still fast as long as the number of interfaces is usually
countable by fingers.

From the user's perspective interfaces are structures containing function pointers
but PW library knows nothing about user-defined types and treats
interfaces as arrays of function pointers.
The number of functions in each interface is stored in a global list
when interface is registered.

Methods of interfaces are invoked like this:
```c
    pw_interface(PwTypeId_MyType, MyInterface)->my_method(self);
```

Note that type id must be provided explicitly and cannot be taken from `self`.
That's because methods are linked to interfaces, not data they process.
If we took type id from `self`, this would lead to infinite loop if the method
called super.

If a method of interface wants to call super method, it should do this way:
```c
    pw_interface(pw_ancestor_of(PwTypeId_MyType), MyInterface)->my_method(self);
```

## Strings

PW strings support any character width, from 1 to 4 bytes.
The initial width can be specified when string is created.
It is automagically expanded when necessary.

Short strings are stored in 128-bit wide PW value structure:
* 1 byte wide: up to 12 characters
* 2 byte wide: up to 6 characters
* 3 byte wide: up to 4 characters
* 4 byte wide: up to 3 characters

Longer strings are stored in dynamically allocated memory blocks.
Blocks contain reference count, so making a copy is fast operation (`clone`, in terms of PW).

Strings are mutable, but they are copied on write.
In-place modifications are allowed only for strings with reference count equal to one.

The length of string is always 32 bit wide, even on ILP64.

### COW rules for strings

* if a string is copied, only refcount is incremented
* if a string is about to be modified and refcount is 1, it is modified in place.
* if a string is about to be modified and refcount is more than 1, a copy is created
  with refcount 1 and then modified in place.
* string capacity is preserved on copy

## Iterators

Iterators are at the very early development stage.
The only iterator at the moment is `LineReader` interface
moved from `StringIO`.

Basically, most iterables may have multiple iterators.
Mind algorithms with nested loops iterating over same array or list.

However, some iterables may support single iterator only.
These are files and most kinds of streams that can be read
only sequentially, without prior knowledge of size.

Such iterators can be considered as singletons.
Moreover, iterables that allow singleton iterators only,
can provide iterator interfaces for self.

There can be multiple versions of iterators for the same iterable.
Back to files, `LineReader` is not the only possible iterator.
There also can be `ByteReader` and `CharReader` and file iterable
may provide all of them.

Iterators that are separate from their iterables are represented
by `PwType_Iterator`.
This type contains iterator state and a clone of iterable value.

Iterables must include `itercount` field that shows how many iterations
are currently in progress.
Methods that modify iterable must fail when this field is nonzero.

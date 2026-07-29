# ZefC Programming Language

ZefC is a transpiled-to-C using Zef as its language definition and Orchard-C's virtual tables as its dispatch mechanism, which itself is inspired by optimizing Objective-C's message send dispatch.

Zef is a dynamically typed object oriented and functional programming language developed primarily to [demonstrate language implementation technique](https://zef-lang.dev/implementation.html).

Orchard is a proof of concept of a little beyond "C++ in C" (as used in Gnome and at the W3C) and adding a small preprocessor, virtual methods, garbage collection, and transparent bridging to other languages.

## Key Features

### From Zef

**Simple Dynamic Types.** Variables and fields do not have static type. All dynamic types are subclasses of `Object`. `int` and `double` are objects. Integers and doubles are both are 64-bit. `null` is a synonym for `0`. `false` is a synonym for `0`. `true` is a synonym for `1`.

**Classes.** Zef's class system is most like Ruby's. Fields are private to their instance, except when exposed via accessors.

**Closures.** Classes and functions can be nested to any depth and doing so creates closures. For example, the member functions of a class nested in a function can see the outer function's local variables.

**No Funny Business.** Zef differs from other dynamic languages in that it lacks `eval` and has no monkey patching facilities. The closest thing to `eval` is load, which loads a source file; however the code in that file runs in its own scope. Classes statically declare their fields; new fields cannot be added dynamically. It's not possible to dynamically add fields to objects or to change object types.

**Packages.** Classes and functions can be nested in packages, making them globally visible to anyone who names or imports the package.

**Garbage Collection.** Allocate what you want. The GC cleans it up for you.

### From Orchard

**Virtual Table Dispatch.** All method names share a namespace and are assigned a sequential selector id so that tables can be grown dynamically as new packages are loaded.

**C++-like method dispatch.**  `(*(obj->isa_))[selector](obj, selector, ...)` at call sites.

**Intermix Zef and C++.** Implementation code in .moc (C++ with ZefC patterns) can mix dispatch code with C calling code, and cpp files are generated.  .zef files are Zef-only, transpiled by the ZefC parser/compiler.

**Mangled argument encoding.** Gets, puts, calls, nested-class surfaces, and operators are mangled with their calling arguments to create a selector.

**Short circuits.** Methods that take native types, assert object type or use type coercion, and then use internal accessors to speed operations.

**Undefined methods.** Selectors that are not implemented on a class's hierarchy are filled with doesNotUnderstand.

### New since Orchard

**Expandable vtables.** Selector IDs allow new symbols to be added as packages are loaded, updating vtables.

**Dynamic Selector Patching.** Selector IDs are assigned at runtime and patched by constructors at load time, similar to how position-independent code is resolved at runtime, enabling dynamic package loading without recompiling existing translation units or pre-calculating IDs globally.

## Example Program

    class Foo {
        # instance field with getter
        readable x

        # constructor
        fn (inX) x = inX

        # Operator overloading by name
        fn add(y) Foo(x.toString + " " + y.toString)

        # Overloading to-string conversion
        fn toString x
    }

    # Prints "hello world" and then a newline
    println(Foo("hello") + Foo("world"))

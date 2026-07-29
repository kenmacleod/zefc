# ZefC Programming Language

ZefC is a compiler that transpiles the Zef language to C++, using Orchard-C (Orchard) style virtual tables for dispatch, which is inspired by Objective-C's message send dispatch.  Some code from Zef and Orchard is borrowed into this project but this project does not have a dependency on them.  ZefC is compiled with Fil-C++; generated code is intended to also be compiled with Fil-C++.

## Lineage - source of inspiration

* Zef is a dynamically typed object oriented and functional programming language developed primarily to [demonstrate language implementation technique](https://zef-lang.dev/implementation.html).

* Orchard is a proof of concept of a little beyond "C++ in C" (as used in Gnome and at the W3C) and adding a small preprocessor, virtual methods, garbage collection, and transparent bridging to other languages.

## ZefC Key Features

### From Zef

**Simple Dynamic Types.** Variables and fields do not have static type. All dynamic types are subclasses of `Object`. `int` and `double` are objects. Integers and doubles are both 64-bit. `null` is a synonym for `0`. `false` is a synonym for `0`. `true` is a synonym for `1`.

**Classes.** Zef's class system is most like Ruby's. Fields are private to their instance, except when exposed via accessors.

**Closures.** Classes and functions can be nested to any depth and doing so creates closures. For example, the member functions of a class nested in a function can see the outer function's local variables.

**No Funny Business.** Zef differs from other dynamic languages in that it lacks `eval` and has no monkey patching facilities. The closest thing to `eval` is load, which loads a source file; however the code in that file runs in its own scope. Classes statically declare their fields; new fields cannot be added dynamically. It's not possible to dynamically add fields to objects or to change object types.

**Packages.** Classes and functions can be nested in packages, making them globally visible to anyone who names or imports the package.

**Garbage Collection.** Allocate what you want. The GC cleans it up for you.

### From Orchard

**Virtual Table Dispatch.** All method names share a namespace and are assigned a sequential selector id.

**C++-like method dispatch.**  `(*(obj->isa_))[selector](obj, selector, ...)` at call sites.

**Intermix MOC and C++.** Implementation code in .moc can mix dispatch code with C++ calling code, and cpp files are generated.

**Mangled argument encoding.** Gets, puts, calls, nested-class surfaces, and operators are mangled with their calling arguments to create a selector.

**Short circuits.** Methods that take native types, assert object type or use type coercion, and then use internal accessors to speed operations.

**Undefined methods.** Selectors that are not implemented on a class's hierarchy are filled with doesNotUnderstand.

### New since Orchard

**Dynamic Loading.** ZefC, like Zef, allows packages to be loaded at run time and vtables to grow.  Selector ID call-sites are patched by constructors, similar to how position-independent code is resolved at runtime, enabling dynamic package loading without recompiling existing translation units or pre-calculating IDs globally.  Like Zef, ZefC does not allow object/class mutation.

**Intermix Zef and C++.**  .zefc (C++ with ZefC patterns) files do this and replace .moc. .zef files are Zef-only, transpiled by the ZefC parser/compiler.

### Build considerations

**Meson.** Uses `meson.build` and Ninja.

**Build targets.** Ninja uses standard GNU-style build targets, `ninja` (to build), `ninja test` for check, `install`, `uninstall`, `clean`, `dist`.  ZefC is intended to be cleanly packaged using Fedora and Debian packaging guidelines for C++ (ZefC build) and language/runtime packages for code ZefC emits.

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

Symbol C++ Library
==================

License
-------
This library includes the [Hsieh Super Fast Hash][HSFH] function, which is
licensed under the LGPL 2.1 license.  I've included the required GPL and LGPL
license files necessary to redistribute it. Since this library depends
crucially on an LGPL component, it too is licensed under LGPL.

[HSFH]: http://www.azillionmonkeys.com/qed/hash.html

Prior Art & Motivation
----------------------
When runtime flexibility is more important than compile-time type checking,
strings are often used as unique identifiers. For example, rather than
defining intergal constants RED, BLUE, GREEN, etc., you might colors around
as strings "red", "blue", "green". This makes it easy to store them in config
files, debug them, and manipulate them at runtime, but means that you have to
handle errors like "bleen" at runtime. One disadvantage of this technique
is that comparing, sorting, and searching for strings is rather slower than
doing the same for numbers, and can waste memory if defensive copies are used.

One way to solve these problems while keeping the runtime flexibility is
to register unique, immutable strings in a single shared registry. Everybody
is using pointers to the same strings so comparisions (at least for equality)
can be made quickly, and the registry makes sure only a single copy of each
string is used. This is called "string interning."

However, string interning comes with its own set of problems. Making the
registry thread-safe is a challenge and reduces performance, and interned 
strings cannot be easily shared between processes or serialized for later 
usage. The registry itself can eat up memory as many one-use strings are 
stored forever, or impact performance if it needs garbage collection.

The key insight behind the Symbol library is that the registry (the pool of the
originals of the interned strings), is itself unnecessary if you're willing
to give up perfect reversability. By giving up the registry, we automatically
remove thread and process syncronization concerns. Since we don't need a lookup
table, we can serialize, precompute, or pass around Symbols to our hearts
content. The mere fact that we're gaining functionality by removing features 
suggests this is an avenue worth exploring.

Now, if we were willing to give up reversability *completely*, we could just 
hash the strings to some unique identifier. However, that that makes debugging 
very difficult indeed, and if someone did need reversabilty for some reason
they would simply be out of luck.

The symbol library provides a compromise: you restrict yourself to C identifiers
(mixed-case alphanumeric characters plus underscores), and you can have perfect
reversability for identifiers of length 10 or less and partial reversability for
longer identifiers. In both cases the resulting Symbol fits into a single
64-bit integer: a particularly simple and high-performance type to work with.
Clients that require reversability can simply restrict themselves to
identifiers under 10 characters: annoying, but not a deal breaker.

What do you *DO* with a library like this?  The next obvious step would be to
provide a key/value data structure, although in C++ std::map<Symbol, T> would
probably work fine. Such a map could be used as a symbol table, perhaps for a
custom language. Or, since symbols can be reliably encoded across different
machines, you could use it as the basis of a binary wire protocol or file
format. The uses of Symbols are essentially the same for those as for interned
strings: keys in key/value pair objects used by dynamic languages, binary file
formats, RPC calls, etc., anywhere performance and memory are critical.

Usage
-----
Build with `make`. Use `make test` to run the unit tests. To use the library,
include symbol.h and link against symbol.a. All functionality is inside
the `symbol` namespace. The primary class is `symbol::Symbol`, a comparable
value type. Symbols fit in 64 bits (`sizeof(symbol::Symbol) == 8`) so there's
no reason to convert them to codes to store them efficently.

The libary and documentation follows a clear nomenclature: *identifiers*
(std::string) are *encoded* to *symbols* (symbol::Symbol) and symbols are
*decoded* to identifiers. The *code* (uint64_t) of a symbol is it's 64-bit 
representation. Generally, identifiers would be used for the plain-text
representation, the code would be used for the binary representation, and
Symbol objects would be used in local memory when working with symbols.

See symbol.h for API details. Here are some examples:

```C++
    using symbol::Symbol;
    std::string identifier = "name";
    Symbol s1 = symbol::encode(identifier);

    // you can also use the implicit constructors from unsigned longs and 
    // strings.  There is no default constructor. 
    
    // the empty string is a special case:
    Symbol("").code() == 0;

    // symbols have value semantics and can be copied and assigned naturally:
    Symbol s2 = s1; 

    // prints "name == name is true"
    std::cout << s1 << " == " << s2 << " is " << 
        std::boolalpha << (s1 == s2) << std::endl;
    // all comparison operators 

    // prints the numeric, coded value of the symbol
    std::cout << s1 << "'s code is " << s1.code() << std::endl;

    // there are several ways to decode to a string:
    symbol::decode(s1) == s1.decode();

    // including implicit conversion:
    std::string("") + s1 == std::string(s1);

    // legal identifiers consist only of upper- and lowercase letters,
    // digits, and underscores, e.g. "Test_Code_321". A SymbolError
    // with be thrown if you attempt to encode an illegal value:
    try { 
        Symbol("I'm illegal!");
    } catch ( symbol::SymbolError& e ) {
        // ...
    }

    // sometimes it's more intuitive to validate an identifier directly:
    if ( symbol::validate(unknownIdentifier) ) ...
```


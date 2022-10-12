
# RefCountingObject system for AngelScript

_Created 2022 by Petr Ohlídal_

_https://github.com/only-a-ptr/RefCountingObject-AngelScript_

This mini-framework lets you use a single reference counting mechanism for both
AngelScript's built-in garbage collection and C++ smart pointers. This allows you to fluently
pass objects between C++ and script context(s) without caring who created the object and where may
references be held.

## Motivation

I maintain [a game project](https://github.com/RigsOfRods/rigs-of-rods)
where AngelScript is an optional dependency
but at the same time plays an important gameplay role.
I wanted to expand the scripting interface by exposing
the internal objects and functions as directly as possible, but I didn't want to do
AngelScript refcounting manually on the C++ side.
I wanted a classic C++ smart pointer (like `std::shared_ptr<>`) that would do it for me,
and which would remain fully functional for C++ even if built without AngelScript.

## How to use

The project was developed against AngelScript 2.35.1; but any reasonably recent version should do.

To install, just copy the 'RefCountingObject\*' files to your project.

Define your C++ classes by implementing `RefCountingObject`
and calling `RegisterRefCountingObject()` for each type.

```
class Foo: RefCountingObject<Foo>{}
Foo::RegisterRefCountingObject("Foo", engine);
```

Define your C++ smart pointers by qualifying `RefCountingObjectPtr<>`
and use them in your interfaces. 
These will become usable interchangeably from both C++ and script.

```
typedef RefCountingObjectPtr<Foo> FooPtr;
// demo API:
static FooPtr gf;
static void SetFoo(FooPtr f) { gf = f; }
static FooPtr GetFoo() { return gf; }
```

Register the smart pointers with `RegisterRefCountingObjectPtr()`
and use them in your application interface.

```
FooPtr::RegisterRefCountingObjectPtr("FooPtr", "Foo", engine);
// ...
engine->RegisterGlobalFunction("void SetFoo(FooPtr@)", asFUNCTION(SetFoo), asCALL_CDECL);
engine->RegisterGlobalFunction("FooPtr@ GetFoo()", asFUNCTION(GetFoo), asCALL_CDECL);
```

In C++, use just the smart pointers and you'll be safe.

```
FooPtr f1 = new Foo(); // refcount 1
SetFoo(f1);            // refcount 2
FooPtr f2 = GetFoo();  // refcount 3
f2 = nullptr;          // refcount 2
f1 = nullptr;          // refcount 1
SetFoo(nullptr);       // refcount 0 -> deleted.
```

In AngelScript, use the native handles.

```
Foo@ f1 = new Foo();   // refcount 1
SetFoo(f1);            // refcount 2
Foo@ f2 = GetFoo();    // refcount 3
@f2 = null;            // refcount 2
@f1 = null;            // refcount 1
SetFoo(null);          // refcount 0 -> deleted.
```

## How it works

This part explains the less obvious bits of AngelScript mechanics.

### How the AngelScript refcounting works

 Intuitively, you would call AddRef() every time you obtain a raw pointer to the object, 
 i.e. when creating a new object or retrieving one from script engine. Well, don't.
 
 * New objects have refcount initialized to 1.
   You must call Release() to dispose of it, but don't call AddRef() unless you're creating additional pointers.
 * When passing the object as parameter from script to C++ function, the refcount is already incremented by the script engine.
   If you don't store the pointer, you must call Release() before the function returns.
 * When passing the object from C++ to script, you must increase the refcount yourself.
 
Documentation: 
   https://www.angelcode.com/angelscript/sdk/docs/manual/doc_as_vs_cpp_types.html#doc_as_vs_cpp_types_3
   https://www.angelcode.com/angelscript/sdk/docs/manual/doc_obj_handle.html#doc_obj_handle_3
   
### How RefCountingObject fits into it

RefCountingObject just implements the essential `AddRef()` and `Release()` functions needed by reference types:
https://www.angelcode.com/angelscript/sdk/docs/manual/doc_reg_basicref.html

RefCountingObjectPtr, for the most part, does the typical smart pointer scheme:
when created, add ref; when deleted, remove ref.

However, there is one major catch: constructing new objects. Since RefCountingObjectPtr must
be intuitive in C++, the only acceptable form of creating objects is this:
```
FooPtr foo_ptr = new Foo(); // C++ programmer reads "Pointer is taken care of".
```
With AngelScript, though, this means a memory leak because the smart pointer added a reference
while one already existed (the temporary one). The treatment expected by AngelScript is:
```
FooPtr foo_ptr = new Foo();
foo_ptr->Release(); // C++ programmer goes "WTF ?!?"
```
**The closure:** RefCountingObjectPtr never increases refcount when assigned a raw pointer (in C++).
Assignment within AngelScript engine is handled by wrapper interface, not available from C++.

  

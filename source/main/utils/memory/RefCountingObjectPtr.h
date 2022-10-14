
// RefCountingObject system for AngelScript
// Copyright (c) 2022 Petr Ohlidal
// https://github.com/only-a-ptr/RefCountingObject-AngelScript

// Adopted from "scripthandle" AngelScript addon, distributed with AngelScript SDK.

#pragma once

#include <angelscript.h>

#ifndef ROR_ASSERT
#   define ROR_ASSERT(_EXPR)  assert(_EXPR)
#endif

template<class T>
class RefCountingObjectPtr
{
public:
    // Constructors
    RefCountingObjectPtr();
    RefCountingObjectPtr(const RefCountingObjectPtr<T> &other);
    ~RefCountingObjectPtr();

    // Assignments
    RefCountingObjectPtr &operator=(const RefCountingObjectPtr<T> &other);
    // Intentionally omitting raw-pointer assignment, for simplicity - see raw pointer constructor.

    // Compare smart ptr
    bool operator==(const RefCountingObjectPtr<T> &o) const { return m_ref == o.m_ref; }
    bool operator!=(const RefCountingObjectPtr<T> &o) const { return m_ref != o.m_ref; }

    // Compare pointer
    bool operator==(const T* o) const { return m_ref == o; }
    bool operator!=(const T* o) const { return m_ref != o; }

    // Compare nullptr
    bool operator==(const nullptr_t) const { return m_ref == nullptr; }
    bool operator!=(const nullptr_t) const { return m_ref != nullptr; }

    // Get the reference
    T *GetRef() { return m_ref; }
    T* operator->() { return m_ref; }
    T* operator->() const { return m_ref; }

    // Boolean conversion (classic pointer check)
    operator bool() const { return (bool)m_ref; }

    // GC callback
    void EnumReferences(AngelScript::asIScriptEngine *engine);
    void ReleaseReferences(AngelScript::asIScriptEngine *engine);

    static void RegisterRefCountingObjectPtr(AngelScript::asIScriptEngine* engine, const char* handle_name, const char* obj_name);

    /// It's CRITICAL that this function gets invoked only once for each object in memory,
    /// and subsequent shared pointers are created from the original shared pointer.
    /// Binding multiple times compromises the ref counting and WILL cause 'double free or corruption' bug!
    static RefCountingObjectPtr<T> Bind(T* ref) { return RefCountingObjectPtr<T>(ref); }

protected:
    explicit RefCountingObjectPtr(T* ref); // Only invoke directly using C++! AngelScript must use a wrapper.

    void Set(T* ref);
    void ReleaseHandle();
    void AddRefHandle();

    // Wrapper functions, to be invoked by AngelScript only!
    static void ConstructDefault(RefCountingObjectPtr<T> *self) { new(self) RefCountingObjectPtr(); }
    static void ConstructCopy(RefCountingObjectPtr<T> *self, const RefCountingObjectPtr &o) { new(self) RefCountingObjectPtr(o); }
    static void ConstructRef(RefCountingObjectPtr<T>* self, void** objhandle);
    static void Destruct(RefCountingObjectPtr<T> *self) { self->~RefCountingObjectPtr(); }
    static T* OpImplCast(RefCountingObjectPtr<T>* self);
    static RefCountingObjectPtr & OpAssign(RefCountingObjectPtr<T>* self, void** objhandle);
    static bool OpEquals(RefCountingObjectPtr<T>* self, void** objhandle);
    static T* DereferenceHandle(void** objhandle);

    T *m_ref;
};

template<class T>
void RefCountingObjectPtr<T>::RegisterRefCountingObjectPtr(AngelScript::asIScriptEngine* engine, const char* handle_name, const char* obj_name)
{
    using namespace AngelScript;

    int r;
    const size_t DECLBUF_MAX = 300;
    char decl_buf[DECLBUF_MAX];

    // With C++11 it is possible to use asGetTypeTraits to automatically determine the flags that represent the C++ class
    r = engine->RegisterObjectType(handle_name, sizeof(RefCountingObjectPtr), asOBJ_VALUE | asOBJ_ASHANDLE | asOBJ_GC | asGetTypeTraits<RefCountingObjectPtr>()); ROR_ASSERT( r >= 0 );

    // construct/destruct
    r = engine->RegisterObjectBehaviour(handle_name, asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(RefCountingObjectPtr::ConstructDefault), asCALL_CDECL_OBJFIRST); ROR_ASSERT( r >= 0 );
    snprintf(decl_buf, DECLBUF_MAX, "void f(%s @&in)", obj_name);
    r = engine->RegisterObjectBehaviour(handle_name, asBEHAVE_CONSTRUCT, decl_buf, asFUNCTION(RefCountingObjectPtr::ConstructRef), asCALL_CDECL_OBJFIRST); ROR_ASSERT( r >= 0 );
    snprintf(decl_buf, DECLBUF_MAX, "void f(const %s &in)", handle_name);
    r = engine->RegisterObjectBehaviour(handle_name, asBEHAVE_CONSTRUCT, decl_buf, asFUNCTION(RefCountingObjectPtr::ConstructCopy), asCALL_CDECL_OBJFIRST); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectBehaviour(handle_name, asBEHAVE_DESTRUCT, "void f()", asFUNCTION(RefCountingObjectPtr::Destruct), asCALL_CDECL_OBJFIRST); ROR_ASSERT( r >= 0 );

    // GC
    r = engine->RegisterObjectBehaviour(handle_name, asBEHAVE_ENUMREFS, "void f(int&in)", asMETHOD(RefCountingObjectPtr,EnumReferences), asCALL_THISCALL); ROR_ASSERT(r >= 0);
    r = engine->RegisterObjectBehaviour(handle_name, asBEHAVE_RELEASEREFS, "void f(int&in)", asMETHOD(RefCountingObjectPtr, ReleaseReferences), asCALL_THISCALL); ROR_ASSERT(r >= 0);

    // Cast
    snprintf(decl_buf, DECLBUF_MAX, "%s @ opImplCast()", obj_name);
    r = engine->RegisterObjectMethod(handle_name, decl_buf, asFUNCTION(RefCountingObjectPtr::OpImplCast), asCALL_CDECL_OBJFIRST); ROR_ASSERT( r >= 0 );

    // Assign
    snprintf(decl_buf, DECLBUF_MAX, "%s &opHndlAssign(const %s &in)", handle_name, handle_name);
    r = engine->RegisterObjectMethod(handle_name, decl_buf, asMETHOD(RefCountingObjectPtr, operator=), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    snprintf(decl_buf, DECLBUF_MAX, "%s &opHndlAssign(const %s @&in)", handle_name, obj_name);
    r = engine->RegisterObjectMethod(handle_name, decl_buf, asFUNCTION(RefCountingObjectPtr::OpAssign), asCALL_CDECL_OBJFIRST); ROR_ASSERT( r >= 0 );

    // Equals
    snprintf(decl_buf, DECLBUF_MAX, "bool opEquals(const %s &in) const", handle_name);
    r = engine->RegisterObjectMethod(handle_name, decl_buf, asMETHODPR(RefCountingObjectPtr, operator==, (const RefCountingObjectPtr &) const, bool), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    snprintf(decl_buf, DECLBUF_MAX, "bool opEquals(const %s @&in) const", obj_name);
    r = engine->RegisterObjectMethod(handle_name, decl_buf, asFUNCTION(RefCountingObjectPtr::OpEquals), asCALL_CDECL_OBJFIRST); ROR_ASSERT( r >= 0 );
}


// ---------------------------- Internals ------------------------------

template<class T>
T* RefCountingObjectPtr<T>::DereferenceHandle(void** objhandle)
{
    // Dereference the handle to get the object itself (see AngelScript SDK, addon 'generic handle', function `Assign()` or `Equals()`).
    return static_cast<T*>(*objhandle);
}

template<class T>
inline void RefCountingObjectPtr<T>::ConstructRef(RefCountingObjectPtr<T>* self, void** objhandle)
{
    T* ref = DereferenceHandle(objhandle);
    new(self)RefCountingObjectPtr(ref);

    // Increase refcount manually because constructor is designed for C++ use only.
    if (ref)
        ref->AddRef();
}

template<class T>
inline T* RefCountingObjectPtr<T>::OpImplCast(RefCountingObjectPtr<T>* self)
{
    T* ref = self->GetRef();
    ref->AddRef();
    return ref;
}

template<class T>
inline RefCountingObjectPtr<T> & RefCountingObjectPtr<T>::OpAssign(RefCountingObjectPtr<T>* self, void** objhandle)
{
    T* ref = DereferenceHandle(objhandle);
    self->Set(ref);
    return *self;
}

template<class T>
inline bool RefCountingObjectPtr<T>::OpEquals(RefCountingObjectPtr<T>* self, void** objhandle)
{
    T* ref = DereferenceHandle(objhandle);
    return self->GetRef() == ref;
}

template<class T>
inline RefCountingObjectPtr<T>::RefCountingObjectPtr()
    : m_ref(nullptr)
{
}

template<class T>
inline RefCountingObjectPtr<T>::RefCountingObjectPtr(const RefCountingObjectPtr<T> &other)
{
    m_ref = other.m_ref;
    AddRefHandle();
}

template<class T>
inline RefCountingObjectPtr<T>::RefCountingObjectPtr(T *ref)
{
    // Used directly from C++, DO NOT increase refcount!
    // It's already been done by constructor/factory/AngelScript (if retrieved from script context).
    // ------------------------------------------
    m_ref  = ref;
}

template<class T>
inline RefCountingObjectPtr<T>::~RefCountingObjectPtr()
{
    ReleaseHandle();
}

template<class T>
inline void RefCountingObjectPtr<T>::ReleaseHandle()
{
    if( m_ref )
    {
        m_ref->Release();
        m_ref = nullptr;
    }
}

template<class T>
inline void RefCountingObjectPtr<T>::AddRefHandle()
{
    if( m_ref )
    {
        m_ref->AddRef();
    }
}

template<class T>
inline RefCountingObjectPtr<T> &RefCountingObjectPtr<T>::operator =(const RefCountingObjectPtr<T> &other)
{
    Set(other.m_ref);
    return *this;
}

template<class T>
inline void RefCountingObjectPtr<T>::Set(T* ref)
{
    if( m_ref == ref )
        return;

    ReleaseHandle();
    m_ref  = ref;
    AddRefHandle();
}

template<class T>
inline void RefCountingObjectPtr<T>::EnumReferences(AngelScript::asIScriptEngine *inEngine)
{
    // If we're holding a reference, we'll notify the garbage collector of it
    if (m_ref)
        inEngine->GCEnumCallback(m_ref);
}

template<class T>
inline void RefCountingObjectPtr<T>::ReleaseReferences(AngelScript::asIScriptEngine * /*inEngine*/)
{
    // Simply clear the content to release the references
    Set(nullptr);
}

/*
MIT License

Copyright (c) 2022 Petr Ohlídal

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

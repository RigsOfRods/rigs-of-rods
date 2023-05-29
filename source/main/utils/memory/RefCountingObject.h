
// RefCountingObject system for AngelScript
// Copyright (c) 2022 Petr Ohlidal
// https://github.com/only-a-ptr/RefCountingObject-AngelScript
// See license (MIT) at the bottom of this file.

#pragma once

#include <angelscript.h>

#include <mutex> // Against accidental threaded access
#include "Application.h" // Provides access to AppContext
#include "AppContext.h" // Stores main thread ID for debug checking


#if !defined(RefCoutingObject_DEBUGTRACE)
#   define RefCoutingObject_DEBUGTRACE()
#endif

#if !defined(RefCountingObject_ASSERT)
#   include <cassert>
#   define RefCountingObject_ASSERT(_Expr_) assert(_Expr_)
#endif

/// Self reference-counting objects, as requred by AngelScript garbage collector.
template<class T> class RefCountingObject
{
public:
    RefCountingObject()
    {
        RefCoutingObject_DEBUGTRACE();
    }

    virtual ~RefCountingObject()
    {
        RefCoutingObject_DEBUGTRACE();
    }

    void AddRef()
    {
        // Detect and prevent accidental threaded access.
        RefCountingObject_ASSERT(RoR::App::GetAppContext()->GetMainThreadID() == std::this_thread::get_id());
        std::unique_lock<std::mutex> lock(m_refcount_mtx);
        m_refcount++;
        RefCoutingObject_DEBUGTRACE();
    }

    void Release()
    {
        // Detect and prevent accidental threaded access.
        RefCountingObject_ASSERT(RoR::App::GetAppContext()->GetMainThreadID() == std::this_thread::get_id());
        int nw_refcount = -1;
        {
            std::unique_lock<std::mutex> lock(m_refcount_mtx);
            m_refcount--;
            nw_refcount = m_refcount;
            RefCoutingObject_DEBUGTRACE();
        }
        if (nw_refcount == 0)
        {
            delete this; // commit suicide! This is legit in C++, but you must completely 100% positively read https://isocpp.org/wiki/faq/freestore-mgmt#delete-this
        }
    }

    static void  RegisterRefCountingObject(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, const char* name)
    {
        int r;

#if defined(AS_USE_NAMESPACE)
        using namespace AngelScript;
#endif

        // Registering the reference type
        r = engine->RegisterObjectType(name, 0, asOBJ_REF); RefCountingObject_ASSERT( r >= 0 );

        // Registering the addref/release behaviours
        r = engine->RegisterObjectBehaviour(name, asBEHAVE_ADDREF, "void f()", asMETHOD(T,AddRef), asCALL_THISCALL); RefCountingObject_ASSERT( r >= 0 );
        r = engine->RegisterObjectBehaviour(name, asBEHAVE_RELEASE, "void f()", asMETHOD(T,Release), asCALL_THISCALL); RefCountingObject_ASSERT( r >= 0 );
    }

    int m_refcount = 0;
    std::mutex m_refcount_mtx; // Against accidental threaded access
};

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

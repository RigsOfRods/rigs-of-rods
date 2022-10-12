
// RefCountingObject system for AngelScript
// Copyright (c) 2022 Petr Ohlidal
// https://github.com/only-a-ptr/RefCountingObject-AngelScript

#pragma once

#include "Application.h"
#include <angelscript.h>

/// Self reference-counting objects, as requred by AngelScript garbage collector.
template<class T> class RefCountingObject
{
public:
    RefCountingObject()
    {
    }

    virtual ~RefCountingObject()
    {
    }

    void AddRef()
    {
        m_refcount++;
    }

    void Release()
    {
        m_refcount--;
        if (m_refcount == 0)
        {
            delete this; // commit suicide! This is legit in C++
        }
    }

    static void  RegisterRefCountingObject(const char* name, AngelScript::asIScriptEngine *engine)
    {
        using namespace AngelScript;

        int r;
        // Registering the reference type
        r = engine->RegisterObjectType(name, 0, asOBJ_REF); ROR_ASSERT( r >= 0 );

        // Registering the addref/release behaviours
        r = engine->RegisterObjectBehaviour(name, asBEHAVE_ADDREF, "void f()", asMETHOD(T,AddRef), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
        r = engine->RegisterObjectBehaviour(name, asBEHAVE_RELEASE, "void f()", asMETHOD(T,Release), asCALL_THISCALL); ROR_ASSERT( r >= 0 );
    }

    int m_refcount = 1; // Initial refcount for any angelscript object.
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

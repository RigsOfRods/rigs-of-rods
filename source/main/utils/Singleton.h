/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "RoRPrerequisites.h"

// damn you "using namespace Ogre" that cause name conflict with Singleton

// 	friend class RoRSingleton<Settings>;

//////////////////////////////////////////////////////////////////////////
template <class T>
class RoRSingleton
{
    static T* _instance;
protected:
    RoRSingleton()
    {
    }

    virtual ~RoRSingleton() { _instance = 0; }
    void _free() { delete this; }
    RoRSingleton(const RoRSingleton&); // not implemented
    const RoRSingleton& operator=(const RoRSingleton&); // not implemented
public:
    inline static T& getSingleton()
    {
        if (!_instance)
            _instance = new T;
        return *_instance;
    }

    inline static T* getSingletonPtr()
    {
        if (!_instance)
            _instance = new T;
        return _instance;
    }

    inline static T* getSingletonPtrNoCreation()
    {
        return _instance;
    }

    static bool singletonExists(void) { return (_instance != 0); }
    static void freeSingleton() { getSingletonPtr()->_free(); }
};

template <class T>
T* RoRSingleton<T>::_instance = 0;

//////////////////////////////////////////////////////////////////////////
template <class T>
class RoRSingletonNoCreation
{
    static T* _instance;
protected:
    RoRSingletonNoCreation()
    {
    }

    virtual ~RoRSingletonNoCreation() { _instance = 0; }
    void _free() { delete this; }
    RoRSingletonNoCreation(const RoRSingletonNoCreation&); // not implemented
    const RoRSingletonNoCreation& operator=(const RoRSingletonNoCreation&); // not implemented
    void setSingleton(T* i) { _instance = i; }
public:
    inline static T& getSingleton()
    {
        MYASSERT(_instance);
        return *_instance;
    }

    inline static T* getSingletonPtr()
    {
        MYASSERT(_instance);
        return _instance;
    }

    inline static T* getSingletonPtrNoCreation()
    {
        return _instance;
    }

    static bool singletonExists(void) { return (_instance != 0); }
    static void freeSingleton() { getSingletonPtr()->_free(); }
};

template <class T>
T* RoRSingletonNoCreation<T>::_instance = 0;


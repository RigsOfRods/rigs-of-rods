/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2024 Petr Ohlidal

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

/// @file

#include "AngelScriptBindings.h"
#include "Application.h"

#include <angelscript.h>

#include <OgreRenderOperation.h>

#include <Overlay/OgreOverlaySystem.h>
#include <Overlay/OgreOverlayManager.h>
#include <Overlay/OgreOverlay.h>
#include <Overlay/OgreOverlayContainer.h>

using namespace Ogre;
using namespace AngelScript;

void RoR::RegisterOgreObjectsCommon(asIScriptEngine* engine)
{
    int r;

    // We start by registering some data types, so angelscript knows that they exist

    // Ogre::Degree
    r = engine->RegisterObjectType("degree", sizeof(Degree), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA | asOBJ_APP_CLASS_ALLFLOATS);
    ROR_ASSERT( r >= 0 );

    // Ogre::Radian
    r = engine->RegisterObjectType("radian", sizeof(Radian), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA | asOBJ_APP_CLASS_ALLFLOATS);
    ROR_ASSERT( r >= 0 );

    // Ogre::Vector2
    r = engine->RegisterObjectType("vector2", sizeof(Vector2), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA | asOBJ_APP_CLASS_ALLFLOATS);
    ROR_ASSERT( r >= 0 );

    // Ogre::Vector3
    r = engine->RegisterObjectType("vector3", sizeof(Vector3), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA | asOBJ_APP_CLASS_ALLFLOATS);
    ROR_ASSERT( r >= 0 );

    // Ogre::Quaternion
    r = engine->RegisterObjectType("quaternion", sizeof(Quaternion), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA | asOBJ_APP_CLASS_ALLFLOATS);
    ROR_ASSERT( r >= 0 );

    // Object properties

    r = engine->RegisterObjectProperty("vector3", "float x", offsetof(Ogre::Vector3, x)); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectProperty("vector3", "float y", offsetof(Ogre::Vector3, y)); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectProperty("vector3", "float z", offsetof(Ogre::Vector3, z)); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectProperty("vector2", "float x", offsetof(Ogre::Vector2, x)); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectProperty("vector2", "float y", offsetof(Ogre::Vector2, y)); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectProperty("quaternion", "float w", offsetof(Quaternion, w)); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectProperty("quaternion", "float x", offsetof(Quaternion, x)); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectProperty("quaternion", "float y", offsetof(Quaternion, y)); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectProperty("quaternion", "float z", offsetof(Quaternion, z)); ROR_ASSERT( r >= 0 );

}

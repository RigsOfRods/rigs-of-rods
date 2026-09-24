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

    // Ogre::ColourValue
    r = engine->RegisterObjectType("color", sizeof(ColourValue), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA | asOBJ_APP_CLASS_ALLFLOATS);
    ROR_ASSERT( r >= 0 );

    // Ogre::Box
    r = engine->RegisterObjectType("box", sizeof(Box), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA | asOBJ_APP_CLASS_ALLINTS);
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

    r = engine->RegisterObjectProperty("color", "float r", offsetof(ColourValue, r)); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectProperty("color", "float g", offsetof(ColourValue, g)); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectProperty("color", "float b", offsetof(ColourValue, b)); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectProperty("color", "float a", offsetof(ColourValue, a)); ROR_ASSERT( r >= 0 );

    r = engine->RegisterObjectProperty("box", "uint left", offsetof(Box, left)); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectProperty("box", "uint top", offsetof(Box, top)); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectProperty("box", "uint right", offsetof(Box, right)); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectProperty("box", "uint bottom", offsetof(Box, bottom)); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectProperty("box", "uint front", offsetof(Box, front)); ROR_ASSERT( r >= 0 );
    r = engine->RegisterObjectProperty("box", "uint back", offsetof(Box, back)); ROR_ASSERT( r >= 0 );

    // More data types - the low-level scene API, under namespace `Ogre`

    r = engine->SetDefaultNamespace("Ogre"); ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectType("TexturePtr", sizeof(TexturePtr), asOBJ_VALUE | asGetTypeTraits<TexturePtr>());
    ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectType("TextureManager", sizeof(TextureManager), asOBJ_REF | asOBJ_NOCOUNT);
    ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectType("Image", sizeof(Image), asOBJ_VALUE | asGetTypeTraits<Image>());
    ROR_ASSERT(r >= 0);

    // NOTE: The `*SharedPtr` is a deprecated alias of `*Ptr` in OGRE 14, but in the version we're using it doesn't exist yet.
    r = engine->RegisterObjectType("HardwarePixelBufferPtr", sizeof(HardwarePixelBufferSharedPtr), asOBJ_VALUE | asGetTypeTraits<HardwarePixelBufferSharedPtr>());
    ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectType("PixelBox", sizeof(PixelBox), asOBJ_VALUE | asGetTypeTraits<PixelBox>());
    ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectType("MaterialManager", sizeof(MaterialManager), asOBJ_REF | asOBJ_NOCOUNT);
    ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectType("MaterialPtr", sizeof(MaterialPtr), asOBJ_VALUE | asGetTypeTraits<MaterialPtr>());
    ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectType("Technique", sizeof(Technique), asOBJ_REF | asOBJ_NOCOUNT);
    ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectType("Pass", sizeof(Pass), asOBJ_REF | asOBJ_NOCOUNT);
    ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectType("TextureUnitState", sizeof(TextureUnitState), asOBJ_REF | asOBJ_NOCOUNT);
    ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectType("Timer", sizeof(Timer), asOBJ_VALUE | asGetTypeTraits<Timer>());
    ROR_ASSERT(r >= 0);

    r = engine->RegisterObjectType("GpuProgramParametersPtr", sizeof(GpuProgramParametersPtr), asOBJ_VALUE | asGetTypeTraits<GpuProgramParametersPtr>());
    ROR_ASSERT(r >= 0);

    // enums, also under namespace `Ogre`

    r = engine->RegisterEnum("ImageFilter"); ROR_ASSERT(r >= 0); // Only registering those which are in OGRE14 docs, even though our older version has more
    r = engine->RegisterEnumValue("ImageFilter", "FILTER_NEAREST", Image::Filter::FILTER_NEAREST); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("ImageFilter", "FILTER_LINEAR", Image::Filter::FILTER_LINEAR); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("ImageFilter", "FILTER_BILINEAR", Image::Filter::FILTER_BILINEAR); ROR_ASSERT(r >= 0);

    r = engine->RegisterEnum("HardwareBufferLockOptions"); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("HardwareBufferLockOptions", "HBL_NORMAL", HardwareBuffer::LockOptions::HBL_NORMAL); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("HardwareBufferLockOptions", "HBL_DISCARD", HardwareBuffer::LockOptions::HBL_DISCARD); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("HardwareBufferLockOptions", "HBL_READ_ONLY", HardwareBuffer::LockOptions::HBL_READ_ONLY); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("HardwareBufferLockOptions", "HBL_NO_OVERWRITE", HardwareBuffer::LockOptions::HBL_NO_OVERWRITE); ROR_ASSERT(r >= 0);
    r = engine->RegisterEnumValue("HardwareBufferLockOptions", "HBL_WRITE_ONLY", HardwareBuffer::LockOptions::HBL_WRITE_ONLY); ROR_ASSERT(r >= 0);

    r = engine->SetDefaultNamespace(""); ROR_ASSERT(r >= 0);

}

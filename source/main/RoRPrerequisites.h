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
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
// created on 30th of April 2010 by Thomas Fischer

#pragma once

#include <OgrePrerequisites.h>
#include <OgreStringConverter.h>

#include <MyGUI_Prerequest.h> // Forward declarations
#if MYGUI_VERSION >= 0x030201
#	define MYGUI_GET_SCANCODE(KEY) (KEY.getValue())
#else
#	define MYGUI_GET_SCANCODE(KEY) (KEY.toValue())
#endif

#include "ForwardDeclarations.h"
#include "GlobalEnvironment.h"
#include "ZeroedMemoryAllocator.h" // this is used quite a lot, so we include it here already
#include "BitFlags.h"

// some config for angelscript, doesnt matter if we compile with angelscript or not as its just a definition
#ifdef USE_ANGELSCRIPT
#ifndef AS_USE_NAMESPACE
#define AS_USE_NAMESPACE
#endif //AS_USE_NAMESPACE
#endif // USE_ANGELSCRIPT

// version information now residing in RoRVersion.h

// some shortcuts for us
#define TOSTRING(x)  Ogre::StringConverter::toString(x)
#define TOUTFSTRING(x)  ANSI_TO_UTF(Ogre::StringConverter::toString(x))
#define PARSEINT(x)  Ogre::StringConverter::parseInt(x)
#define PARSEREAL(x) Ogre::StringConverter::parseReal(x)
#define HydraxLOG(msg) Ogre::LogManager::getSingleton().logMessage("[Hydrax] " + Ogre::String(msg));

// debug asserts
// #define FEAT_DEBUG_ASSERT

#ifdef FEAT_DEBUG_ASSERT
# ifdef _WIN32
// __debugbreak will break into the debugger in visual studio
#  define MYASSERT(x)       do { if (x) { } else { LOGSAFE("***ASSERT FAILED: "+OGREFUNCTIONSTRING); __debugbreak(); }; } while(0)
# endif // _WIN32
#else //!FEAT_DEBUG_ASSERT
# define MYASSERT(x)         assert(x)
#endif //FEAT_DEBUG_ASSERT

// some platform fixes
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#define strnlen(str,len) strlen(str)
#endif

#define ACTOR_RESOURCE_GROUP "ActorRG"

#define CHARACTER_ANIM_NAME_LEN 25 // Restricted for networking

enum VisibilityMasks {
	DEPTHMAP_ENABLED  = BITMASK(1),
	DEPTHMAP_DISABLED = BITMASK(2),
	HIDE_MIRROR       = BITMASK(3),
};

extern GlobalEnvironment *gEnv;

enum LoaderType 
{ 
	LT_None, 
	LT_Terrain, 
	LT_Vehicle,
	LT_Truck, 
	LT_Car, 
	LT_Boat,
	LT_Airplane, 
	LT_Trailer,
	LT_Train, 
	LT_Load, 
	LT_Extension, 
	LT_Network,
	LT_NetworkWithBoat, 
	LT_Heli, 
	LT_SKIN,
	LT_AllBeam 
};

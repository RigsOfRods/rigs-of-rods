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

#include "ZeroedMemoryAllocator.h" // this is used quite a lot, so we include it here already
#include "BitFlags.h"
#include <OgreWindowEventUtilities.h>
#include <OgreOverlayManager.h>

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

#define CHARACTER_ANIM_NAME_LEN 10 // Restricted for networking

enum VisibilityMasks {
	DEPTHMAP_ENABLED  = BITMASK(1),
	DEPTHMAP_DISABLED = BITMASK(2),
	HIDE_MIRROR       = BITMASK(3),
};

enum LoaderType
{
    LT_None,
    LT_Terrain,   // Invocable from GUI; No script alias, used in main menu
    LT_Vehicle,   // Script "vehicle",   ext: truck car
    LT_Truck,     // Script "truck",     ext: truck car
    LT_Car,       // Script "car",       ext: car
    LT_Boat,      // Script "boat",      ext: boat
    LT_Airplane,  // Script "airplane",  ext: airplane
    LT_Trailer,   // Script "trailer",   ext: trailer
    LT_Train,     // Script "train",     ext: train
    LT_Load,      // Script "load",      ext: load
    LT_Extension, // Script "extension", ext: trailer load
    LT_Skin,      // No script alias, invoked automatically
    LT_AllBeam    // Invocable from GUI; Script "all",  ext: truck car boat airplane train load
};

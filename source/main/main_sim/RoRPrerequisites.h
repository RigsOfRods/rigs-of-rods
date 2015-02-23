/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

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

// Defines whether Checked Iterators are enabled. If defined as 1, unsafe iterator use causes a runtime error. If defined as 0, checked iterators are disabled.
// TBD - tdev
// needs to be consistent across _ALL_ libs and code, thus disabled
//#ifdef _WIN32
//#define _SECURE_SCL 0
//#endif // _WIN32

// add some ogre headers
#include <OgreAxisAlignedBox.h>
#include <OgreColourValue.h>
#include <OgreHeaderPrefix.h>
#include <OgreLogManager.h>
#include <OgreQuaternion.h>
#include <OgreString.h>
#include <OgreStringConverter.h>
#include <OgreUTFString.h>
#include <OgreVector2.h>
#include <OgreVector3.h>
#include <OgrePrerequisites.h>

#include "ForwardDeclarations.h"
#include "GlobalEnvironment.h"
#include "ZeroedMemoryAllocator.h" // this is used quite a lot, so we include it here already
#include "../common/BitFlags.h"

#include <MyGUI_Prerequest.h> // Forward declarations

// some config for angelscript, doesnt matter if we compile with angelscript or not as its just a definition
#ifdef USE_ANGELSCRIPT
#ifndef AS_USE_NAMESPACE
#define AS_USE_NAMESPACE
#endif //AS_USE_NAMESPACE
#endif // USE_ANGELSCRIPT

// macro that checks for the correct ogre version and aborts compilation if not correct
#if OGRE_VERSION < 0x010701
#error You need at least Ogre version 1.7.1, older versions are not supported
#endif

// version information now residing in RoRVersion.h

// some shortcuts for us
#define LOG(x)       Ogre::LogManager::getSingleton().logMessage(x)
#define LOGSAFE(x)   do { if (Ogre::LogManager::getSingletonPtr()) Ogre::LogManager::getSingleton().logMessage(x);  } while(0) // use this if you log whenever its unsure if Ogre was started properly beforehand
#define TOSTRING(x)  Ogre::StringConverter::toString(x)
#define TOUTFSTRING(x)  ANSI_TO_UTF(Ogre::StringConverter::toString(x))
#define PARSEINT(x)  Ogre::StringConverter::parseInt(x)
#define PARSEREAL(x) Ogre::StringConverter::parseReal(x)
#define HydraxLOG(msg) Ogre::LogManager::getSingleton().logMessage("[Hydrax] " + Ogre::String(msg));

#define OGREFUNCTIONSTRING  Ogre::String(__FUNCTION__)+" @ "+Ogre::String(__FILE__)+":"+TOSTRING(__LINE__)

// debug for pthread mutexes
// #define FEAT_DEBUG_MUTEX

#ifdef FEAT_DEBUG_MUTEX
# define MUTEX_LOCK(x)       do { LOGSAFE("***MUTEX-LOCK  : mutex "+TOSTRING((uintptr_t)x)+" in function "+OGREFUNCTIONSTRING); pthread_mutex_lock(x);   } while(0)
# define MUTEX_UNLOCK(x)     do { LOGSAFE("***MUTEX-UNLOCK: mutex "+TOSTRING((uintptr_t)x)+" in function "+OGREFUNCTIONSTRING); pthread_mutex_unlock(x); } while(0)
#else //!FEAT_DEBUG_MUTEX
# define MUTEX_LOCK(x)       pthread_mutex_lock(x);
# define MUTEX_UNLOCK(x)     pthread_mutex_unlock(x);
#endif //FEAT_DEBUG_MUTEX

// debug asserts
// #define FEAT_DEBUG_ASSERT

// STL Erase fix for dinkumware STL x|
#ifdef _MSC_VER
#define STL_ERASE(x,y) y = x.erase(y)
#else
#define STL_ERASE(x,y) x.erase(y++)
#endif // _MSC_VER

#ifdef FEAT_DEBUG_ASSERT
# ifdef _WIN32
// __debugbreak will break into the debugger in visual studio
#  define MYASSERT(x)       do { if (x) { } else { LOGSAFE("***ASSERT FAILED: "+OGREFUNCTIONSTRING); __debugbreak(); }; } while(0)
# endif // _WIN32
#else //!FEAT_DEBUG_ASSERT
# define MYASSERT(x)         assert(x)
#endif //FEAT_DEBUG_ASSERT


// replace standard allocations with nedmalloc
//#define REPLACE_SYSTEM_ALLOCATOR
//#include "nedmalloc.h"
//CAUTION, do not try to use normal free on nedmalloc'ed memory and the other way round
// if you are not sure that this replacement is consistent, better leave it out.


// some platform fixes
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#define strnlen(str,len) strlen(str)
#endif

enum VisibilityMasks {
	DEPTHMAP_ENABLED  = BITMASK(1),
	DEPTHMAP_DISABLED = BITMASK(2),
	HIDE_MIRROR       = BITMASK(3),
};

extern GlobalEnvironment *gEnv;

enum GameStates
{
	NONE_LOADED    = 0,
	TERRAIN_LOADED = 1, //!< Simulate terrain & wait for user to select vehicle.
	ALL_LOADED     = 2, //!< Run simulation.
	EXITING        = 3, //!< No effect, filled at exit, never checked.
	RELOADING      = 5, //!< Loading new rig.
	PAUSE          = 6
};

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

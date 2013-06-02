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
#ifndef __RoRPrerequisites_H_
#define __RoRPrerequisites_H_

// Defines whether Checked Iterators are enabled. If defined as 1, unsafe iterator use causes a runtime error. If defined as 0, checked iterators are disabled.
// TBD - tdev
// needs to be consistent across _ALL_ libs and code, thus disabled
//#ifdef WIN32
//#define _SECURE_SCL 0
//#endif //WIN32

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

#include "GlobalEnvironment.h"
#include "ZeroedMemoryAllocator.h" // this is used quite a lot, so we include it here already

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
# ifdef WIN32
// __debugbreak will break into the debugger in visual studio
#  define MYASSERT(x)       do { if (x) { } else { LOGSAFE("***ASSERT FAILED: "+OGREFUNCTIONSTRING); __debugbreak(); }; } while(0)
# endif //WIN32
#else //!FEAT_DEBUG_ASSERT
# define MYASSERT(x)         assert(x)
#endif //FEAT_DEBUG_ASSERT


// replace standard allocations with nedmalloc
//#define REPLACE_SYSTEM_ALLOCATOR
//#include "nedmalloc.h"
//CAUTION, do not try to use normal free on nedmalloc'ed memory and the other way round
// if you are not sure that this replacement is consistent, better leave it out.

// some tool to define the bitmasks. We use this, as it it far better readable (prevents errors)
#define BITMASK(x) (1 << (x-1))
// BITMASK(1) = 0x00000001 = 0b00....0001
// BITMASK(2) = 0x00000002 = 0b00....0010

class AeroEngine;
class Airbrake;
class Airfoil;
class AppState;
class Autopilot;
class Axle;
class Beam;
class BeamEngine;
class BeamThreadStats;
class Buoyance;
class CacheEntry;
class Character;
class ChatSystem;
class CmdKeyInertia;
class Collisions;
class ColoredTextAreaOverlayElement;
class Dashboard;
class DashBoard;
class DashBoardManager;
class DOFManager;
class DotSceneLoader;
class DustPool;
class Editor;
class Envmap;
class Flexable;
class FlexAirfoil;
class FlexBody;
class FlexMesh;
class FlexObj;
class HDRListener;
class ICameraBehavior;
class IWater;
class HeatHaze;
class HeightFinder;
class SurveyMapManager;
class SurveyMapEntity;
class MapTextureCreator;
class MaterialFunctionMapper;
class MaterialReplacer;
class MeshObject;
class Mirrors;
class Network;
class OverlayWrapper;
class PointColDetector;
class PositionStorage;
class ProceduralManager;
class Rail;
class RailGroup;
class Replay;
class RigsOfRods;
class Road2;
class Road;
class RoRFrameListener;
class ScopeLog;
class Screwprop;
class Skidmark;
class Skin;
class SkyManager;
class SlideNode;
class ShadowManager;
class SoundManager;
class SoundScriptInstance;
class SoundScriptManager;
class TerrainGeometryManager;
class TerrainHeightFinder;
class TerrainManager;
class TerrainObjectManager;
class ThreadPool;
class IThreadTask;
class TorqueCurve;
class TruckEditor;
class TruckHUD;
class Turboprop;
class VideoCamera;
class Water;

namespace Ogre
{
	class MovableText;
};

#ifdef USE_SOCKETW
class SWBaseSocket;
#endif //USE_SOCKETW

// some platform fixes
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#define strnlen(str,len) strlen(str)
#endif

// forward typedefs for structs

// little helper macro that should prevent typos and increase readability
// "FWDCLSTRUCT(node)" will be "typedef struct node node_t"
#define FWDCLSTRUCT(x) typedef struct x x##_t


FWDCLSTRUCT(node);
FWDCLSTRUCT(beam);
FWDCLSTRUCT(shock);
FWDCLSTRUCT(collcab_rate);
FWDCLSTRUCT(soundsource);
FWDCLSTRUCT(contacter);
FWDCLSTRUCT(rigidifier);
FWDCLSTRUCT(wheel);
FWDCLSTRUCT(vwheel);
FWDCLSTRUCT(ropable);
FWDCLSTRUCT(wing);
FWDCLSTRUCT(command);
FWDCLSTRUCT(rotator);
FWDCLSTRUCT(flare);
FWDCLSTRUCT(prop);
FWDCLSTRUCT(rope);
FWDCLSTRUCT(exhaust);
FWDCLSTRUCT(cparticle);
FWDCLSTRUCT(debugtext);
FWDCLSTRUCT(rig);
FWDCLSTRUCT(collision_box);
FWDCLSTRUCT(tie);
FWDCLSTRUCT(hook);
FWDCLSTRUCT(ground_model);
FWDCLSTRUCT(client);
FWDCLSTRUCT(authorinfo);

enum VisibilityMasks {
	DEPTHMAP_ENABLED  = BITMASK(1),
	DEPTHMAP_DISABLED = BITMASK(2),
	HIDE_MIRROR       = BITMASK(3),
};

extern GlobalEnvironment *gEnv;

#endif // __RoRPrerequisites_H_

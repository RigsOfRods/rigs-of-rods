
#pragma once

// STANDARD LIBRARY
#include <string>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <sstream>
#include <memory>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <stdexcept> 

// OGRE (3d engine)
#include <Ogre.h>
#include <OgreVector3.h> // not in Ogre.h
#include <OgreQuaternion.h> // not in Ogre.h
#include <OgreRTShaderSystem.h> // not in Ogre.h
#include <OgreFontManager.h> // not in Ogre.h
#include <OgreTerrain.h>
#include <OgreTerrainGroup.h>
#include <Terrain/OgreTerrainPaging.h>

// OIS (inputs)
#include <OIS.h>

// CAELUM (OGRE sky plugin)
#ifdef USE_CAELUM
#include <Caelum.h>
#endif

// MyGUI
#include <MyGUI.h> // required dep.

// OPENAL (audio)
#ifdef __APPLE__
  #include <OpenAL/al.h>
  #include <OpenAL/alc.h>
#else
  #include <AL/al.h>
  #include <AL/alc.h>
#endif // __APPLE__

// ANGELSCRIPT
#ifdef USE_ANGELSCRIPT
#include <angelscript.h>
#endif

// JSON
#include <json/json.h> // formally optional, but actually required

// PAGED GEOMETRY (OGRE terrain/vegetation plugin)
#ifdef USE_PAGED
#include "BatchPage.h"
#include "GrassLoader.h"
#include "ImpostorPage.h"
#include "PagedGeometry.h"
#include "TreeLoader2D.h"
#include "TreeLoader3D.h"
#endif //USE_PAGED

#ifdef ROR_USE_OGRE_1_9
#	include <Overlay/OgreOverlaySystem.h>
#endif
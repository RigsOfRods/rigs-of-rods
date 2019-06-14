#pragma once

#include "../../source/main/terrain/TerrainObjectManager.h"

#ifdef ROR_FAKES_IMPL

    void TerrainObjectManager::LoadTerrainObject(std::string const &,Ogre::Vector3 const &,Ogre::Vector3 const &,class Ogre::SceneNode *,std::string const &,std::string const &,bool,int,bool) {}
    void TerrainObjectManager::unloadObject(std::string const &) {}
#endif // ROR_FAKES_IMPL

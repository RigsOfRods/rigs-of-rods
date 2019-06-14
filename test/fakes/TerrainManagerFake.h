#pragma once

#include "../../source/main/terrain/TerrainManager.h"

#ifdef ROR_FAKES_IMPL
    TerrainManager::TerrainManager(void) {}
    TerrainManager::~TerrainManager(void) {}

    float TerrainManager::GetHeightAt(float,float) {return 0.f;}
    bool TerrainManager::HasPredefinedActors(void) {return false;}
    bool TerrainManager::LoadAndPrepareTerrain(std::string) {return false;}
#endif // ROR_FAKES_IMPL

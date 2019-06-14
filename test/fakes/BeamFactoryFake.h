#pragma once

#include "../../source/main/physics/BeamFactory.h"

#ifdef ROR_FAKES_IMPL
    std::string RoR::ActorManager::GetQuicksaveFilename(std::string) {return "";}
    bool RoR::ActorManager::LoadScene(std::string) {return false;}
    bool RoR::ActorManager::SaveScene(std::string) {return false;}    
    RoR::ActorManager::~ActorManager(void) {}
    RoR::ActorManager::ActorManager(void) {}
#endif // ROR_FAKES_IMPL

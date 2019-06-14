#pragma once

#include "../../source/main/resources/CacheSystem.h"

#ifdef ROR_FAKES_IMPL
    CacheEntry *  CacheSystem::FindEntryByFilename(std::string) { return nullptr; }
    bool          CacheSystem::CheckResourceLoaded(std::string&, std::string&) { return false; }
    void CacheSystem::UnloadActorFromMemory(std::string) {}
    bool CacheSystem::CheckResourceLoaded(std::string &) {return false;}
    std::shared_ptr<RoR::SkinDef> CacheSystem::FetchSkinDef(class CacheEntry *) { return nullptr; }
    CacheSystem::CacheSystem(void) {}

    CacheEntry::CacheEntry() {}
#endif // ROR_FAKES_IMPL

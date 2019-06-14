#pragma once

#include "../../source/main/resources/ContentManager.h"

#ifdef ROR_FAKES_IMPL
    void RoR::ContentManager::resourceStreamOpened(std::string const &,std::string const &,class Ogre::Resource *,class Ogre::SharedPtr<class Ogre::DataStream> &) {}
    bool RoR::ContentManager::resourceCollision(class Ogre::Resource *,class Ogre::ResourceManager *) {return false;}
    bool RoR::ContentManager::handleEvent(class Ogre::ScriptCompiler *,class Ogre::ScriptCompilerEvent *,void *) {return false;}
    Ogre::SharedPtr<Ogre::DataStream> RoR::ContentManager::resourceLoading(std::string const &, std::string const &, Ogre::Resource *) { return /*nullptr*/ Ogre::SharedPtr<Ogre::DataStream>(); }
    void RoR::ContentManager::LoadGameplayResources(void) {}
#endif // ROR_FAKES_IMPL

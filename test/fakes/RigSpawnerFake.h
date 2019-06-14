#pragma once

#include "../../source/main/physics/RigSpawner.h"

#include <string>

#ifdef ROR_FAKES_IMPL
    void         ActorSpawner::Setup(class Actor*, class std::shared_ptr<struct RigDef::File>, class Ogre::SceneNode*, class Ogre::Vector<3, float> const&) {}
    Actor*       ActorSpawner::SpawnActor(void) { return nullptr; }
    bool         ActorSpawner::AddModule(class std::basic_string<char, struct std::char_traits<char>, class std::allocator<char> > const&) { return false; }
    void         ActorSpawner::SetupDefaultSoundSources(class Actor *)                 {}/*static*/
    void         ActorSpawner::ComposeName(class RoR::Str<100>&, char const*, int, int) {}/*static*/
    std::string  ActorSpawner::GetSubmeshGroundmodelName(void) { return ""; }
    std::string  ActorSpawner::ProcessMessagesToString(void)   { return ""; }
#endif // ROR_FAKES_IMPL

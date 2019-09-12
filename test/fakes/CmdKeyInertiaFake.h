#pragma once

#include "../../source/main/physics/CmdKeyInertia.h"

#ifdef ROR_FAKES_IMPL
    RoR::CmdKeyInertia::CmdKeyInertia() {}
    int RoR::CmdKeyInertia::SetCmdKeyDelay(RoR::CmdKeyInertiaConfig& cfg, Ogre::Real, Ogre::Real, Ogre::String, Ogre::String) { return 0; }
    void RoR::CmdKeyInertia::ResetCmdKeyDelay() {}
    Ogre::Real RoR::CmdKeyInertia::CalcCmdKeyDelay(Ogre::Real cmdInput, Ogre::Real dt) { return 0.f; }
    void RoR::CmdKeyInertiaConfig::LoadDefaultInertiaModels(void) {}
#endif // ROR_FAKES_IMPL

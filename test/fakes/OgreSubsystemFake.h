#pragma once

#include "../../source/main/gfx/OgreSubsystem.h"
#include <OgreString.h>

#ifdef ROR_FAKES_IMPL
    RoR::OgreSubsystem::OgreSubsystem(void) {}
    RoR::OgreSubsystem::~OgreSubsystem(void) {}

    bool RoR::OgreSubsystem::StartOgre(Ogre::String const& hwnd, Ogre::String const& mainhwnd) { return false; }
#endif // ROR_FAKES_IMPL

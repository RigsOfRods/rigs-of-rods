#pragma once

#include "../../source/main/utils/Utils.h"

#include <OgreString.h>
#include <OgreMath.h>

#ifdef ROR_FAKES_IMPL
    Ogre::String RoR::Utils::Sha1Hash(std::string const&) { return ""; }
    Ogre::Real Round(Ogre::Real value, unsigned short ndigits /* = 0 */) { return 0.f; }
    void fixRenderWindowIcon(class Ogre::RenderWindow *) {}
    void UpdatePresence() {} // Discord rich presence
#endif // ROR_FAKES_IMPL

#pragma once

#include "../../source/main/utils/Utils.h"

#include <OgreString.h>
#include <OgreMath.h>

#ifdef ROR_FAKES_IMPL
    Ogre::String sha1sum(const char *key, int len) { return ""; }
    Ogre::Real Round(Ogre::Real value, unsigned short ndigits /* = 0 */) { return 0.f; }
    void fixRenderWindowIcon(class Ogre::RenderWindow *) {}
#endif // ROR_FAKES_IMPL

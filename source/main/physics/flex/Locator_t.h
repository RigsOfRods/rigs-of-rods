#pragma once

#include "SimData.h"

#include <OgreVector3.h>

namespace RoR {

struct Locator_t
{
    NodeIdx_t ref;
    NodeIdx_t nx;
    NodeIdx_t ny;
    NodeIdx_t nz;
    Ogre::Vector3 coords;
};

} // namespace RoR

#pragma once

#include <OgreVector3.h>
#include <SimData.h>

namespace RoR {

struct Locator_t
{
    NodeNum_t ref;
    NodeNum_t nx;
    NodeNum_t ny;
    NodeNum_t nz;
    Ogre::Vector3 coords;
};

} // namespace RoR

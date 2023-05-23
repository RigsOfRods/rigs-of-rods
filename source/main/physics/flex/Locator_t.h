#pragma once

#include <Ogre.h>
#include <SimData.h>

#include <algorithm>

namespace RoR {

struct Locator_t
{
    NodeNum_t ref;
    NodeNum_t nx;
    NodeNum_t ny;
    Ogre::Vector3 coords;

    NodeNum_t getSmallestNode() { return std::min(ref, std::min(nx, ny)); }
    NodeNum_t getBiggestNode() { return std::min(ref, std::min(nx, ny)); }
    NodeNum_t getMean() { return (ref + nx + ny) / 3; }
};

} // namespace RoR

#pragma once

#include <Ogre.h>
#include <SimData.h>

#include <algorithm>

namespace RoR {

    struct Locator_t
    {
        NodeNum_t ref = NODENUM_INVALID;
        NodeNum_t nx = NODENUM_INVALID;
        NodeNum_t ny = NODENUM_INVALID;
        Ogre::Vector3 coords = Ogre::Vector3::ZERO;
        bool is_forvert = false;

        NodeNum_t getSmallestNode() { return std::min(ref, std::min(nx, ny)); }
        NodeNum_t getBiggestNode() { return std::min(ref, std::min(nx, ny)); }
        NodeNum_t getMean() { return (ref + nx + ny) / 3; }
    };

    struct ForvertTempData //!< Node resolution must be done in `ActorSpawner`, but vert resolution in `FlexBody`
    {
        NodeNum_t nref = NODENUM_INVALID;
        NodeNum_t nx = NODENUM_INVALID;
        NodeNum_t ny = NODENUM_INVALID;
        int vert_index = 0;
    };

} // namespace RoR

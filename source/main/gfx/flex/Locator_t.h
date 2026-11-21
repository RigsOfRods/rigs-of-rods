#pragma once

#include <OgreVector3.h>
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

    /// REF/X/Y nodes shared by all flexbody vertices (of one actor) with the same locator nodes, see `GfxActor::RegisterFlexbodyBasis()`.
    struct FlexNodeTriplet
    {
        NodeNum_t fnt_ref = NODENUM_INVALID;
        NodeNum_t fnt_x = NODENUM_INVALID;
        NodeNum_t fnt_y = NODENUM_INVALID;
    };

    /// Affine basis of a `FlexNodeTriplet`, calculated once per frame (see `FlexBody::calcNodeBases()`).
    /// 4 floats per vector so the SSE kernel can load each with a single read; the 4th float is unused.
    struct FlexNodeBasis
    {
        float fnb_ref[4];     //!< REF node position
        float fnb_diff_x[4];  //!< X node minus REF node
        float fnb_diff_y[4];  //!< Y node minus REF node
        float fnb_normal[4];  //!< normalise(diff_x cross diff_y)
    };

    struct ForvertTempData //!< Node resolution must be done in `ActorSpawner`, but vert resolution in `FlexBody`
    {
        NodeNum_t nref = NODENUM_INVALID;
        NodeNum_t nx = NODENUM_INVALID;
        NodeNum_t ny = NODENUM_INVALID;
        int vert_index = 0;
    };

} // namespace RoR

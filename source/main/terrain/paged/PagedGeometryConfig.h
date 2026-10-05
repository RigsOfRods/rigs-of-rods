#ifndef PagedGeometryConfig_h__
#define PagedGeometryConfig_h__

/* Define the library version */
#define PAGEDGEOMETRY_VERSION_MAJOR 1
#define PAGEDGEOMETRY_VERSION_MINOR 3
#define PAGEDGEOMETRY_VERSION_PATCH 0
#define PAGEDGEOMETRY_VERSION       "1.3.0"

/* Define if we use ogre random */
/* #undef PAGEDGEOMETRY_USE_OGRE_RANDOM */

/* some helpful OIS macro */
/* #undef OIS_USING_DIR */

#include "Application.h"

namespace Forests
{
    /** \brief A technique used to render grass. Passed to `GrassLayer::setRenderTechnique()`.  */
    enum GrassTechnique
    {
        //RIGSOFRODS: DO NOT change numbers - serialized into TOBJ fileformat.

        /// Grass constructed of randomly placed and rotated quads
        GRASSTECH_QUAD = 0,
        /// Grass constructed of two quads forming a "X" cross shape
        GRASSTECH_CROSSQUADS = 1,
        /// Grass constructed of camera-facing billboard quads
        GRASSTECH_SPRITE = 2,

        // RIGSOFRODS: Limits for checking.
        GRASSTECH_MIN = 0,
        GRASSTECH_MAX = 2
    };

    inline GrassTechnique ValidateGrassTech(GrassTechnique src)
    {
        if (src >= GRASSTECH_MIN && src <= GRASSTECH_MAX)
        {
            return src;
        }
        else
        {
            LOG("[RoR] Invalid grass param 'technique', falling back to default '1' (GRASSTECH_CROSSQUADS)");
            return GRASSTECH_CROSSQUADS;
        }
    }

    /** \brief A technique used to fade grass into the distance. Passed to GrassLayer::setFadeTechnique().
    * RIGSOFRODS: Renumbered to match TOBJ fileformat.
    */
    enum FadeTechnique
    {
        // RIGSOFRODS: DO NOT change numbers - serialized into TOBJ fileformat.

        /// Grass that fades into the distance with transparency. Fairly effective in most cases.
        FADETECH_ALPHA = 2,
        /// Grass that fades in by "growing" up out of the ground. Very effective when grass fades in against the sky, or with alpha-rejected grass.
        FADETECH_GROW = 0,
        /// Grass that fades in by slowly becoming opaque while it "grows" up out of the ground. Effective with alpha grass fading in against the sky.
        FADETECH_ALPHAGROW = 1
    };


} // namespace Forests

#endif //PagedGeometryConfig_h__

// This file is part of the Caelum project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution.

#ifndef CAELUMPORT__CAELUM_PREREQUISITES_H
#define CAELUMPORT__CAELUM_PREREQUISITES_H

// Include external headers
#ifdef __APPLE__
#include "Ogre/Ogre.h"
#else
#include "Ogre.h"
#include "OgreBuildSettings.h"
#endif

#include <memory>


/** Caelum namespace
 *
 *  All of %Caelum is inside this namespace (except for macros).
 *
 *  @note: This was caelum with a lowercase 'c' in version 0.3
 */
namespace CaelumPort
{
    // Caelum needs a lot of precission for astronomical calculations.
    // Very few calculations use it, and the precission IS required.
    typedef double LongReal;

    // Use some ogre types.
    using Ogre::uint8;
    using Ogre::uint16;
    using Ogre::ushort;
    using Ogre::uint32;
    using Ogre::uint;

    using Ogre::Real;
    using Ogre::String;

    /// Resource group name for caelum resources.
    // RIGSOFRODS: Resources are loaded by `ContentManager`, name must match.
    static const String RESOURCE_GROUP_NAME = "SkyXRG";

    // Render group for caelum stuff
    // It's best to have them all together
    enum CaelumRenderQueueGroupId
    {
        CAELUM_RENDER_QUEUE_STARFIELD       = Ogre::RENDER_QUEUE_SKIES_EARLY + 0,
        CAELUM_RENDER_QUEUE_MOON_BACKGROUND = Ogre::RENDER_QUEUE_SKIES_EARLY + 1,
        CAELUM_RENDER_QUEUE_SKYDOME         = Ogre::RENDER_QUEUE_SKIES_EARLY + 2,
        CAELUM_RENDER_QUEUE_MOON            = Ogre::RENDER_QUEUE_SKIES_EARLY + 3,
        CAELUM_RENDER_QUEUE_SUN             = Ogre::RENDER_QUEUE_SKIES_EARLY + 4,
        CAELUM_RENDER_QUEUE_CLOUDS          = Ogre::RENDER_QUEUE_SKIES_EARLY + 5,
        CAELUM_RENDER_QUEUE_GROUND_FOG      = Ogre::RENDER_QUEUE_SKIES_EARLY + 6,
    };

    // Forward declarations
    class UniversalClock;
    class SkyDome;
    class BaseSkyLight;
    class Moon;
    class SphereSun;
    class SpriteSun;
    class ImageStarfield;
    class PointStarfield;
    class CloudSystem;
    class CaelumSystem;
    class FlatCloudLayer;
    class PrecipitationController;
    class PrecipitationInstance;
    class GroundFog;
    class DepthComposer;
    class DepthComposerInstance;
    class DepthRenderer;
}

#endif // CAELUM__CAELUM_PREREQUISITES_H

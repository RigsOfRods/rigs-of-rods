#pragma once
#ifndef __OGRE_PARTICLE_CUSTOM_PARAM_H__
#define __OGRE_PARTICLE_CUSTOM_PARAM_H__

#include <OgreParticle.h>

namespace Ogre {

/// @addtogroup Gfx
/// @{

/// @addtogroup Particle
/// @{

/// custom visual data for shader renderer
class ParticleCustomParam : public ParticleVisualData
{
public:
    ParticleCustomParam() : paramValue(0, 0, 0, 0)
    {
    }

    virtual ~ParticleCustomParam()
    {
    }

    Vector4 paramValue;
};

/// @} // addtogroup Particle
/// @} // addtogroup Gfx

} // namespace Ogre

#endif // __OGRE_PARTICLE_CUSTOM_PARAM_H__

#pragma once
#ifndef __OGRE_PARTICLE_CUSTOM_PARAM_H__
#define __OGRE_PARTICLE_CUSTOM_PARAM_H__

#include <OgreParticle.h>

namespace Ogre {
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
}

#endif // __OGRE_PARTICLE_CUSTOM_PARAM_H__

// This file is part of the Caelum project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution.


#include "CaelumPort/Sun.h"
#include "CaelumPort/InternalUtilities.h"

namespace CaelumPort
{


    const Ogre::String SpriteSun::SUN_MATERIAL_NAME = "CaelumSpriteSun";

    SpriteSun::SpriteSun (
            Ogre::SceneManager *sceneMgr,
            Ogre::SceneNode *caelumRootNode, 
            const Ogre::String &sunTextureName,
            const Ogre::Degree& sunTextureAngularSize
    ):
        BaseSkyLight(sceneMgr, caelumRootNode),
        mSunTextureAngularSize(sunTextureAngularSize)
    {
        Ogre::String uniqueSuffix = "/" + InternalUtilities::pointerToString (this);
    }

    SpriteSun::~SpriteSun () { }

    void SpriteSun::setBodyColour (const Ogre::ColourValue &colour) {
        BaseSkyLight::setBodyColour (colour);


    }

    void SpriteSun::setSunTexture (const Ogre::String &textureName) {
        //RIGSOFRODS: Nothing to do, sun is drawn by SkyX shader
    }

    void SpriteSun::setSunTextureAngularSize(const Ogre::Degree& sunTextureAngularSize){
        mSunTextureAngularSize = sunTextureAngularSize;
    }

    void SpriteSun::notifyCameraChanged (Ogre::Camera *cam) {
        // This calls setFarRadius
        BaseSkyLight::notifyCameraChanged(cam);

        // Set sun position.
        Ogre::Real sunDistance = mRadius - mRadius * Ogre::Math::Tan(mSunTextureAngularSize);
        mNode->setPosition (-mDirection * sunDistance);

        // Set sun scaling in [1.0 ~ 1.2] range
        float factor = 1.2f - mBodyColour.b * 0.2f;
        float scale = factor * sunDistance * Ogre::Math::Tan(mSunTextureAngularSize);
        mNode->setScale (Ogre::Vector3::UNIT_SCALE * scale);
    }
}

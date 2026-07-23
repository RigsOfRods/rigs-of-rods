// This file is part of the Caelum project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution.


#include "CaelumPort/CaelumExceptions.h"
#include "CaelumPort/InternalUtilities.h"
#include "CaelumPort/Moon.h"
#include <memory>

namespace CaelumPort
{
    const Ogre::String Moon::MOON_MATERIAL_NAME = "Caelum/PhaseMoon";
    const Ogre::String Moon::MOON_BACKGROUND_MATERIAL_NAME = "Caelum/MoonBackground";

    Moon::Moon (
            Ogre::SceneManager *sceneMgr,
            Ogre::SceneNode *caelumRootNode, 
            const Ogre::String& moonTextureName,
            Ogre::Degree angularSize
    ):
            BaseSkyLight(sceneMgr, caelumRootNode),
            mAngularSize(angularSize)
    {
        Ogre::String uniqueSuffix = "/" + InternalUtilities::pointerToString(this);




    }

    Moon::~Moon () {
    }

    void Moon::setBodyColour (const Ogre::ColourValue &colour) {
        BaseSkyLight::setBodyColour(colour);


    }

    void Moon::setMoonTexture (const Ogre::String &textureName)
    {
        //RIGSOFRODS: Removed materials and billboardsets, moon is drawn by SkyX.
    }

    void Moon::setMoonTextureAngularSize(const Ogre::Degree& moonTextureAngularSize){
        mAngularSize = moonTextureAngularSize;
    }

    void Moon::notifyCameraChanged (Ogre::Camera *cam) {
        // This calls setFarRadius
        BaseSkyLight::notifyCameraChanged(cam);

        // Set moon position.
        Ogre::Real moonDistance = mRadius - mRadius * Ogre::Math::Tan(mAngularSize);
        mNode->setPosition(-mDirection * moonDistance);

        // Set moon scaling in [1.0 ~ 1.2] range
        float factor = 1.2f - mBodyColour.b * 0.2f;
        float scale = factor * moonDistance * Ogre::Math::Tan(mAngularSize);
        mNode->setScale (Ogre::Vector3::UNIT_SCALE * scale);
    }

}

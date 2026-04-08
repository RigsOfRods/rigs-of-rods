// This file is part of the Caelum project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution.

#ifndef CAELUMPORT__MOON_H
#define CAELUMPORT__MOON_H

#include "CaelumPort/CaelumPrerequisites.h"
#include "CaelumPort/SkyLight.h"
#include "CaelumPort/FastGpuParamRef.h"

namespace CaelumPort
{
    /** Class representing the moon.
     *  Drawn as two billboards; one after the stars and one after the skydome.
     *  Drawing it before the skydome will make it invisible in daylight; and that's bad.
     */
    class  Moon:
            public BaseSkyLight
    {
    public:
        /// Name of the moon material.
        static const Ogre::String MOON_MATERIAL_NAME;

        /// Name of the moon background material.
        static const Ogre::String MOON_BACKGROUND_MATERIAL_NAME;

    private:
        //RIGSOFRODS: Removed materials and billboardsets, moon is drawn by SkyX.

        /// The moon sprite visible angle
        Ogre::Degree mAngularSize;



    public:
        /** Constructor.
         */
        Moon (
                Ogre::SceneManager *sceneMgr,
                Ogre::SceneNode *caelumRootNode,
                const Ogre::String& moonTextureName = "moon_disc.dds", 
                Ogre::Degree angularSize = Ogre::Degree(3.77f));

        virtual ~Moon ();

        /** Updates the moon material.
            @param textureName The new moon texture name.
         */
        void setMoonTexture (const Ogre::String &textureName);
        
        /** Updates the moon size.
            @param moon TextureAngularSize The new moon texture angular size.
         */
        void setMoonTextureAngularSize(const Ogre::Degree& moonTextureAngularSize);

        /** Sets the moon sphere colour.
            @param colour The colour used to draw the moon
         */
        void setBodyColour (const Ogre::ColourValue &colour);


    public:
        /// Handle camera change.
        virtual void notifyCameraChanged (Ogre::Camera *cam);


    };
}

#endif // CAELUM__MOON_H

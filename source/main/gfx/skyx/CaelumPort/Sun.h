// This file is part of the Caelum project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution.

#ifndef CAELUMPORT__SUN_H
#define CAELUMPORT__SUN_H

#include "CaelumPort/CaelumPrerequisites.h"
#include "CaelumPort/SkyLight.h"

namespace CaelumPort
{
    class BaseSkyLight;
    class SphereSun;
    class SpriteSun;

    typedef SpriteSun Sun;

    /** Class representing the sun as billboard with texture on it.
     */
    class  SpriteSun : public BaseSkyLight
    {
    public:
        /// Name of the sun material.
        static const Ogre::String SUN_MATERIAL_NAME;

    protected:

        //RIGSOFRODS: Removed material and billboardset, sun is drawn by SkyX shader
        
        /// The sun sprite visible angle
        Ogre::Degree mSunTextureAngularSize;

    public:
        /** Constructor.
            @param sceneMgr The scene manager where the lights will be created.
            @param sunTextureAngularSize 0.53f is real angular size of Sun and Moon, 3.77f is compatible with SphereSun
         */
        SpriteSun (
                Ogre::SceneManager *sceneMgr,
                Ogre::SceneNode *caelumRootNode,
                const Ogre::String& sunTextureName = "sun_disc.png", 
                const Ogre::Degree& sunTextureAngularSize = Ogre::Degree(3.77f));

        /** Destructor.
            @note If a sun position model is in use, it will be deleted.
         */
        virtual ~SpriteSun ();

        /** Updates the sun material.
            @param textureName The new sun texture name.
         */
        void setSunTexture (const Ogre::String &textureName);
        
        /** Updates the sun size.
            @param sunTextureAngularSize The new sun texture angular size.
         */
        void setSunTextureAngularSize(const Ogre::Degree& sunTextureAngularSize);

        /** Sets the sun sphere colour.
            @param colour The colour used to draw the sun
         */
        void setBodyColour (const Ogre::ColourValue &colour);

    public:
        /// Handle camera change.
        virtual void notifyCameraChanged (Ogre::Camera *cam);


    };
}

#endif // CAELUM__SUN_H

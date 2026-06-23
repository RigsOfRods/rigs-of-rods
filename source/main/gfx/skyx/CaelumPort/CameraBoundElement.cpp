// This file is part of the Caelum project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution.

#include "CaelumPort/CameraBoundElement.h"

namespace CaelumPort
{
    const Ogre::Real CameraBoundElement::CAMERA_NEAR_DISTANCE_MULTIPLIER = 10;

    CameraBoundElement::CameraBoundElement():
            mAutoRadius(true)
    {
    }

    CameraBoundElement::~CameraBoundElement()
    {
    }

    void CameraBoundElement::notifyCameraChanged (Ogre::Camera *cam) {
        if (mAutoRadius) {
            if (cam->getFarClipDistance () > 0) {
                setFarRadius((cam->getFarClipDistance () + cam->getNearClipDistance ()) / 2);
            } else {
                setFarRadius(cam->getNearClipDistance () * CAMERA_NEAR_DISTANCE_MULTIPLIER);
            }
        }   
    }

    void CameraBoundElement::forceFarRadius (Ogre::Real radius) {
        if (radius > 0) {
            mAutoRadius = false;
            setFarRadius(radius);
        } else {
            mAutoRadius = true;
        }
    }

    bool CameraBoundElement::getAutoRadius () const {
        return mAutoRadius;
    }

    void CameraBoundElement::setAutoRadius () {
        forceFarRadius (-1);
    }

    void CameraBoundElement::setFarRadius(Ogre::Real radius) {
    }
}

// This file is part of the Caelum project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution.

#ifndef CAELUMPORT__CAMERA_BOUND_ELEMENT_H
#define CAELUMPORT__CAMERA_BOUND_ELEMENT_H

#include "CaelumPort/CaelumPrerequisites.h"

namespace CaelumPort
{
    /** A camera-bound element.
     *
     *  This should be used as a base class for domes which follow the camera.
     *  It is only meant to be used inside Caelum.
     *
     *  By default this class work in autoradius mode; where it automatically
     *  resizes itself for camera near/far clipping radius. It will correctly
     *  handle infinite far clip planes.
     *
     *  This is meant to be used with depth_check and depth_write off.
     *  Trying to place an object "as far as possible" causes precision
     *  troubles; and was removed in version 0.4.
     *
     *  If far clip distance is finite the radius will be (near + far) / 2.
     *  If far clip distance is infinite (0) the radius will be 10 * near/
     */
    class  CameraBoundElement
    {
    private:
        /// Defines if the element has an automatic "far" radius or not.
        bool mAutoRadius;
        
    public:
        /** Constructor. Sets auto radius to true.
         */
        CameraBoundElement();

        /// Virtual Destructor.
        virtual ~CameraBoundElement ();

        /** Notify new camera conditions.
         *  This method notifies that a new camera is about to be used, so
         *  this element can follow it or perform other operations.
         *  The default implementation calls setRadius if in autoRadius mode.
         *  @param cam The new camera.
         */
        virtual void notifyCameraChanged (Ogre::Camera *cam) = 0;

        /** Forces the "far" size of the element to a specific radius.
         *
         *  If greater than zero this disables AutoRadius mode and forces a
         *  fixed radius. If this is negative or zero the radius is set
         *  automatically in notifyCameraChanged.
         *
         *  AutoRadius is turned on by default.
         *
         *  @param radius The positive radius of the element, or a 
         *          negative/zero value for AutoRadius mode.
         */
        void forceFarRadius (Ogre::Real radius);

        /** Checks if this element is in auto-radius mode.
         *  While in autoradius mode the element is automatically resized fit
         *  between the near and far radius.
         */
        bool getAutoRadius () const;

        /** Re-enable auto-radius; if disabled.
         *  Auto-radius is on by default; but can be disabled. This function
         *  can turn it back on.
         */
        void setAutoRadius ();

        /** Camera distances multiplier for the far clipping distance. 
         *  This threshold will be multiplied with the far clipping distance,
         *  if the camera doesn't use an infinite far clipping plane.
         */
        static const Ogre::Real CAMERA_FAR_DISTANCE_MULTIPLIER;

        /** Camera distances multiplier for the near clipping distance. 
         *  This threshold will be multiplied with the near clipping distance,
         *  if the camera does use an infinite far clipping plane.
         */
        static const Ogre::Real CAMERA_NEAR_DISTANCE_MULTIPLIER;

    protected:
        /** Abstract method to set the radius for this elements
         *  Derived classes should override this and resize their domes.
         *  The actual radius for the dome is controlled in the base class.
         */
        virtual void setFarRadius (Ogre::Real radius);
    };
}

#endif // CAELUM__CAMERA_BOUND_ELEMENT_H

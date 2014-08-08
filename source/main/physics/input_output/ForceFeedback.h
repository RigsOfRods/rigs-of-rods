/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __ForceFeedback_H_
#define __ForceFeedback_H_

#include "RoRPrerequisites.h"
#include "OISForceFeedback.h"

class ForceFeedback : public ZeroedMemoryAllocator
{
private:
	OIS::ForceFeedback* ffdevice;
	OIS::Effect* hydroEffect;
	Ogre::Real overall_gain;
	Ogre::Real stress_gain;
	Ogre::Real centering_gain;
	Ogre::Real camera_gain;
	bool enabled_state;
public:

	ForceFeedback(OIS::ForceFeedback* ffdevice, Ogre::Real overall_gain, Ogre::Real stress_gain, Ogre::Real centering_gain, Ogre::Real camera_gain);
	~ForceFeedback();

	/*we take here :
	  -roll and pitch inertial forces at the camera: this is not used currently, but it can be used for 2 axes force feedback devices, like FF joysticks, to render shocks
	  -wheel speed and direction command, for the artificial auto-centering (which is wheel speed dependant)
	  -hydro beam stress, the ideal data source for FF wheels
	 */
	void setForces(Ogre::Real roll, Ogre::Real pitch, Ogre::Real wspeed, Ogre::Real dircommand, Ogre::Real stress);
	void setEnabled(bool b);

};

#endif // __ForceFeedback_H_

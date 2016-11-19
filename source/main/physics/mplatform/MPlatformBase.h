/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

// This class is the skeleton of motion platform control in RoR.
// Please derive your own class from this to make it work with your motion platform!
// 
// Imre, Nagy Jr. (ror@nebulon.hu)

#pragma once

#ifdef USE_MPLATFORM

#include "RoRPrerequisites.h"

typedef struct // struct is used for motion platforms
{
    int time;

    float	x; // absolute coordinates
    float	y;
    float	z;

    float	x_acc; // accelerations regarding to different vectors
    float	y_acc;
    float	z_acc;

    float	head; // orientations
    float	roll;
    float	pitch;

    float	head_acc; // accelerations of orientations
    float	roll_acc;
    float	pitch_acc;

    float	steer; // user inputs
    float	throttle;
    float	brake;
    float	clutch;

    float	speed; // different stats
    float	rpm;
    int		gear;
    float	avg_friction;
} mstat_t;

class MPlatform_Base : public ZeroedMemoryAllocator
{
public:
    MPlatform_Base();
    virtual ~MPlatform_Base();

    virtual bool connect(); // initialize connection to motion platform
    virtual bool disconnect(); // clean up connection with motion platform

    // update motion platform. returning false if cannot update, like sending buffer is full
    virtual bool update(Ogre::Vector3 pos, Ogre::Quaternion quat, mstat_t statinfo);
    virtual bool update(float posx, float posy, float posz, float roll, float pitch, float head, float roll_acc, float pitch_acc, float head_acc);
};

#endif

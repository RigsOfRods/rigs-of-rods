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

#pragma once

#include <OgrePrerequisites.h>

#include "RoRPrerequisites.h"

struct differential_data_t
{
    Ogre::Real speed[2];
    Ogre::Real delta_rotation; // sign is first relative to the second
    Ogre::Real out_torque[2];
    Ogre::Real in_torque;
    Ogre::Real dt;
};

enum DiffType
{
    SPLIT_DIFF = 0,
    OPEN_DIFF,
    LOCKED_DIFF
};

class Axle : public ZeroedMemoryAllocator
{
public:
    Axle();

    int wheel_1; //! array location of wheel 1
    int wheel_2; //! array location of wheel 2
    //! difference of rotational position between two axles... a kludge at best
    Ogre::Real delta_rotation;
    //! torsion spring rate binding wheels together.
    Ogre::Real torsion_rate;
    Ogre::Real torsion_damp;

    void addDiffType(DiffType diff);
    void toggleDiff();
    void calcTorque( differential_data_t& diff_data );
    Ogre::UTFString getDiffTypeName();

    //! a differential that always splits the torque evenly, this is the original method
    static void calcSeperatedDiff( differential_data_t& diff_data);
    //! more power goes to the faster spining wheel
    static void calcOpenDiff( differential_data_t& diff_data );
    //! ensures both wheels rotate at the the same speed
    static void calcLockedDiff( differential_data_t& diff_data );

private:

    //! type of differential
    int which_diff;

    //! available diffs
    std::vector<DiffType> available_diff_method;
};


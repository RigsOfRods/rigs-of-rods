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

#ifdef USE_MPLATFORM
#include "MPlatformBase.h"

MPlatform_Base::MPlatform_Base()
{}

MPlatform_Base::~MPlatform_Base()
{}

bool MPlatform_Base::connect()
{
    return false;
}

bool MPlatform_Base::disconnect()
{
    return false;
}

bool MPlatform_Base::update(Ogre::Vector3 pos, Ogre::Quaternion quat, mstat_t statinfo)
{
    return false;
}

bool MPlatform_Base::update(float posx, float posy, float posz, float roll, float pitch, float head, float acc_roll, float acc_pitch, float acc_head)
{
    return false;
}

#endif // USE_MPLATFORM

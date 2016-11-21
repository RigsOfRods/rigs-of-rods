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
#include "MPlatformFD.h"

MPlatform_FD::MPlatform_FD()
{}

MPlatform_FD::~MPlatform_FD()
{}


bool MPlatform_FD::connect()
{
    
    if (mySocket.connect(12345, "127.0.0.1"))
    {
        LOG("Connected to feedback server: ");
        return true;
    }
    else
    {
        LOG("Unable to connect to feedback server: ");
        return false;
    }
}

bool MPlatform_FD::disconnect()
{
    return true;
}

bool MPlatform_FD::update(Ogre::Vector3 pos, Ogre::Quaternion quat, mstat_t statinfo)
{
    memset(&mStatinfo, 0, sizeof(mstat_t));

/* get global position	*/
    float x = pos.x;
    float y = pos.y;
    float z = pos.z;

/* get orientation */
    float head  = 0.0f;
    float pitch = 0.0f;
    float roll  = 0.0f;

/* get misc info */
    float throttle	= 0.0f;
    float brake		= 0.0f;
    float clutch	= 0.0f;
    float speed		= 0.0f;
    float steering	= 0.0f;

/* placebo values */

    mStatinfo.x = 1.23f;
    mStatinfo.y = 2.34f;
    mStatinfo.z = 3.45f;

//mySocket.send(buf, length, error);
    mySocket.send((char *)&mStatinfo, sizeof(mstat_t), &error);
//	if (error) {return false;}
    return true;
}

#endif // USE_MPLATFORM

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

#ifdef USE_MPLATFORM

#include "RoRPrerequisites.h"
#include "MPlatformBase.h"
#include "SocketW.h"

class MPlatform_FD : public MPlatform_Base
{
public:
    MPlatform_FD();
    ~MPlatform_FD();

    bool connect();
    bool disconnect();
    bool update(Ogre::Vector3 pos, Ogre::Quaternion quat, mstat_t statinfo);

private:
    mstat_t	mStatinfo;
    SWInetSocket mySocket;
    SWBaseSocket::SWBaseError error;
};

#endif // USE_MPLATFORM

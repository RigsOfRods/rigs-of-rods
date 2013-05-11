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
// created: 26th of March 201, thomas fischer thomas{AT}thomasfischer{DOT}biz
#ifdef USE_MUMBLE

#ifndef MUMBLEINTEGRATION
#define MUMBLEINTEGRATION

#include "RoRPrerequisites.h"
#include "Singleton.h"

#include <windows.h>

class MumbleIntegration : public RoRSingleton<MumbleIntegration>, public ZeroedMemoryAllocator
{
public:
	MumbleIntegration();
    void update(Ogre::Vector3 cameraPos, Ogre::Vector3 avatarPos);

protected:
	~MumbleIntegration();
	
	void initMumble();
	
	struct LinkedMem
	{
#ifdef WIN32
		UINT32	uiVersion;
		DWORD	uiTick;
#else
		uint32_t uiVersion;
		uint32_t uiTick;
#endif
		float	fAvatarPosition[3];
		float	fAvatarFront[3];
		float	fAvatarTop[3];
		wchar_t	name[256];
		float	fCameraPosition[3];
		float	fCameraFront[3];
		float	fCameraTop[3];
		wchar_t	identity[256];
#ifdef WIN32
		UINT32	context_len;
#else
		uint32_t context_len;
#endif
		unsigned char context[256];
		wchar_t description[2048];
	};

	LinkedMem *lm;
};

#endif //MUMBLEINTEGRATION

#endif //USE_MUMBLE

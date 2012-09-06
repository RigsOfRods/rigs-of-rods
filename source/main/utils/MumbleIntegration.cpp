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
// created: 26th of March 2011, thomas fischer thomas{AT}thomasfischer{DOT}biz

#ifdef USE_MUMBLE

#include "MumbleIntegration.h"

#include <Ogre.h>
#include "Settings.h"

using namespace Ogre;

// small utility function to convert a char string to a widechar string
int convertCharSet(wchar_t *wcstring, const char *s)
{
#ifdef WIN32
	// Convert to a wchar_t*
	size_t origsize = strlen(s) + 1;
	size_t convertedChars = 0;
	mbstowcs_s(&convertedChars, wcstring, origsize, s, _TRUNCATE);
	return (int)convertedChars;
#else
	// TODO
#endif //WIN32
}

MumbleIntegration::MumbleIntegration() : lm(NULL)
{
	initMumble();
}

MumbleIntegration::~MumbleIntegration()
{
}

void MumbleIntegration::initMumble()
{
#ifdef WIN32
	HANDLE hMapObject = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"MumbleLink");
	if (hMapObject == NULL)
		return;

	lm = (LinkedMem *) MapViewOfFile(hMapObject, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(LinkedMem));
	if (lm == NULL)
	{
		CloseHandle(hMapObject);
		hMapObject = NULL;
		return;
	}
#else
	char memname[256];
	snprintf(memname, 256, "/MumbleLink.%d", getuid());

	int shmfd = shm_open(memname, O_RDWR, S_IRUSR | S_IWUSR);

	if (shmfd < 0)
	{
		return;
	}

	lm = (LinkedMem *)(mmap(NULL, sizeof(struct LinkedMem), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd,0));

	if (lm == (void *)(-1))
	{
		lm = NULL;
		return;
	}
#endif
}

void MumbleIntegration::update(Ogre::Vector3 cameraPos, Ogre::Vector3 avatarPos)
{
	if (! lm) return;

	if (lm->uiVersion != 2)
	{
		wcsncpy(lm->name, L"Rigs of Rods", 256);
		wcsncpy(lm->description, L"This plugin links Rigs of Rods with Mumble", 2048);
		lm->uiVersion = 2;
	}
	lm->uiTick++;

	// Left handed coordinate system.
	// X positive towards "left".
	// Y positive towards "up".
	// Z positive towards "into screen".
	//
	// 1 unit = 1 meter

	// Unit vector pointing out of the avatars eyes (here Front looks into scene).
	lm->fAvatarFront[0] = 0.0f;
	lm->fAvatarFront[1] = 0.0f;
	lm->fAvatarFront[2] = 1.0f;

	// Unit vector pointing out of the top of the avatars head (here Top looks straight up).
	lm->fAvatarTop[0] = 0.0f;
	lm->fAvatarTop[1] = 1.0f;
	lm->fAvatarTop[2] = 0.0f;

	// Position of the avatar (here standing slightly off the origin)
	lm->fAvatarPosition[0] = avatarPos.x;
	lm->fAvatarPosition[1] = avatarPos.y;
	lm->fAvatarPosition[2] = avatarPos.z;

	// Same as avatar but for the camera.
	lm->fCameraPosition[0] = cameraPos.x;
	lm->fCameraPosition[1] = cameraPos.y;
	lm->fCameraPosition[2] = cameraPos.z;

	lm->fCameraFront[0] = 0.0f;
	lm->fCameraFront[1] = 0.0f;
	lm->fCameraFront[2] = 1.0f;

	lm->fCameraTop[0] = 0.0f;
	lm->fCameraTop[1] = 1.0f;
	lm->fCameraTop[2] = 0.0f;

	// Identifier which uniquely identifies a certain player in a context (e.g. the ingame Name).
	convertCharSet(lm->identity, SSETTING("Nickname", "Anonymous").c_str());

	// Context should be equal for players which should be able to hear each other positional and
	// differ for those who shouldn't (e.g. it could contain the server+port and team)
	// TODO: separate teams
	int teamID = 0;
	sprintf((char *)lm->context, "%s:%s|%d", SSETTING("Server name", "-").c_str(), SSETTING("Server port", "1337").c_str(), teamID);
	lm->context_len = (int)strnlen((char *)lm->context, 256);
}

#endif // USE_MUMBLE

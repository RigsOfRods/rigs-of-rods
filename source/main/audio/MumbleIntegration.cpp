/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

/// @file
/// @author Thomas Fischer (thomas{AT}thomasfischer{DOT}biz)
/// @date   26th of March 2011

#ifdef USE_MUMBLE

#include "MumbleIntegration.h"

#include "Application.h"

#include <Ogre.h>
#include <MyGUI_UString.h>

using namespace Ogre;

MumbleIntegration::MumbleIntegration() : lm(NULL)
{
    initMumble();
}

MumbleIntegration::~MumbleIntegration()
{
    if (lm != nullptr)
        delete lm;
}

void MumbleIntegration::initMumble()
{
#ifdef _WIN32
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
#endif // _WIN32
}

void MumbleIntegration::SetNonPositionalAudio()
{
    if (! lm)
        return;

    this->update(
        Ogre::Vector3::ZERO,
        Ogre::Vector3(0.0f, 0.0f, 1.0f),
        Ogre::Vector3(0.0f, 1.0f, 0.0f),
        Ogre::Vector3::ZERO,
        Ogre::Vector3(0.0f, 0.0f, 1.0f),
        Ogre::Vector3(0.0f, 1.0f, 0.0f));
}

void MumbleIntegration::update(Ogre::Vector3 cameraPos, Ogre::Vector3 cameraDir, Ogre::Vector3 cameraUp, Ogre::Vector3 avatarPos, Ogre::Vector3 avatarDir, Ogre::Vector3 avatarUp)
{
    if (! lm)
        return;

    if (lm->uiVersion != 2)
    {
        wcsncpy(lm->name, L"Rigs of Rods", 256);
        wcsncpy(lm->description, L"This plugin links Rigs of Rods with Mumble", 2048);
        lm->uiVersion = 2;
    }
    lm->uiTick++;

    // Left handed coordinate system ( http://wiki.mumble.info/index.php?title=Link#Coordinate_system )
    // X positive towards "right".
    // Y positive towards "up".
    // Z positive towards "front".
    //
    // 1 unit = 1 meter

    // OGRE uses right-handed coordinate system ( http://www.ogre3d.org/tikiwiki/tiki-index.php?page=Basic+Tutorial+1&structure=Tutorials )
    // X positive towards "right".
    // Y positive towards "up".
    // Z positive towards "back". => conversion necessary!
    //
    // 1 unit = 1 meter (in RoR)

    // We need unit vectors
    avatarDir.normalise();
    avatarUp.normalise();
    cameraDir.normalise();
    cameraUp.normalise();

    // Position of the avatar
    lm->fAvatarPosition[0] = avatarPos.x;
    lm->fAvatarPosition[1] = avatarPos.y;
    lm->fAvatarPosition[2] = -avatarPos.z;

    // Unit vector pointing out of the avatars eyes
    lm->fAvatarFront[0] = avatarDir.x;
    lm->fAvatarFront[1] = avatarDir.y;
    lm->fAvatarFront[2] = -avatarDir.z;

    // Unit vector pointing out of the top of the avatars head
    lm->fAvatarTop[0] = avatarUp.x;
    lm->fAvatarTop[1] = avatarUp.y;
    lm->fAvatarTop[2] = -avatarUp.z;

    // Same as avatar but for the camera.
    lm->fCameraPosition[0] = cameraPos.x;
    lm->fCameraPosition[1] = cameraPos.y;
    lm->fCameraPosition[2] = -cameraPos.z;

    lm->fCameraFront[0] = cameraDir.x;
    lm->fCameraFront[1] = cameraDir.y;
    lm->fCameraFront[2] = -cameraDir.z;

    lm->fCameraTop[0] = cameraUp.x;
    lm->fCameraTop[1] = cameraUp.y;
    lm->fCameraTop[2] = -cameraUp.z;

    // Identifier which uniquely identifies a certain player in a context (e.g. the ingame Name).
    MyGUI::UString player_name(RoR::App::mp_player_name->GetStr());
    wcsncpy(lm->identity, player_name.asWStr_c_str(), 256);

    // Context should be equal for players which should be able to hear each other _positional_ and
    // differ for those who shouldn't (e.g. it could contain the server+port and team)
    // This ensures that Mumble users in the same channel playing on different servers
    // don't hear each other positional since the positional information would be wrong

    // TODO: Right now we only create contexts based on server identification but
    // some servers allow players to play on different maps independently
    // so we should take that into account as well

    int teamID = 0; // RoR currently doesn't have any kind of team-based gameplay
    int port = RoR::App::mp_server_port->GetInt();
    port = (port != 0) ? port : 1337;
    sprintf((char *)lm->context, "%s:%d|%d", RoR::App::mp_server_host->GetStr().c_str(), port, teamID);
    lm->context_len = (int)strnlen((char *)lm->context, 256);
}

#endif // USE_MUMBLE

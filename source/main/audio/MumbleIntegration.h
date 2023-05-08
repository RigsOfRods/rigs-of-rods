/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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

#pragma once

#ifdef USE_MUMBLE

#include "Application.h"

#ifdef _WIN32
#include <windows.h>
#else
  #include <sys/mman.h>
  #include <fcntl.h> /* For O_* constants */
#endif // _WIN32

namespace RoR {

/// @addtogroup Audio
/// @{

class MumbleIntegration
{
public:
    MumbleIntegration();
    ~MumbleIntegration();
    
    void Update();

protected:
    void initMumble();
    void updateMumble(Ogre::Vector3 cameraPos, Ogre::Vector3 cameraDir, Ogre::Vector3 cameraUp, Ogre::Vector3 avatarPos, Ogre::Vector3 avatarDir, Ogre::Vector3 avatarUp);
    void SetNonPositionalAudio(); // for main menu

    struct LinkedMem
    {
#ifdef _WIN32
        UINT32 uiVersion;
        DWORD uiTick;
#else
        uint32_t uiVersion;
        uint32_t uiTick;
#endif // _WIN32
        float fAvatarPosition[3];
        float fAvatarFront[3];
        float fAvatarTop[3];
        wchar_t name[256];
        float fCameraPosition[3];
        float fCameraFront[3];
        float fCameraTop[3];
        wchar_t identity[256];
#ifdef _WIN32
        UINT32 context_len;
#else
        uint32_t context_len;
#endif // _WIN32
        unsigned char context[256];
        wchar_t description[2048];
    };

    LinkedMem* lm = nullptr;
};

/// @}

} // namespace RoR

#endif //USE_MUMBLE

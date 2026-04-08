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

#pragma once

#include "Application.h"
#include "CameraManager.h"

#include "gfx/skyx/SkyX.h"

namespace RoR {

/// @addtogroup Gfx
/// @{

static const std::string SKYX_DEFAULT_CONFIG_FILE = "SkyXDefault.skx"; // fallback, in game resources.
static const std::string SKYX_USER_CONFIG_FILE = "skyx.cfg"; // primary, in user's /config dir.

class SkyXManager
{
public:
    SkyXManager(Ogre::String configFile);
    ~SkyXManager();

    Ogre::Light* getMainLight();

    bool update( float dt );

    SkyX::SkyX* GetSkyX() { return mSkyX; }

    // CaelumPort updates ported from `SkyManager`
    void NotifyCaelumPortCameraChanged(Ogre::Camera* newCamera);
    void DetectTerrainLightmapUpdateFromCaelumPort();
    std::string GetCaelumPortPrettyTime();
    double GetCaelumPortTime();
    void SetCaelumPortTime(double time);
    Ogre::Light* GetCaelumPortMainLight();

protected:
    void DetectPlayerMovement(float dt);

    SkyX::SkyX* mSkyX = nullptr;
    Ogre::SceneNode* mGroupingSceneNode = nullptr;

    SkyX::CfgFileManager* mCfgFileManager = nullptr;

    CaelumPort::LongReal mLastLightmapUpdateCaelumClock = 0.0;
    ActorPtr mLastPlayerActor;
    CameraManager::CameraBehaviors mLastCameraBehavior = CameraManager::CAMERA_BEHAVIOR_INVALID;
    Ogre::Vector3 mLastPlayerPosition = Ogre::Vector3::ZERO;
};

/// @} // addtogroup Gfx

} // namespace RoR

/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2016 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "ForwardDeclarations.h"

struct Global
{

    enum AppState
    {
        APP_STATE_NONE,
        APP_STATE_BOOTSTRAP,
        APP_STATE_MAIN_MENU,
        APP_STATE_CHANGEMAP,
        APP_STATE_SIMULATION,
    };

    enum MultiplayerState
    {
        MP_STATE_NONE,
        MP_STATE_DISABLED,
        MP_STATE_CONNECTED,
    };

};

class GlobalEnvironment
{
public:

    GlobalEnvironment() :
          cameraManager(0)
        , collisions(0)
        , frameListener(0)
        , mainCamera(0)
        , mrTime(0)
        , player(0)
        , sceneManager(0)
        , sky(0)
        , surveyMap(0)
        , terrainManager(0)
        , threadPool(0)
        , multiplayer_state(Global::MP_STATE_DISABLED)
        , next_multiplayer_state(Global::MP_STATE_NONE)
        , app_state(Global::APP_STATE_BOOTSTRAP)
        , next_app_state(Global::APP_STATE_NONE)
    {}

    Ogre::Camera*       mainCamera;
    Ogre::SceneManager* sceneManager;

    CameraManager*      cameraManager;
    Character*          player;
    Collisions*         collisions;
    SurveyMapManager*   surveyMap;
    RoRFrameListener*   frameListener;
    SkyManager*         sky;
    TerrainManager*     terrainManager;
    ThreadPool*         threadPool;

    float                     mrTime;
    Global::MultiplayerState  multiplayer_state;      ///< Current state
    Global::MultiplayerState  next_multiplayer_state; ///< Requested state change
    Global::AppState          app_state;              ///< Current state
    Global::AppState          next_app_state;         ///< Requested state change
};

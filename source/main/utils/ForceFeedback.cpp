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

#include "ForceFeedback.h"

#include "Application.h"
#include "Actor.h"
#include "Console.h"
#include "GameContext.h"
#include "InputEngine.h"
#include "Language.h"

#include <OgreString.h>

namespace RoR {

ForceFeedback::ForceFeedback()
    : m_device(nullptr)
    , m_effect_id(-1)
    , m_device_id(-1)
    , m_enabled(false)
{
    memset(&m_effect, 0, sizeof(m_effect));
}

ForceFeedback::~ForceFeedback()
{
    if (m_device && m_effect_id >= 0)
    {
        SDL_HapticDestroyEffect(m_device, m_effect_id);
    }
}

void ForceFeedback::Setup()
{
    m_device = App::GetInputEngine()->getForceFeedbackDevice();
    if (!m_device)
    {
        return;
    }

    LOG("ForceFeedback: device found");

    // Try to disable auto-centering (may not be supported)
    if (SDL_HapticSetAutocenterEnabled(m_device, 0) != 0)
    {
        // Not supported on all platforms, ignore failure
    }

    // Set initial gain to 0 (will be enabled later)
    SDL_HapticSetGain(m_device, 0);

    // Set up constant force effect
    m_effect.type = SDL_HAPTIC_CONSTANT;
    m_effect.constant.direction.type = SDL_HAPTIC_CARTESIAN;
    m_effect.constant.direction.dir[0] = 0; // X axis
    m_effect.constant.direction.dir[1] = 0;
    m_effect.constant.direction.dir[2] = 0;
    m_effect.constant.length = SDL_HAPTIC_INFINITY;
    m_effect.constant.level = 0;
    m_effect.constant.attack_length = 0;
    m_effect.constant.attack_level = 0;
    m_effect.constant.fade_length = 0;
    m_effect.constant.fade_level = 0;

    m_effect_id = SDL_HapticNewEffect(m_device, &m_effect);
    if (m_effect_id < 0)
    {
        LOG("ForceFeedback: failed to create effect: " + std::string(SDL_GetError()));
        m_device = nullptr;
    }
}

void ForceFeedback::SetForces(float roll, float pitch, float wspeed, float dircommand, float stress)
{
    if (!m_device || m_effect_id < 0)
        return;

    float stress_gain = App::io_ffb_stress_gain->getFloat();
    float centering_gain = App::io_ffb_center_gain->getFloat();

    // Compute force in OIS convention: -10000 to +10000
    float ff = -stress * stress_gain + dircommand * 100.0f * centering_gain * wspeed * wspeed;
    if (ff > 10000.0f)  ff = 10000.0f;
    if (ff < -10000.0f) ff = -10000.0f;

    // Scale to SDL haptic level: -32768 to +32767
    Sint16 sdl_level = static_cast<Sint16>(ff * 32767.0f / 10000.0f);

    m_effect.constant.level = sdl_level;

    if (SDL_HapticUpdateEffect(m_device, m_effect_id, &m_effect) != 0)
    {
        // Fallback: destroy and recreate (needed on macOS)
        SDL_HapticDestroyEffect(m_device, m_effect_id);
        m_effect_id = SDL_HapticNewEffect(m_device, &m_effect);
    }
}

void ForceFeedback::SetEnabled(bool b)
{
    if (!m_device)
        return;

    if (b != m_enabled)
    {
        Uint16 gain = (b) ? static_cast<Uint16>(App::io_ffb_master_gain->getFloat() * 100.0f) : 0;
        SDL_HapticSetGain(m_device, gain);
    }
    m_enabled = b;
}

void ForceFeedback::Update()
{
    if (!m_device)
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_WARNING,
                                      _L("Disabling force feedback - no controller found"));
        App::io_ffb_enabled->setVal(false);
        return;
    }

    ActorPtr player_actor = App::GetGameContext()->GetPlayerActor();
    if (player_actor && player_actor->ar_driveable == TRUCK)
    {
        Ogre::Vector3 ff_vehicle = player_actor->GetFFbBodyForces();
        this->SetForces(
            -ff_vehicle.dotProduct(player_actor->GetCameraRoll()) / 10000.0,
             ff_vehicle.dotProduct(player_actor->GetCameraDir())  / 10000.0,
            player_actor->ar_wheel_speed,
            player_actor->ar_hydro_dir_command,
            player_actor->GetFFbHydroForces());
    }
}

} // namespace RoR

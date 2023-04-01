/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2015-2020 Petr Ohlidal

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

#include "Actor.h"

#include "EngineSim.h"
#include "InputEngine.h"

#include <Ogre.h>

using namespace RoR;

void Actor::cruisecontrolToggle()
{
    cc_mode = !cc_mode;

    if (cc_mode)
    {
        cc_target_speed = ar_avg_wheel_speed;
        cc_target_rpm = ar_engine->getRPM();
    }
    else
    {
        cc_target_speed = 0;
        cc_target_rpm = 0;
        cc_accs.clear();
    }
}

void Actor::UpdateCruiseControl(float dt)
{
    if ((ar_engine->getGear() > 0 && App::GetInputEngine()->getEventValue(EV_TRUCK_BRAKE) > 0.05f) ||
        (ar_engine->getGear() > 0 && cc_target_speed < cc_target_speed_lower_limit) ||
        (ar_engine->getGear() > 0 && ar_parking_brake) ||
        (ar_engine->getGear() < 0) ||
        !ar_engine->isRunning() ||
        !ar_engine->hasContact())
    {
        this->cruisecontrolToggle();
        return;
    }

    if (ar_engine->getGear() != 0 && App::GetInputEngine()->getEventValue(EV_TRUCK_MANUAL_CLUTCH) > 0.05f)
        return;

    float acc = ar_engine->getAccToHoldRPM();

    if (ar_engine->getGear() > 0)
    {
        // Try to maintain the target speed
        float power_weight_ratio = getTotalMass(true) / ar_engine->getEnginePower();
        acc += (cc_target_speed - ar_wheel_speed) * power_weight_ratio * 0.25;
    }
    else if (ar_engine->getGear() == 0) // out of gear
    {
        // Try to maintain the target rpm
        float speed_range = (ar_engine->getMaxRPM() - ar_engine->getMinRPM()) / 50.0f;
        acc += ar_engine->getEngineInertia() * (cc_target_rpm - ar_engine->getRPM()) / speed_range;
    }

    cc_accs.push_front(Ogre::Math::Clamp(acc, -1.0f, +1.0f));
    if (cc_accs.size() > 30)
    {
        cc_accs.pop_back();
    }

    float avg_acc = 0.0f;
    for (unsigned int i = 0; i < cc_accs.size(); i++)
    {
        avg_acc += cc_accs[i];
    }
    avg_acc /= cc_accs.size();

    ar_engine->autoSetAcc(Ogre::Math::Clamp(avg_acc, ar_engine->getAcc(), 1.0f));

    if (App::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_ACCL))
    {
        if (ar_engine->getGear() > 0)
        {
            cc_target_speed *= pow(2.0f, dt / 5.0f);
            cc_target_speed = std::max(cc_target_speed_lower_limit, cc_target_speed);
            if (sl_enabled)
            {
                cc_target_speed = std::min(cc_target_speed, sl_speed_limit);
            }
        }
        else if (ar_engine->getGear() == 0) // out of gear
        {
            cc_target_rpm *= pow(2.0f, dt / 5.0f);
            cc_target_rpm = std::min(cc_target_rpm, ar_engine->getMaxRPM());
        }
    }
    if (App::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_DECL))
    {
        if (ar_engine->getGear() > 0)
        {
            cc_target_speed *= pow(0.5f, dt / 5.0f);
            cc_target_speed = std::max(cc_target_speed_lower_limit, cc_target_speed);
        }
        else if (ar_engine->getGear() == 0) // out of gear
        {
            cc_target_rpm *= pow(0.5f, dt / 5.0f);
            cc_target_rpm = std::max(ar_engine->getMinRPM(), cc_target_rpm);
        }
    }
    if (App::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_READJUST))
    {
        cc_target_speed = std::max(ar_avg_wheel_speed, cc_target_speed);
        if (sl_enabled)
        {
            cc_target_speed = std::min(cc_target_speed, sl_speed_limit);
        }
        cc_target_rpm = ar_engine->getRPM();
    }

    if (cc_can_brake)
    {
        if (ar_avg_wheel_speed > cc_target_speed + 0.5f && !App::GetInputEngine()->getEventValue(EV_TRUCK_ACCELERATE))
        {
            float brake = (ar_avg_wheel_speed - cc_target_speed) * 0.5f;
            ar_brake = std::min(brake, 1.0f);
        }
    }
}

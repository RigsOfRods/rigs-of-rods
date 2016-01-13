/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2016 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

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

#include "LandVehicleSimulation.h"

#include "Application.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "Console.h"
#include "InputEngine.h"
#include "Language.h"
#include "SoundScriptManager.h"
#include "GUIManager.h"

using namespace RoR;

void LandVehicleSimulation::UpdateVehicle(Beam* curr_truck, float seconds_since_last_frame)
{
	using namespace Ogre;

	if (!curr_truck->replaymode)
	{
		if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LEFT_MIRROR_LEFT))
			curr_truck->leftMirrorAngle-=0.001;

		if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_LEFT_MIRROR_RIGHT))
			curr_truck->leftMirrorAngle+=0.001;

		if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_RIGHT_MIRROR_LEFT))
			curr_truck->rightMirrorAngle-=0.001;

		if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_RIGHT_MIRROR_RIGHT))
			curr_truck->rightMirrorAngle+=0.001;

	} // end of (!curr_truck->replaymode) block

	if (!curr_truck->replaymode)
	{
		// steering
		float tmp_left_digital  = RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_STEER_LEFT,  false, InputEngine::ET_DIGITAL);
		float tmp_right_digital = RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_STEER_RIGHT, false, InputEngine::ET_DIGITAL);
		float tmp_left_analog   = RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_STEER_LEFT,  false, InputEngine::ET_ANALOG);
		float tmp_right_analog  = RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_STEER_RIGHT, false, InputEngine::ET_ANALOG);

		float sum = -std::max(tmp_left_digital,tmp_left_analog)+ std::max(tmp_right_digital,tmp_right_analog);

		if (sum < -1) sum = -1;
		if (sum > 1) sum = 1;

		curr_truck->hydrodircommand = sum;

		if ((tmp_left_digital < tmp_left_analog) || (tmp_right_digital < tmp_right_analog))
		{
			curr_truck->hydroSpeedCoupling = false;
		} 
		else
		{
			curr_truck->hydroSpeedCoupling = true;
		}

        PowertrainState pw_state = curr_truck->powertrain->GetStateOnMainThread();
        PowertrainCommandQueue& cmd_queue = curr_truck->powertrain->GetQueueOnMainThread();
        auto* input_engine = RoR::Application::GetInputEngine();

        if (curr_truck->powertrain != nullptr)
        {
#ifdef USE_MYGUI
            if (pw_state.transmission_shiftmodes_toggled)
            {
                // force gui update
                curr_truck->triggerGUIFeaturesChanged();
                switch(pw_state.transmission_auto_shift_mode)
                {
                case Gearbox::SHIFTMODE_AUTOMATIC:
                    RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Automatic shift"), "cog.png", 3000);
                    RoR::Application::GetGuiManager()->PushNotification("Gearbox Mode:", "Automatic shift");
                    break;
                case Gearbox::SHIFTMODE_SEMIAUTO:
                    RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Manual shift - Auto clutch"), "cog.png", 3000);
                    RoR::Application::GetGuiManager()->PushNotification("Gearbox Mode:", "Manual shift - Auto clutch");
                    break;
                case Gearbox::SHIFTMODE_MANUAL:
                    RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Fully Manual: sequential shift"), "cog.png", 3000);
                    RoR::Application::GetGuiManager()->PushNotification("Gearbox Mode:", "Fully Manual: sequential shift");
                    break;
                case Gearbox::SHIFTMODE_MANUAL_STICK:
                    RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Fully manual: stick shift"), "cog.png", 3000);
                    RoR::Application::GetGuiManager()->PushNotification("Gearbox Mode:", "Fully manual: stick shift");
                    break;
                case Gearbox::SHIFTMODE_MANUAL_RANGES:
                    RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Fully Manual: stick shift with ranges"), "cog.png", 3000);
                    RoR::Application::GetGuiManager()->PushNotification("Gearbox Mode:", "Fully Manual: stick shift with ranges");
                    break;
                }
            }

            // Ranges
            if (pw_state.transmission_gear_range_selected == 1)
            {
                RoR::Application::GetConsole()->putMessage(
                    Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Low range selected"), "cog.png", 3000);
            } 
            else if (pw_state.transmission_gear_range_selected == 2)
            {
                RoR::Application::GetConsole()->putMessage(
                    Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Mid range selected"), "cog.png", 3000);
            } 
            else if (pw_state.transmission_gear_range_selected == 3)
            {
                RoR::Application::GetConsole()->putMessage(
                    Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("High range selected"), "cog.png", 3000);
            }

            // Axle
            if (pw_state.transmission_axlelock_toggled == -1)
            {
                RoR::Application::GetConsole()->putMessage(
                    Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, 
                    _L("No differential installed on current vehicle!"), "warning.png", 3000);
                RoR::Application::GetGuiManager()->PushNotification("Differential:", "No differential installed on current vehicle!");
            }
            else if (pw_state.transmission_axlelock_toggled == 1)
            {
                RoR::Application::GetConsole()->putMessage(
                    Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, 
                    _L("Differentials switched to: ") + curr_truck->getAxleLockName(), "cog.png", 3000);
                RoR::Application::GetGuiManager()->PushNotification("Differential:", "Differentials switched to: " + curr_truck->getAxleLockName());
            }
#endif //USE_MYGUI

            PowertrainInputStates inputs;
            inputs.Reset();

            inputs.acceleration            = input_engine->getEventValue(EV_TRUCK_ACCELERATE);
            inputs.brake                   = input_engine->getEventValue(EV_TRUCK_BRAKE);
            inputs.manual_clutch           = input_engine->getEventValue(EV_TRUCK_MANUAL_CLUTCH);

            inputs.autoshift_up            = input_engine->getEventBoolValueBounce(EV_TRUCK_AUTOSHIFT_UP);
            inputs.autoshift_down          = input_engine->getEventBoolValueBounce(EV_TRUCK_AUTOSHIFT_DOWN);
            inputs.toggle_contact          = input_engine->getEventBoolValueBounce(EV_TRUCK_TOGGLE_CONTACT);
            inputs.switch_shift_modes      = input_engine->getEventBoolValueBounce(EV_TRUCK_SWITCH_SHIFT_MODES);
            inputs.shift_up                = input_engine->getEventBoolValueBounce(EV_TRUCK_SHIFT_UP);
            inputs.shift_down              = input_engine->getEventBoolValueBounce(EV_TRUCK_SHIFT_DOWN);
            inputs.shift_neutral           = input_engine->getEventBoolValueBounce(EV_TRUCK_SHIFT_NEUTRAL);
            inputs.shift_lowrange          = input_engine->getEventBoolValueBounce(EV_TRUCK_SHIFT_LOWRANGE);
            inputs.shift_midrange          = input_engine->getEventBoolValueBounce(EV_TRUCK_SHIFT_MIDRANGE);
            inputs.shift_highrange         = input_engine->getEventBoolValueBounce(EV_TRUCK_SHIFT_HIGHRANGE);
            inputs.parking_brake_toggle    = input_engine->getEventBoolValueBounce(EV_TRUCK_PARKING_BRAKE);
            inputs.antilock_brake_toggle   = input_engine->getEventBoolValueBounce(EV_TRUCK_ANTILOCK_BRAKE);
            inputs.traction_control_toggle = input_engine->getEventBoolValueBounce(EV_TRUCK_TRACTION_CONTROL);
            inputs.cruise_control_toggle   = input_engine->getEventBoolValueBounce(EV_TRUCK_CRUISE_CONTROL);

            inputs.starter                 = input_engine->getEventBoolValue(EV_TRUCK_STARTER);
            inputs.shift_gear_reverse      = input_engine->getEventBoolValue(EV_TRUCK_SHIFT_GEAR_REVERSE);
            inputs.shift_neutral           = input_engine->getEventBoolValue(EV_TRUCK_SHIFT_NEUTRAL);
            inputs.cruise_control_accel    = input_engine->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_ACCL);
            inputs.cruise_control_decel    = input_engine->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_DECL);
            inputs.cruise_control_readjust = input_engine->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_READJUST);

            inputs.direct_shift[ 0]        = input_engine->getEventBoolValue(EV_TRUCK_SHIFT_GEAR01);
            inputs.direct_shift[ 1]        = input_engine->getEventBoolValue(EV_TRUCK_SHIFT_GEAR02);
            inputs.direct_shift[ 2]        = input_engine->getEventBoolValue(EV_TRUCK_SHIFT_GEAR03);
            inputs.direct_shift[ 3]        = input_engine->getEventBoolValue(EV_TRUCK_SHIFT_GEAR04);
            inputs.direct_shift[ 4]        = input_engine->getEventBoolValue(EV_TRUCK_SHIFT_GEAR05);
            inputs.direct_shift[ 5]        = input_engine->getEventBoolValue(EV_TRUCK_SHIFT_GEAR06);
            inputs.direct_shift[ 6]        = input_engine->getEventBoolValue(EV_TRUCK_SHIFT_GEAR07);
            inputs.direct_shift[ 7]        = input_engine->getEventBoolValue(EV_TRUCK_SHIFT_GEAR08);
            inputs.direct_shift[ 8]        = input_engine->getEventBoolValue(EV_TRUCK_SHIFT_GEAR09);
            inputs.direct_shift[ 9]        = input_engine->getEventBoolValue(EV_TRUCK_SHIFT_GEAR10);
            inputs.direct_shift[10]        = input_engine->getEventBoolValue(EV_TRUCK_SHIFT_GEAR11);
            inputs.direct_shift[11]        = input_engine->getEventBoolValue(EV_TRUCK_SHIFT_GEAR12);
            inputs.direct_shift[12]        = input_engine->getEventBoolValue(EV_TRUCK_SHIFT_GEAR13);
            inputs.direct_shift[13]        = input_engine->getEventBoolValue(EV_TRUCK_SHIFT_GEAR14);
            inputs.direct_shift[14]        = input_engine->getEventBoolValue(EV_TRUCK_SHIFT_GEAR15);
            inputs.direct_shift[15]        = input_engine->getEventBoolValue(EV_TRUCK_SHIFT_GEAR16);
            inputs.direct_shift[16]        = input_engine->getEventBoolValue(EV_TRUCK_SHIFT_GEAR17);
            inputs.direct_shift[17]        = input_engine->getEventBoolValue(EV_TRUCK_SHIFT_GEAR18);

            cmd_queue.input_states = inputs;
        } // end of (powertrain != nullptr)

        cmd_queue.input_states.toggle_axle_lock = input_engine->getEventBoolValueBounce(EV_TRUCK_TOGGLE_AXLE_LOCK);
    } // end of ->replaymode

#ifdef USE_OPENAL
	if (curr_truck->ispolice)
	{
		if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_HORN))
		{
			SoundScriptManager::getSingleton().trigToggle(curr_truck, SS_TRIG_HORN);
		}
	} 
	else
	{
		if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_HORN) && !curr_truck->replaymode)
		{
			SoundScriptManager::getSingleton().trigStart(curr_truck, SS_TRIG_HORN);
		} else
		{
			SoundScriptManager::getSingleton().trigStop(curr_truck, SS_TRIG_HORN);
		}
	}
#endif // OPENAL
}

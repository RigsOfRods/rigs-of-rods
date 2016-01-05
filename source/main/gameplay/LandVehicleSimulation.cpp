/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

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

void LandVehicleSimulation::UpdateCruiseControl(Beam* curr_truck, float dt)
{
    PowertrainState powertrain = curr_truck->powertrain->GetStateOnMainThread();
    PowertrainCommandQueue& cmd_queue = curr_truck->powertrain->GetQueueOnMainThread();

	if (RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_BRAKE) > 0.05f ||
		RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_MANUAL_CLUTCH) > 0.05f ||
		(curr_truck->cc_target_speed < curr_truck->cc_target_speed_lower_limit) ||
		(curr_truck->parkingbrake && powertrain.transmission_current_gear > 0) ||
		!powertrain.engine_is_running ||
		!powertrain.engine_starter_has_contact)
	{
		curr_truck->cruisecontrolToggle();
		return;
	}

    int curr_gear = powertrain.transmission_current_gear;
	if (curr_gear > 0)
	{
		// Try to maintain the target speed
		if (curr_truck->cc_target_speed > curr_truck->WheelSpeed)
		{
			float accl = (curr_truck->cc_target_speed - curr_truck->WheelSpeed) * 2.0f;
			accl = std::max(powertrain.current_acceleration, accl);
			accl = std::min(accl, 1.0f);
			cmd_queue.AddCommand(PowertrainCommand::COMMAND_SET_ACC, accl);
		}
	} 
    else if (curr_gear == 0) // out of gear
	{
		// Try to maintain the target rpm
		if (curr_truck->cc_target_rpm > powertrain.engine_current_rpm)
		{
			float accl = (curr_truck->cc_target_rpm - powertrain.engine_current_rpm) * 0.01f;
			accl = std::max(powertrain.current_acceleration, accl);
			accl = std::min(accl, 1.0f);
			cmd_queue.AddCommand(PowertrainCommand::COMMAND_SET_ACC, accl);
		}
	}

	if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_ACCL))
	{
		if (curr_gear > 0)
		{
			curr_truck->cc_target_speed += 2.5f * dt;
			curr_truck->cc_target_speed  = std::max(curr_truck->cc_target_speed_lower_limit, curr_truck->cc_target_speed);
			if (curr_truck->sl_enabled)
			{
				curr_truck->cc_target_speed  = std::min(curr_truck->cc_target_speed, curr_truck->sl_speed_limit);
			}
		}
        else if (curr_gear == 0) // out of gear
		{
			curr_truck->cc_target_rpm += 1000.0f * dt;
			curr_truck->cc_target_rpm  = std::min(curr_truck->cc_target_rpm, powertrain.conf_engine_max_rpm);
		}
	}
	if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_DECL))
	{
		if (curr_gear > 0)
		{
			curr_truck->cc_target_speed -= 2.5f * dt;
			curr_truck->cc_target_speed  = std::max(curr_truck->cc_target_speed_lower_limit, curr_truck->cc_target_speed);
		}
        else if (curr_gear == 0) // out of gear
		{
			curr_truck->cc_target_rpm -= 1000.0f * dt;
			curr_truck->cc_target_rpm  = std::max(powertrain.conf_engine_min_rpm, curr_truck->cc_target_rpm);
		}
	}
	if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_CRUISE_CONTROL_READJUST))
	{
		curr_truck->cc_target_speed = std::max(curr_truck->WheelSpeed, curr_truck->cc_target_speed);
		if (curr_truck->sl_enabled)
		{
			curr_truck->cc_target_speed = std::min(curr_truck->cc_target_speed, curr_truck->sl_speed_limit);
		}
		curr_truck->cc_target_rpm   = powertrain.engine_current_rpm;
	}

	if (curr_truck->cc_can_brake)
	{
		if (curr_truck->WheelSpeed > curr_truck->cc_target_speed + 0.5f && !RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_ACCELERATE))
		{
			float brake = (curr_truck->WheelSpeed - curr_truck->cc_target_speed) * 0.5f;
			brake = std::min(brake, 1.0f);
			curr_truck->brake = curr_truck->brakeforce * brake;
		}
	}
}

void LandVehicleSimulation::CheckSpeedLimit(Beam* curr_truck, float dt)
{
    PowertrainState powertrain = curr_truck->powertrain->GetStateOnMainThread();
	if (curr_truck->sl_enabled && powertrain.transmission_current_gear != 0)
	{
		float accl = (curr_truck->sl_speed_limit - std::abs(curr_truck->WheelSpeed)) * 2.0f;
		accl = std::max(0.0f, accl);
		accl = std::min(accl, powertrain.current_acceleration);

        PowertrainCommandQueue& cmd_queue = curr_truck->powertrain->GetQueueOnMainThread();
        cmd_queue.AddCommand(PowertrainCommand::COMMAND_SET_ACC, accl);
	}
}

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

		if (curr_truck->powertrain != nullptr)
		{
            PowertrainState pw_state = curr_truck->powertrain->GetStateOnMainThread();
            PowertrainCommandQueue& cmd_queue = curr_truck->powertrain->GetQueueOnMainThread();

			bool arcadeControls = BSETTING("ArcadeControls", false);

			float accl  = RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_ACCELERATE);
			float brake = RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_BRAKE);

			// arcade controls are only working with auto-clutch!
			if (!arcadeControls || pw_state.transmission_auto_shift_mode > Gearbox::SHIFTMODE_SEMIAUTO)
			{
				// classic mode, realistic
                cmd_queue.AddCommand(PowertrainCommand::COMMAND_AUTO_SET_ACC, accl);
				curr_truck->brake = brake * curr_truck->brakeforce;
			} 
			else
			{
				// start engine
				if (pw_state.engine_starter_has_contact && !pw_state.engine_is_running && (accl > 0 || brake > 0))
				{
					cmd_queue.AddCommand(PowertrainCommand::COMMAND_START);
				}

				// arcade controls: hey - people wanted it x| ... <- and it's convenient
				if (pw_state.transmission_current_gear >= 0)
				{
					// neutral or drive forward, everything is as its used to be: brake is brake and accel. is accel.
					cmd_queue.AddCommand(PowertrainCommand::COMMAND_AUTO_SET_ACC, accl);
					curr_truck->brake = brake * curr_truck->brakeforce;
				} 
				else
				{
					// reverse gear, reverse controls: brake is accel. and accel. is brake.
					cmd_queue.AddCommand(PowertrainCommand::COMMAND_AUTO_SET_ACC, accl);
					curr_truck->brake = accl * curr_truck->brakeforce;
				}

				// only when the truck really is not moving anymore
				if (fabs(curr_truck->WheelSpeed) <= 1.0f)
				{
					float velocity = 0.0f;

					if (curr_truck->cameranodepos[0] >= 0 && curr_truck->cameranodedir[0] >= 0)
					{
						Vector3 hdir = (curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition - curr_truck->nodes[curr_truck->cameranodedir[0]].RelPosition).normalisedCopy();
						velocity = hdir.dotProduct(curr_truck->nodes[0].Velocity);
					}

					// switching point, does the user want to drive forward from backward or the other way round? change gears?
					if (velocity < 1.0f && brake > 0.5f && accl < 0.5f && pw_state.transmission_current_gear > 0)
					{
						// we are on the brake, jump to reverse gear
						if (pw_state.transmission_auto_shift_mode == Gearbox::SHIFTMODE_AUTOMATIC)
						{
                            cmd_queue.AddCommand(PowertrainCommand::COMMAND_AUTO_SHIFT_SET, static_cast<float>(Gearbox::AUTOSWITCH_REAR));
						} 
						else
						{
                            cmd_queue.AddCommand(PowertrainCommand::COMMAND_SET_GEAR, -1.f);
						}
					} 
                    else if (velocity > -1.0f && brake < 0.5f && accl > 0.5f && pw_state.transmission_current_gear < 0)
					{
						// we are on the gas pedal, jump to first gear when we were in rear gear
                        if (pw_state.transmission_auto_shift_mode == Gearbox::SHIFTMODE_AUTOMATIC)
						{									
							cmd_queue.AddCommand(PowertrainCommand::COMMAND_AUTO_SHIFT_SET, static_cast<float>(Gearbox::AUTOSWITCH_DRIVE));
						} 
						else
						{
							cmd_queue.AddCommand(PowertrainCommand::COMMAND_SET_GEAR, 1.f);
						}
					}
				}
			}

			// ============ Gearbox simulation - to be ported into LuaPowertrain ==================

			if (pw_state.transmission_auto_shift_mode == Gearbox::SHIFTMODE_AUTOMATIC)
			{
				if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_AUTOSHIFT_UP))
				{
                    cmd_queue.AddCommand(PowertrainCommand::COMMAND_AUTO_SHIFT_UP);
				}
				if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_AUTOSHIFT_DOWN))
				{
					cmd_queue.AddCommand(PowertrainCommand::COMMAND_AUTO_SHIFT_DOWN);
				}
			}

			if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_CONTACT))
			{
				cmd_queue.AddCommand(PowertrainCommand::COMMAND_TOGGLE_CONTACT);
			}

			if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_STARTER) && pw_state.engine_starter_has_contact)
			{
				// starter
                cmd_queue.AddCommand(PowertrainCommand::COMMAND_SET_STARTER, 1.f);
#ifdef USE_OPENAL
				SoundScriptManager::getSingleton().trigStart(curr_truck, SS_TRIG_STARTER);
#endif // OPENAL
			} 
			else
			{
				cmd_queue.AddCommand(PowertrainCommand::COMMAND_SET_STARTER, 0.f);
#ifdef USE_OPENAL
				SoundScriptManager::getSingleton().trigStop(curr_truck, SS_TRIG_STARTER);
#endif // OPENAL
			}

			if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SWITCH_SHIFT_MODES))
			{
				// toggle Auto shift
				cmd_queue.AddCommand(PowertrainCommand::COMMAND_TOGGLE_AUTO_MODE);

				// force gui update
				curr_truck->triggerGUIFeaturesChanged();
#ifdef USE_MYGUI
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
#endif //USE_MYGUI
			}

			// joy clutch
			float cval = RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_MANUAL_CLUTCH);
            cmd_queue.AddCommand(PowertrainCommand::COMMAND_SET_MANUAL_CLUTCH, cval);

			bool gear_changed_rel = false;
			int shiftmode = pw_state.transmission_auto_shift_mode;

			if (shiftmode <= Gearbox::SHIFTMODE_MANUAL) // auto, semi auto and sequential shifting
			{
				if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_UP))
				{
					cmd_queue.AddCommand(PowertrainCommand::COMMAND_SHIFT, 1.f); // shift up
					gear_changed_rel = true;
				} 
				else if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_DOWN))
				{
					if (shiftmode  > Gearbox::SHIFTMODE_SEMIAUTO ||
						shiftmode == Gearbox::SHIFTMODE_SEMIAUTO  && !arcadeControls ||
						shiftmode == Gearbox::SHIFTMODE_SEMIAUTO  && pw_state.transmission_current_gear > 0 ||
						shiftmode == Gearbox::SHIFTMODE_AUTOMATIC)
					{
						cmd_queue.AddCommand(PowertrainCommand::COMMAND_SHIFT, -1.f); // shift down
						gear_changed_rel = true;
					}
				} 
				else if (shiftmode != Gearbox::SHIFTMODE_AUTOMATIC && RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_NEUTRAL))
				{
					cmd_queue.AddCommand(PowertrainCommand::COMMAND_SHIFT_TO, 0.f);
				}
			} 
			else //if (shiftmode > BeamEngine::MANUAL) // h-shift or h-shift with ranges shifting
			{
				bool gear_changed = false;
				bool found        = false;
				int curgear       = pw_state.transmission_current_gear;
				int curgearrange    = pw_state.transmission_current_gear_range;
				int gearoffset      = std::max(0, curgear - curgearrange * 6);

				// one can select range only if in neutral
				if (shiftmode==Gearbox::SHIFTMODE_MANUAL_RANGES && curgear == 0)
				{
					//  maybe this should not be here, but should experiment
					if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_LOWRANGE) && curgearrange != 0)
					{
                        cmd_queue.AddCommand(PowertrainCommand::COMMAND_SET_GEAR_RANGE, 0.f);
						gear_changed = true;
#ifdef USE_MYGUI
						RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Low range selected"), "cog.png", 3000);
#endif //USE_MYGUI
					} 
					else if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_MIDRANGE)  && curgearrange != 1 && pw_state.conf_transmission_num_gears_ranges>1)
					{
						cmd_queue.AddCommand(PowertrainCommand::COMMAND_SET_GEAR_RANGE, 1.f);
						gear_changed = true;
#ifdef USE_MYGUI
						RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Mid range selected"), "cog.png", 3000);
#endif //USE_MYGUI
					} 
					else if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_SHIFT_HIGHRANGE) && curgearrange != 2 && pw_state.conf_transmission_num_gears_ranges>2)
					{
						cmd_queue.AddCommand(PowertrainCommand::COMMAND_SET_GEAR_RANGE, 2.f);
						gear_changed = true;
#ifdef USE_MYGUI
						RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("High range selected"), "cog.png", 3000);
#endif // USE_MYGUI
					}
				}
//zaxxon
				if (curgear == -1)
				{
					gear_changed = !RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_GEAR_REVERSE);
				} 
				else if (curgear > 0 && curgear < 19)
				{
					if (shiftmode==Gearbox::SHIFTMODE_MANUAL)
					{
						gear_changed = !RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_GEAR01 + curgear -1);
					} 
                    else
					{
						gear_changed = !RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_GEAR01 + gearoffset-1); // range mode
					}
				}

				if (gear_changed || curgear == 0)
				{
					if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_GEAR_REVERSE))
					{
						cmd_queue.AddCommand(PowertrainCommand::COMMAND_SHIFT_TO, -1.f);
						found = true;
					} 
					else if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_NEUTRAL))
					{
						cmd_queue.AddCommand(PowertrainCommand::COMMAND_SHIFT_TO, 0.f);
						found = true;
					} 
					else
					{
						if (shiftmode == Gearbox::SHIFTMODE_MANUAL_STICK)
						{
							for (int i=1; i < 19 && !found; i++)
							{
								if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_GEAR01 + i - 1))
								{
									cmd_queue.AddCommand(PowertrainCommand::COMMAND_SHIFT_TO, i);
									found = true;
								}
							}
						} 
						else // BeamEngine::MANUALMANUAL_RANGES
						{
							for (int i=1; i < 7 && !found; i++)
							{
								if (RoR::Application::GetInputEngine()->getEventBoolValue(EV_TRUCK_SHIFT_GEAR01 + i - 1))
								{
                                    int val = i + curgearrange * 6;
									cmd_queue.AddCommand(PowertrainCommand::COMMAND_SHIFT_TO, val);
									found = true;
								}
							}
						}
					}
					if (!found)
					{
						cmd_queue.AddCommand(PowertrainCommand::COMMAND_SHIFT_TO, 0.f);
					}
				} // end of if (gear_changed)
			} // end of shitmode > BeamEngine::MANUAL

			// anti roll back in BeamEngine::AUTOMATIC (DRIVE, TWO, ONE) mode
			if  (pw_state.transmission_mode  == Gearbox::SHIFTMODE_AUTOMATIC &&
			   (pw_state.transmission_auto_shift_mode == Gearbox::AUTOSWITCH_DRIVE ||
				pw_state.transmission_auto_shift_mode  == Gearbox::AUTOSWITCH_TWO ||
				pw_state.transmission_auto_shift_mode  == Gearbox::AUTOSWITCH_ONE) &&
				curr_truck->WheelSpeed < +0.1f)
			{
				Vector3 dirDiff = (curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition - curr_truck->nodes[curr_truck->cameranodedir[0]].RelPosition).normalisedCopy();
				Degree pitchAngle = Radian(asin(dirDiff.dotProduct(Vector3::UNIT_Y)));

				if (pitchAngle.valueDegrees() > +1.0f)
				{
					if (sin(pitchAngle.valueRadians()) * curr_truck->getTotalMass() > pw_state.engine_current_torque / 2.0f)
					{
						curr_truck->brake = curr_truck->brakeforce;
					} 
					else
					{
						curr_truck->brake = curr_truck->brakeforce * (1.0f - accl);
					}
				}
			}

			// anti roll forth in BeamEngine::AUTOMATIC (REAR) mode
			if (pw_state.transmission_mode  == Gearbox::SHIFTMODE_AUTOMATIC &&
				pw_state.transmission_auto_shift_mode  == Gearbox::AUTOSWITCH_REAR &&
				curr_truck->WheelSpeed > -0.1f)
			{
				Vector3 dirDiff = (curr_truck->nodes[curr_truck->cameranodepos[0]].RelPosition - curr_truck->nodes[curr_truck->cameranodedir[0]].RelPosition).normalisedCopy();
				Degree pitchAngle = Radian(asin(dirDiff.dotProduct(Vector3::UNIT_Y)));
				float accl = RoR::Application::GetInputEngine()->getEventValue(EV_TRUCK_ACCELERATE);

				if (pitchAngle.valueDegrees() < -1.0f)
				{
					if (sin(pitchAngle.valueRadians()) * curr_truck->getTotalMass() < pw_state.engine_current_torque / 2.0f)
					{
						curr_truck->brake = curr_truck->brakeforce;
					} 
					else
					{
						curr_truck->brake = curr_truck->brakeforce * (1.0f - accl);
					}
				}
			}
		} // end of ->engine
#ifdef USE_OPENAL
		if (curr_truck->brake > curr_truck->brakeforce / 6.0f)
		{
			SoundScriptManager::getSingleton().trigStart(curr_truck, SS_TRIG_BRAKE);
		} 
		else
		{
			SoundScriptManager::getSingleton().trigStop(curr_truck, SS_TRIG_BRAKE);
		}
#endif // USE_OPENAL
	} // end of ->replaymode

	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TOGGLE_AXLE_LOCK))
	{
		// toggle auto shift
		if (!curr_truck->getAxleLockCount())
		{
#ifdef USE_MYGUI
			RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("No differential installed on current vehicle!"), "warning.png", 3000);
			RoR::Application::GetGuiManager()->PushNotification("Differential:", "No differential installed on current vehicle!");
#endif // USE_MYGUI
		} 
		else
		{
			curr_truck->toggleAxleLock();
#ifdef USE_MYGUI
			RoR::Application::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("Differentials switched to: ") + curr_truck->getAxleLockName(), "cog.png", 3000);
			RoR::Application::GetGuiManager()->PushNotification("Differential:", "Differentials switched to: " + curr_truck->getAxleLockName());
#endif // USE_MYGUI
		}
	}

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

	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_PARKING_BRAKE))
	{
		curr_truck->parkingbrakeToggle();
	}

	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_ANTILOCK_BRAKE))
	{
		if (curr_truck->alb_present && !curr_truck->alb_notoggle)
		{
			curr_truck->antilockbrakeToggle();
		}
	}

	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_TRACTION_CONTROL))
	{
		if (curr_truck->tc_present && !curr_truck->tc_notoggle) curr_truck->tractioncontrolToggle();
	}

	if (RoR::Application::GetInputEngine()->getEventBoolValueBounce(EV_TRUCK_CRUISE_CONTROL))
	{
		curr_truck->cruisecontrolToggle();
	}
	if (curr_truck->cc_mode)
	{
		LandVehicleSimulation::UpdateCruiseControl(curr_truck, seconds_since_last_frame);
	}
	LandVehicleSimulation::CheckSpeedLimit(curr_truck, seconds_since_last_frame);
}

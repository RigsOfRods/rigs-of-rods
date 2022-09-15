/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2022 Petr Ohlidal

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
/// @author Petr Ohlidal
/// @date   01-2022
 
#include "InputEngine.h"
#include "ScriptEngine.h"

using namespace AngelScript;

void registerInputEngineObject(asIScriptEngine* engine)
{
    using namespace RoR;
    int result = 0;

    result = engine->RegisterObjectType("InputEngineClass", sizeof(InputEngine), asOBJ_REF | asOBJ_NOCOUNT); ROR_ASSERT(result>=0);
    
    result = engine->RegisterObjectMethod("InputEngineClass", "string getEventCommand(inputEvents ev)", asMETHOD(InputEngine,getEventCommand), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("InputEngineClass", "string getEventCommandTrimmed(inputEvents ev)", asMETHOD(InputEngine,getEventCommandTrimmed), asCALL_THISCALL); ROR_ASSERT(result>=0);

// TODO: register OIS::KeyCode or port to SDL2 already (for Apple OSX support)
//    result = engine->RegisterObjectMethod("InputEngineClass", "bool isKeyDown(int keycode)", asMETHOD(InputEngine,isKeyDown), asCALL_THISCALL); ROR_ASSERT(result>=0);
//    result = engine->RegisterObjectMethod("InputEngineClass", "bool isKeyDownEffective(int keycode)", asMETHOD(InputEngine,isKeyDownEffective), asCALL_THISCALL); ROR_ASSERT(result>=0);

    result = engine->RegisterObjectMethod("InputEngineClass", "bool getEventBoolValue(inputEvents ev)", asMETHOD(InputEngine,getEventBoolValue), asCALL_THISCALL); ROR_ASSERT(result>=0);
    result = engine->RegisterObjectMethod("InputEngineClass", "bool getEventBoolValueBounce(inputEvents ev, float time = 0.2f)", asMETHOD(InputEngine,getEventBoolValueBounce), asCALL_THISCALL); ROR_ASSERT(result>=0);    
}

void registerEventTypeEnum(asIScriptEngine* engine)
{
    using namespace RoR;
    int result = 0;

    result = engine->RegisterEnum("inputEvents"); ROR_ASSERT(result >= 0);

    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_AIRBRAKES_FULL",        EV_AIRPLANE_AIRBRAKES_FULL      ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_AIRBRAKES_LESS",        EV_AIRPLANE_AIRBRAKES_LESS      ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_AIRBRAKES_MORE",        EV_AIRPLANE_AIRBRAKES_MORE      ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_AIRBRAKES_NONE",        EV_AIRPLANE_AIRBRAKES_NONE      ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_BRAKE",                 EV_AIRPLANE_BRAKE               ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_ELEVATOR_DOWN",         EV_AIRPLANE_ELEVATOR_DOWN       ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_ELEVATOR_UP",           EV_AIRPLANE_ELEVATOR_UP         ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_FLAPS_FULL",            EV_AIRPLANE_FLAPS_FULL          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_FLAPS_LESS",            EV_AIRPLANE_FLAPS_LESS          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_FLAPS_MORE",            EV_AIRPLANE_FLAPS_MORE          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_FLAPS_NONE",            EV_AIRPLANE_FLAPS_NONE          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_PARKING_BRAKE",         EV_AIRPLANE_PARKING_BRAKE       ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_REVERSE",               EV_AIRPLANE_REVERSE             ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_RUDDER_LEFT",           EV_AIRPLANE_RUDDER_LEFT         ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_RUDDER_RIGHT",          EV_AIRPLANE_RUDDER_RIGHT        ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_STEER_LEFT",            EV_AIRPLANE_STEER_LEFT          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_STEER_RIGHT",           EV_AIRPLANE_STEER_RIGHT         ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_THROTTLE",              EV_AIRPLANE_THROTTLE            ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_THROTTLE_AXIS",         EV_AIRPLANE_THROTTLE_AXIS       ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_THROTTLE_DOWN",         EV_AIRPLANE_THROTTLE_DOWN       ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_THROTTLE_FULL",         EV_AIRPLANE_THROTTLE_FULL       ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_THROTTLE_NO",           EV_AIRPLANE_THROTTLE_NO         ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_THROTTLE_UP",           EV_AIRPLANE_THROTTLE_UP         ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_AIRPLANE_TOGGLE_ENGINES",        EV_AIRPLANE_TOGGLE_ENGINES      ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_BOAT_CENTER_RUDDER",             EV_BOAT_CENTER_RUDDER           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_BOAT_REVERSE",                   EV_BOAT_REVERSE                 ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_BOAT_STEER_LEFT",                EV_BOAT_STEER_LEFT              ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_BOAT_STEER_LEFT_AXIS",           EV_BOAT_STEER_LEFT_AXIS         ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_BOAT_STEER_RIGHT",               EV_BOAT_STEER_RIGHT             ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_BOAT_STEER_RIGHT_AXIS",          EV_BOAT_STEER_RIGHT_AXIS        ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_BOAT_THROTTLE_AXIS",             EV_BOAT_THROTTLE_AXIS           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_BOAT_THROTTLE_DOWN",             EV_BOAT_THROTTLE_DOWN           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_BOAT_THROTTLE_UP",               EV_BOAT_THROTTLE_UP             ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_SKY_DECREASE_TIME",              EV_SKY_DECREASE_TIME            ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_SKY_DECREASE_TIME_FAST",         EV_SKY_DECREASE_TIME_FAST       ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_SKY_INCREASE_TIME",              EV_SKY_INCREASE_TIME            ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_SKY_INCREASE_TIME_FAST",         EV_SKY_INCREASE_TIME_FAST       ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CAMERA_CHANGE",                  EV_CAMERA_CHANGE                ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CAMERA_DOWN",                    EV_CAMERA_DOWN                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CAMERA_FREE_MODE",               EV_CAMERA_FREE_MODE             ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CAMERA_FREE_MODE_FIX",           EV_CAMERA_FREE_MODE_FIX         ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CAMERA_LOOKBACK",                EV_CAMERA_LOOKBACK              ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CAMERA_RESET",                   EV_CAMERA_RESET                 ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CAMERA_ROTATE_DOWN",             EV_CAMERA_ROTATE_DOWN           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CAMERA_ROTATE_LEFT",             EV_CAMERA_ROTATE_LEFT           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CAMERA_ROTATE_RIGHT",            EV_CAMERA_ROTATE_RIGHT          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CAMERA_ROTATE_UP",               EV_CAMERA_ROTATE_UP             ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CAMERA_UP",                      EV_CAMERA_UP                    ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CAMERA_ZOOM_IN",                 EV_CAMERA_ZOOM_IN               ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CAMERA_ZOOM_IN_FAST",            EV_CAMERA_ZOOM_IN_FAST          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CAMERA_ZOOM_OUT",                EV_CAMERA_ZOOM_OUT              ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CAMERA_ZOOM_OUT_FAST",           EV_CAMERA_ZOOM_OUT_FAST         ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CHARACTER_BACKWARDS",            EV_CHARACTER_BACKWARDS          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CHARACTER_FORWARD",              EV_CHARACTER_FORWARD            ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CHARACTER_JUMP",                 EV_CHARACTER_JUMP               ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CHARACTER_LEFT",                 EV_CHARACTER_LEFT               ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CHARACTER_RIGHT",                EV_CHARACTER_RIGHT              ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CHARACTER_ROT_DOWN",             EV_CHARACTER_ROT_DOWN           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CHARACTER_ROT_UP",               EV_CHARACTER_ROT_UP             ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CHARACTER_RUN",                  EV_CHARACTER_RUN                ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CHARACTER_SIDESTEP_LEFT",        EV_CHARACTER_SIDESTEP_LEFT      ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_CHARACTER_SIDESTEP_RIGHT",       EV_CHARACTER_SIDESTEP_RIGHT     ); ROR_ASSERT(result >= 0);

    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_01",                    EV_COMMANDS_01                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_02",                    EV_COMMANDS_02                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_03",                    EV_COMMANDS_03                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_04",                    EV_COMMANDS_04                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_05",                    EV_COMMANDS_05                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_06",                    EV_COMMANDS_06                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_07",                    EV_COMMANDS_07                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_08",                    EV_COMMANDS_08                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_09",                    EV_COMMANDS_09                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_10",                    EV_COMMANDS_10                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_11",                    EV_COMMANDS_11                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_12",                    EV_COMMANDS_12                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_13",                    EV_COMMANDS_13                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_14",                    EV_COMMANDS_14                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_15",                    EV_COMMANDS_15                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_16",                    EV_COMMANDS_16                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_17",                    EV_COMMANDS_17                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_18",                    EV_COMMANDS_18                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_19",                    EV_COMMANDS_19                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_20",                    EV_COMMANDS_20                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_21",                    EV_COMMANDS_21                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_22",                    EV_COMMANDS_22                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_23",                    EV_COMMANDS_23                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_24",                    EV_COMMANDS_24                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_25",                    EV_COMMANDS_25                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_26",                    EV_COMMANDS_26                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_27",                    EV_COMMANDS_27                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_28",                    EV_COMMANDS_28                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_29",                    EV_COMMANDS_29                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_30",                    EV_COMMANDS_30                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_31",                    EV_COMMANDS_31                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_32",                    EV_COMMANDS_32                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_33",                    EV_COMMANDS_33                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_34",                    EV_COMMANDS_34                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_35",                    EV_COMMANDS_35                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_36",                    EV_COMMANDS_36                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_37",                    EV_COMMANDS_37                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_38",                    EV_COMMANDS_38                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_39",                    EV_COMMANDS_39                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_40",                    EV_COMMANDS_40                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_41",                    EV_COMMANDS_41                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_42",                    EV_COMMANDS_42                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_43",                    EV_COMMANDS_43                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_44",                    EV_COMMANDS_44                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_45",                    EV_COMMANDS_45                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_46",                    EV_COMMANDS_46                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_47",                    EV_COMMANDS_47                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_48",                    EV_COMMANDS_48                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_49",                    EV_COMMANDS_49                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_50",                    EV_COMMANDS_50                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_51",                    EV_COMMANDS_51                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_52",                    EV_COMMANDS_52                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_53",                    EV_COMMANDS_53                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_54",                    EV_COMMANDS_54                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_55",                    EV_COMMANDS_55                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_56",                    EV_COMMANDS_56                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_57",                    EV_COMMANDS_57                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_58",                    EV_COMMANDS_58                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_59",                    EV_COMMANDS_59                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_60",                    EV_COMMANDS_60                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_61",                    EV_COMMANDS_61                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_62",                    EV_COMMANDS_62                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_63",                    EV_COMMANDS_63                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_64",                    EV_COMMANDS_64                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_65",                    EV_COMMANDS_65                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_66",                    EV_COMMANDS_66                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_67",                    EV_COMMANDS_67                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_68",                    EV_COMMANDS_68                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_69",                    EV_COMMANDS_69                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_70",                    EV_COMMANDS_70                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_71",                    EV_COMMANDS_71                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_72",                    EV_COMMANDS_72                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_73",                    EV_COMMANDS_73                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_74",                    EV_COMMANDS_74                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_75",                    EV_COMMANDS_75                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_76",                    EV_COMMANDS_76                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_77",                    EV_COMMANDS_77                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_78",                    EV_COMMANDS_78                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_79",                    EV_COMMANDS_79                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_80",                    EV_COMMANDS_80                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_81",                    EV_COMMANDS_81                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_82",                    EV_COMMANDS_82                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_83",                    EV_COMMANDS_83                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMANDS_84",                    EV_COMMANDS_84                  ); ROR_ASSERT(result >= 0);

    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_ACCELERATE_SIMULATION",   EV_COMMON_ACCELERATE_SIMULATION ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_DECELERATE_SIMULATION",   EV_COMMON_DECELERATE_SIMULATION ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_RESET_SIMULATION_PACE",   EV_COMMON_RESET_SIMULATION_PACE ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_AUTOLOCK",                EV_COMMON_AUTOLOCK              ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_CONSOLE_TOGGLE",          EV_COMMON_CONSOLE_TOGGLE        ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_ENTER_CHATMODE",          EV_COMMON_ENTER_CHATMODE        ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_ENTER_OR_EXIT_TRUCK",     EV_COMMON_ENTER_OR_EXIT_TRUCK   ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_ENTER_NEXT_TRUCK",        EV_COMMON_ENTER_NEXT_TRUCK      ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_ENTER_PREVIOUS_TRUCK",    EV_COMMON_ENTER_PREVIOUS_TRUCK  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_REMOVE_CURRENT_TRUCK",    EV_COMMON_REMOVE_CURRENT_TRUCK  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_RESPAWN_LAST_TRUCK",      EV_COMMON_RESPAWN_LAST_TRUCK    ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_FOV_LESS",                EV_COMMON_FOV_LESS              ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_FOV_MORE",                EV_COMMON_FOV_MORE              ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_FOV_RESET",               EV_COMMON_FOV_RESET             ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_FULLSCREEN_TOGGLE",       EV_COMMON_FULLSCREEN_TOGGLE     ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_HIDE_GUI",                EV_COMMON_HIDE_GUI              ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_TOGGLE_DASHBOARD",        EV_COMMON_TOGGLE_DASHBOARD      ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_LOCK",                    EV_COMMON_LOCK                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_NETCHATDISPLAY",          EV_COMMON_NETCHATDISPLAY        ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_NETCHATMODE",             EV_COMMON_NETCHATMODE           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_OUTPUT_POSITION",         EV_COMMON_OUTPUT_POSITION       ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_GET_NEW_VEHICLE",         EV_COMMON_GET_NEW_VEHICLE       ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_PRESSURE_LESS",           EV_COMMON_PRESSURE_LESS         ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_PRESSURE_MORE",           EV_COMMON_PRESSURE_MORE         ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKLOAD",               EV_COMMON_QUICKLOAD             ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKSAVE",               EV_COMMON_QUICKSAVE             ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUIT_GAME",               EV_COMMON_QUIT_GAME             ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_REPAIR_TRUCK",            EV_COMMON_REPAIR_TRUCK          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_REPLAY_BACKWARD",         EV_COMMON_REPLAY_BACKWARD       ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_REPLAY_FAST_BACKWARD",    EV_COMMON_REPLAY_FAST_BACKWARD  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_REPLAY_FAST_FORWARD",     EV_COMMON_REPLAY_FAST_FORWARD   ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_REPLAY_FORWARD",          EV_COMMON_REPLAY_FORWARD        ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_RESCUE_TRUCK",            EV_COMMON_RESCUE_TRUCK          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_RESET_TRUCK",             EV_COMMON_RESET_TRUCK           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_TOGGLE_RESET_MODE",       EV_COMMON_TOGGLE_RESET_MODE     ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_ROPELOCK",                EV_COMMON_ROPELOCK              ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_SAVE_TERRAIN",            EV_COMMON_SAVE_TERRAIN          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_SCREENSHOT",              EV_COMMON_SCREENSHOT            ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_SCREENSHOT_BIG",          EV_COMMON_SCREENSHOT_BIG        ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_SECURE_LOAD",             EV_COMMON_SECURE_LOAD           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_SEND_CHAT",               EV_COMMON_SEND_CHAT             ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_TOGGLE_DEBUG_VIEW",       EV_COMMON_TOGGLE_DEBUG_VIEW     ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_CYCLE_DEBUG_VIEWS",       EV_COMMON_CYCLE_DEBUG_VIEWS     ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_TOGGLE_TERRAIN_EDITOR",   EV_COMMON_TOGGLE_TERRAIN_EDITOR ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_TOGGLE_CUSTOM_PARTICLES", EV_COMMON_TOGGLE_CUSTOM_PARTICLES); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_TOGGLE_MAT_DEBUG",        EV_COMMON_TOGGLE_MAT_DEBUG      ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_TOGGLE_REPLAY_MODE",      EV_COMMON_TOGGLE_REPLAY_MODE    ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_TOGGLE_PHYSICS",          EV_COMMON_TOGGLE_PHYSICS        ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_TOGGLE_STATS",            EV_COMMON_TOGGLE_STATS          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_TOGGLE_TRUCK_BEACONS",    EV_COMMON_TOGGLE_TRUCK_BEACONS  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_TOGGLE_TRUCK_LOW_BEAMS",  EV_COMMON_TOGGLE_TRUCK_LOW_BEAMS); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_TOGGLE_TRUCK_HIGH_BEAMS", EV_COMMON_TOGGLE_TRUCK_HIGH_BEAMS); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_TOGGLE_TRUCK_FOG_LIGHTS", EV_COMMON_TOGGLE_TRUCK_FOG_LIGHTS); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_TRUCK_INFO",              EV_COMMON_TRUCK_INFO            ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_TRUCK_DESCRIPTION",       EV_COMMON_TRUCK_DESCRIPTION     ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_TRUCK_REMOVE",            EV_COMMON_TRUCK_REMOVE          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_GRASS_LESS",                     EV_GRASS_LESS                   ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_GRASS_MORE",                     EV_GRASS_MORE                   ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_GRASS_MOST",                     EV_GRASS_MOST                   ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_GRASS_NONE",                     EV_GRASS_NONE                   ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_GRASS_SAVE",                     EV_GRASS_SAVE                   ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_MENU_DOWN",                      EV_MENU_DOWN                    ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_MENU_LEFT",                      EV_MENU_LEFT                    ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_MENU_RIGHT",                     EV_MENU_RIGHT                   ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_MENU_SELECT",                    EV_MENU_SELECT                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_MENU_UP",                        EV_MENU_UP                      ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_SURVEY_MAP_TOGGLE_ICONS",        EV_SURVEY_MAP_TOGGLE_ICONS      ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_SURVEY_MAP_CYCLE",               EV_SURVEY_MAP_CYCLE             ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_SURVEY_MAP_TOGGLE",              EV_SURVEY_MAP_TOGGLE            ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_SURVEY_MAP_ZOOM_IN",             EV_SURVEY_MAP_ZOOM_IN           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_SURVEY_MAP_ZOOM_OUT",            EV_SURVEY_MAP_ZOOM_OUT          ); ROR_ASSERT(result >= 0);
                                                                                                                           
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_ACCELERATE",               EV_TRUCK_ACCELERATE             ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_ACCELERATE_MODIFIER_25",   EV_TRUCK_ACCELERATE_MODIFIER_25 ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_ACCELERATE_MODIFIER_50",   EV_TRUCK_ACCELERATE_MODIFIER_50 ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_ANTILOCK_BRAKE",           EV_TRUCK_ANTILOCK_BRAKE         ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_AUTOSHIFT_DOWN",           EV_TRUCK_AUTOSHIFT_DOWN         ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_AUTOSHIFT_UP",             EV_TRUCK_AUTOSHIFT_UP           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_BLINK_LEFT",               EV_TRUCK_BLINK_LEFT             ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_BLINK_RIGHT",              EV_TRUCK_BLINK_RIGHT            ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_BLINK_WARN",               EV_TRUCK_BLINK_WARN             ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_BRAKE",                    EV_TRUCK_BRAKE                  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_BRAKE_MODIFIER_25",        EV_TRUCK_BRAKE_MODIFIER_25      ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_BRAKE_MODIFIER_50",        EV_TRUCK_BRAKE_MODIFIER_50      ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_CRUISE_CONTROL",           EV_TRUCK_CRUISE_CONTROL         ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_CRUISE_CONTROL_ACCL",      EV_TRUCK_CRUISE_CONTROL_ACCL    ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_CRUISE_CONTROL_DECL",      EV_TRUCK_CRUISE_CONTROL_DECL    ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_CRUISE_CONTROL_READJUST",  EV_TRUCK_CRUISE_CONTROL_READJUST); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_HORN",                     EV_TRUCK_HORN                   ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_LEFT_MIRROR_LEFT",         EV_TRUCK_LEFT_MIRROR_LEFT       ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_LEFT_MIRROR_RIGHT",        EV_TRUCK_LEFT_MIRROR_RIGHT      ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_LIGHTTOGGLE01",            EV_TRUCK_LIGHTTOGGLE01          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_LIGHTTOGGLE02",            EV_TRUCK_LIGHTTOGGLE02          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_LIGHTTOGGLE03",            EV_TRUCK_LIGHTTOGGLE03          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_LIGHTTOGGLE04",            EV_TRUCK_LIGHTTOGGLE04          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_LIGHTTOGGLE05",            EV_TRUCK_LIGHTTOGGLE05          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_LIGHTTOGGLE06",            EV_TRUCK_LIGHTTOGGLE06          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_LIGHTTOGGLE07",            EV_TRUCK_LIGHTTOGGLE07          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_LIGHTTOGGLE08",            EV_TRUCK_LIGHTTOGGLE08          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_LIGHTTOGGLE09",            EV_TRUCK_LIGHTTOGGLE09          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_LIGHTTOGGLE10",            EV_TRUCK_LIGHTTOGGLE10          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_MANUAL_CLUTCH",            EV_TRUCK_MANUAL_CLUTCH          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_PARKING_BRAKE",            EV_TRUCK_PARKING_BRAKE          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_TRAILER_PARKING_BRAKE",    EV_TRUCK_TRAILER_PARKING_BRAKE  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_RIGHT_MIRROR_LEFT",        EV_TRUCK_RIGHT_MIRROR_LEFT      ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_RIGHT_MIRROR_RIGHT",       EV_TRUCK_RIGHT_MIRROR_RIGHT     ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_DOWN",               EV_TRUCK_SHIFT_DOWN             ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_GEAR01",             EV_TRUCK_SHIFT_GEAR01           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_GEAR02",             EV_TRUCK_SHIFT_GEAR02           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_GEAR03",             EV_TRUCK_SHIFT_GEAR03           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_GEAR04",             EV_TRUCK_SHIFT_GEAR04           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_GEAR05",             EV_TRUCK_SHIFT_GEAR05           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_GEAR06",             EV_TRUCK_SHIFT_GEAR06           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_GEAR07",             EV_TRUCK_SHIFT_GEAR07           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_GEAR08",             EV_TRUCK_SHIFT_GEAR08           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_GEAR09",             EV_TRUCK_SHIFT_GEAR09           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_GEAR10",             EV_TRUCK_SHIFT_GEAR10           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_GEAR11",             EV_TRUCK_SHIFT_GEAR11           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_GEAR12",             EV_TRUCK_SHIFT_GEAR12           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_GEAR13",             EV_TRUCK_SHIFT_GEAR13           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_GEAR14",             EV_TRUCK_SHIFT_GEAR14           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_GEAR15",             EV_TRUCK_SHIFT_GEAR15           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_GEAR16",             EV_TRUCK_SHIFT_GEAR16           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_GEAR17",             EV_TRUCK_SHIFT_GEAR17           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_GEAR18",             EV_TRUCK_SHIFT_GEAR18           ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_GEAR_REVERSE",       EV_TRUCK_SHIFT_GEAR_REVERSE     ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_HIGHRANGE",          EV_TRUCK_SHIFT_HIGHRANGE        ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_LOWRANGE",           EV_TRUCK_SHIFT_LOWRANGE         ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_MIDRANGE",           EV_TRUCK_SHIFT_MIDRANGE         ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_NEUTRAL",            EV_TRUCK_SHIFT_NEUTRAL          ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SHIFT_UP",                 EV_TRUCK_SHIFT_UP               ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_STARTER",                  EV_TRUCK_STARTER                ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_STEER_LEFT",               EV_TRUCK_STEER_LEFT             ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_STEER_RIGHT",              EV_TRUCK_STEER_RIGHT            ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_SWITCH_SHIFT_MODES",       EV_TRUCK_SWITCH_SHIFT_MODES     ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_TOGGLE_CONTACT",           EV_TRUCK_TOGGLE_CONTACT         ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_TOGGLE_FORWARDCOMMANDS",   EV_TRUCK_TOGGLE_FORWARDCOMMANDS ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_TOGGLE_IMPORTCOMMANDS",    EV_TRUCK_TOGGLE_IMPORTCOMMANDS  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_TOGGLE_INTER_AXLE_DIFF",   EV_TRUCK_TOGGLE_INTER_AXLE_DIFF ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_TOGGLE_INTER_WHEEL_DIFF",  EV_TRUCK_TOGGLE_INTER_WHEEL_DIFF); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_TOGGLE_PHYSICS",           EV_TRUCK_TOGGLE_PHYSICS         ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_TOGGLE_TCASE_4WD_MODE",    EV_TRUCK_TOGGLE_TCASE_4WD_MODE  ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_TOGGLE_TCASE_GEAR_RATIO",  EV_TRUCK_TOGGLE_TCASE_GEAR_RATIO); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_TOGGLE_VIDEOCAMERA",       EV_TRUCK_TOGGLE_VIDEOCAMERA     ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCK_TRACTION_CONTROL",         EV_TRUCK_TRACTION_CONTROL       ); ROR_ASSERT(result >= 0);

    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKSAVE_01",            EV_COMMON_QUICKSAVE_01          ); ROR_ASSERT(result >= 0);               
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKSAVE_02",            EV_COMMON_QUICKSAVE_02          ); ROR_ASSERT(result >= 0);               
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKSAVE_03",            EV_COMMON_QUICKSAVE_03          ); ROR_ASSERT(result >= 0);               
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKSAVE_04",            EV_COMMON_QUICKSAVE_04          ); ROR_ASSERT(result >= 0);               
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKSAVE_05",            EV_COMMON_QUICKSAVE_05          ); ROR_ASSERT(result >= 0);               
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKSAVE_06",            EV_COMMON_QUICKSAVE_06          ); ROR_ASSERT(result >= 0);               
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKSAVE_07",            EV_COMMON_QUICKSAVE_07          ); ROR_ASSERT(result >= 0);               
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKSAVE_08",            EV_COMMON_QUICKSAVE_08          ); ROR_ASSERT(result >= 0);               
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKSAVE_09",            EV_COMMON_QUICKSAVE_09          ); ROR_ASSERT(result >= 0);               
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKSAVE_10",            EV_COMMON_QUICKSAVE_10          ); ROR_ASSERT(result >= 0);
                                                                                                                           
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKLOAD_01",            EV_COMMON_QUICKLOAD_01          ); ROR_ASSERT(result >= 0);               
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKLOAD_02",            EV_COMMON_QUICKLOAD_02          ); ROR_ASSERT(result >= 0);               
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKLOAD_03",            EV_COMMON_QUICKLOAD_03          ); ROR_ASSERT(result >= 0);               
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKLOAD_04",            EV_COMMON_QUICKLOAD_04          ); ROR_ASSERT(result >= 0);               
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKLOAD_05",            EV_COMMON_QUICKLOAD_05          ); ROR_ASSERT(result >= 0);               
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKLOAD_06",            EV_COMMON_QUICKLOAD_06          ); ROR_ASSERT(result >= 0);               
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKLOAD_07",            EV_COMMON_QUICKLOAD_07          ); ROR_ASSERT(result >= 0);               
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKLOAD_08",            EV_COMMON_QUICKLOAD_08          ); ROR_ASSERT(result >= 0);               
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKLOAD_09",            EV_COMMON_QUICKLOAD_09          ); ROR_ASSERT(result >= 0);               
    result = engine->RegisterEnumValue("inputEvents", "EV_COMMON_QUICKLOAD_10",            EV_COMMON_QUICKLOAD_10          ); ROR_ASSERT(result >= 0);               

    result = engine->RegisterEnumValue("inputEvents", "EV_TRUCKEDIT_RELOAD",               EV_TRUCKEDIT_RELOAD             ); ROR_ASSERT(result >= 0);               

    result = engine->RegisterEnumValue("inputEvents", "EV_ROAD_EDITOR_POINT_INSERT",       EV_ROAD_EDITOR_POINT_INSERT     ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_ROAD_EDITOR_POINT_GOTO",         EV_ROAD_EDITOR_POINT_GOTO       ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_ROAD_EDITOR_POINT_SET_POS",      EV_ROAD_EDITOR_POINT_SET_POS    ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_ROAD_EDITOR_POINT_DELETE",       EV_ROAD_EDITOR_POINT_DELETE     ); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("inputEvents", "EV_ROAD_EDITOR_REBUILD_MESH",       EV_ROAD_EDITOR_REBUILD_MESH     ); ROR_ASSERT(result >= 0);
}

void RoR::RegisterInputEngine(asIScriptEngine* engine)
{
    registerEventTypeEnum(engine);
    registerInputEngineObject(engine);
}
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

#include "InputEngine.h"

#include "Actor.h"
#include "AppContext.h"
#include "Console.h"
#include "ContentManager.h"
#include "GUIManager.h"
#include "Language.h"

#include <regex>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_mouse.h>

using namespace RoR;

InputEvent eventInfo[] = {
    // Common: generic
    {"COMMON_ACCELERATE_SIMULATION",  EV_COMMON_ACCELERATE_SIMULATION,  "Keyboard CTRL+EQUALS",         _LC("InputEvent", "accelerate the simulation")},
    {"COMMON_DECELERATE_SIMULATION",  EV_COMMON_DECELERATE_SIMULATION,  "Keyboard SHIFT+EQUALS",        _LC("InputEvent", "decelerate the simulation")},
    {"COMMON_RESET_SIMULATION_PACE",  EV_COMMON_RESET_SIMULATION_PACE,  "Keyboard BACKSLASH",           _LC("InputEvent", "reset the simulation pace")},
    {"COMMON_OUTPUT_POSITION",        EV_COMMON_OUTPUT_POSITION,        "Keyboard H",                   _LC("InputEvent", "write current position to log (you can open the logfile and reuse the position)")},
    {"COMMON_QUIT_GAME",              EV_COMMON_QUIT_GAME,              "Keyboard EXPL+ESCAPE",         _LC("InputEvent", "exit the game")},
    {"COMMON_QUICKLOAD",              EV_COMMON_QUICKLOAD,              "Keyboard MULTIPLY",            _LC("InputEvent", "quickload scene")},
    {"COMMON_QUICKSAVE",              EV_COMMON_QUICKSAVE,              "Keyboard DIVIDE",              _LC("InputEvent", "quicksave scene")},
    {"COMMON_SCREENSHOT",             EV_COMMON_SCREENSHOT,             "Keyboard EXPL+SYSRQ",          _LC("InputEvent", "take a screenshot")},
    {"COMMON_SCREENSHOT_BIG",         EV_COMMON_SCREENSHOT_BIG,         "Keyboard EXPL+CTRL+SYSRQ",     _LC("InputEvent", "take a big screenshot (3 times the screen size)")},
    {"COMMON_TOGGLE_MAT_DEBUG",       EV_COMMON_TOGGLE_MAT_DEBUG,       "",                             _LC("InputEvent", "debug purpose - dont use")},
    {"COMMON_TOGGLE_PHYSICS",         EV_COMMON_TOGGLE_PHYSICS,         "Keyboard EXPL+J",              _LC("InputEvent", "enable or disable physics")},
    {"COMMON_FOV_LESS",               EV_COMMON_FOV_LESS,               "Keyboard EXPL+NUMPAD7",        _LC("InputEvent", "decreases the current FOV value")},
    {"COMMON_FOV_MORE",               EV_COMMON_FOV_MORE,               "Keyboard EXPL+CTRL+NUMPAD7",   _LC("InputEvent", "increase the current FOV value")},
    {"COMMON_FOV_RESET",              EV_COMMON_FOV_RESET,              "Keyboard EXPL+SHIFT+NUMPAD7",  _LC("InputEvent", "reset the FOV value")},
    {"COMMON_SAVE_TERRAIN",           EV_COMMON_SAVE_TERRAIN,           "Keyboard EXPL+ALT+SHIF+CTRL+M",_LC("InputEvent", "save the currently loaded terrain to a mesh file")},
    {"COMMON_TOGGLE_TERRAIN_EDITOR",  EV_COMMON_TOGGLE_TERRAIN_EDITOR,  "Keyboard EXPL+SHIFT+Y",        _LC("InputEvent", "toggle terrain editor")},
    {"COMMON_FULLSCREEN_TOGGLE",      EV_COMMON_FULLSCREEN_TOGGLE,      "Keyboard EXPL+ALT+RETURN",     _LC("InputEvent", "toggle between windowed and fullscreen mode")},

    // Comon: actor interaction
    {"COMMON_ENTER_OR_EXIT_TRUCK",    EV_COMMON_ENTER_OR_EXIT_TRUCK,    "Keyboard RETURN",              _LC("InputEvent", "enter or exit a truck")},
    {"COMMON_ENTER_NEXT_TRUCK",       EV_COMMON_ENTER_NEXT_TRUCK,       "Keyboard EXPL+CTRL+RBRACKET",  _LC("InputEvent", "enter next truck")},
    {"COMMON_ENTER_PREVIOUS_TRUCK",   EV_COMMON_ENTER_PREVIOUS_TRUCK,   "Keyboard EXPL+CTRL+LBRACKET",  _LC("InputEvent", "enter previous truck")},
    {"COMMON_REMOVE_CURRENT_TRUCK",   EV_COMMON_REMOVE_CURRENT_TRUCK,   "Keyboard EXPL+CTRL+DELETE",    _LC("InputEvent", "remove current truck")},
    {"COMMON_TRUCK_REMOVE",           EV_COMMON_TRUCK_REMOVE,           "Keyboard EXPL+CTRL+SHIFT+DELETE",_LC("InputEvent", "delete current truck")},
    {"COMMON_RESPAWN_LAST_TRUCK",     EV_COMMON_RESPAWN_LAST_TRUCK,     "Keyboard EXPL+CTRL+PERIOD",    _LC("InputEvent", "respawn last truck")},
    {"COMMON_GET_NEW_VEHICLE",        EV_COMMON_GET_NEW_VEHICLE,        "Keyboard EXPL+CTRL+G",         _LC("InputEvent", "get new vehicle")},
    {"COMMON_PRESSURE_LESS",          EV_COMMON_PRESSURE_LESS,          "Keyboard LBRACKET",            _LC("InputEvent", "decrease tire pressure (note: only very few trucks support this)")},
    {"COMMON_PRESSURE_MORE",          EV_COMMON_PRESSURE_MORE,          "Keyboard RBRACKET",            _LC("InputEvent", "increase tire pressure (note: only very few trucks support this)")},
    {"COMMON_LOCK",                   EV_COMMON_LOCK,                   "Keyboard EXPL+L",              _LC("InputEvent", "connect hook node to a node in close proximity")},
    {"COMMON_AUTOLOCK",               EV_COMMON_AUTOLOCK,               "Keyboard EXPL+ALT+L",          _LC("InputEvent", "unlock autolock hook node")},
    {"COMMON_ROPELOCK",               EV_COMMON_ROPELOCK,               "Keyboard EXPL+CTRL+L",         _LC("InputEvent", "connect a rope to a node in close proximity")},
    {"COMMON_REPAIR_TRUCK",           EV_COMMON_REPAIR_TRUCK,           "Keyboard BACK",                _LC("InputEvent", "repair truck")},
    {"COMMON_LIVE_REPAIR_MODE",       EV_COMMON_LIVE_REPAIR_MODE,       "Keyboard ALT+BACK",            _LC("InputEvent", "toggle truck interactive repair mode")},
    {"COMMON_RESCUE_TRUCK",           EV_COMMON_RESCUE_TRUCK,           "Keyboard EXPL+R",              _LC("InputEvent", "teleport to rescue truck")},
    {"COMMON_RESET_TRUCK",            EV_COMMON_RESET_TRUCK,            "Keyboard I",                   _LC("InputEvent", "reset truck to original starting position")},
    {"COMMON_TOGGLE_RESET_MODE",      EV_COMMON_TOGGLE_RESET_MODE,      "Keyboard EXPL+APOSTROPHE",     _LC("InputEvent", "toggle reset mode")},
    {"COMMON_SECURE_LOAD",            EV_COMMON_SECURE_LOAD,            "Keyboard O",                   _LC("InputEvent", "tie a load to the truck")},
    {"COMMON_TOGGLE_TRUCK_BEACONS",   EV_COMMON_TOGGLE_TRUCK_BEACONS,   "Keyboard M",                   _LC("InputEvent", "toggle truck beacons")},
    {"COMMON_TOGGLE_TRUCK_LOW_BEAMS", EV_COMMON_TOGGLE_TRUCK_LOW_BEAMS, "Keyboard EXPL+N",              _LC("InputEvent", "toggle truck low beams")},
    {"COMMON_CYCLE_TRUCK_LIGHTS",     EV_COMMON_CYCLE_TRUCK_LIGHTS,     "Keyboard EXPL+CTRL+N",         _LC("InputEvent", "cycle between light modes")},
    {"COMMON_TOGGLE_TRUCK_HIGH_BEAMS",EV_COMMON_TOGGLE_TRUCK_HIGH_BEAMS,"Keyboard EXPL+SHIFT+N",        _LC("InputEvent", "toggle truck high beams")},
    {"COMMON_TOGGLE_TRUCK_FOG_LIGHTS",EV_COMMON_TOGGLE_TRUCK_FOG_LIGHTS,"Keyboard EXPL+ALT+N",          _LC("InputEvent", "toggle truck fog lights")},
    {"COMMON_TOGGLE_CUSTOM_PARTICLES",EV_COMMON_TOGGLE_CUSTOM_PARTICLES,"Keyboard G",                   _LC("InputEvent", "toggle particle cannon")},
    {"COMMON_TOGGLE_REPLAY_MODE",     EV_COMMON_TOGGLE_REPLAY_MODE,     "Keyboard EXPL+CTRL+J",         _LC("InputEvent", "enable or disable replay mode")},
    {"COMMON_REPLAY_FORWARD",         EV_COMMON_REPLAY_FORWARD,         "Keyboard EXPL+RIGHT",          _LC("InputEvent", "more replay forward")},
    {"COMMON_REPLAY_BACKWARD",        EV_COMMON_REPLAY_BACKWARD,        "Keyboard EXPL+LEFT",           _LC("InputEvent", "more replay backward")},
    {"COMMON_REPLAY_FAST_FORWARD",    EV_COMMON_REPLAY_FAST_FORWARD,    "Keyboard EXPL+SHIFT+RIGHT",    _LC("InputEvent", "move replay fast forward")},
    {"COMMON_REPLAY_FAST_BACKWARD",   EV_COMMON_REPLAY_FAST_BACKWARD,   "Keyboard EXPL+SHIFT+LEFT",     _LC("InputEvent", "move replay fast backward")},

    // Common: UI
    {"COMMON_CONSOLE_TOGGLE",         EV_COMMON_CONSOLE_TOGGLE,         "Keyboard EXPL+GRAVE",          _LC("InputEvent", "show / hide the console")},
    {"COMMON_ENTER_CHATMODE",         EV_COMMON_ENTER_CHATMODE,         "Keyboard EXPL+Y",              _LC("InputEvent", "enter the chat")},
    {"COMMON_SEND_CHAT",              EV_COMMON_SEND_CHAT,              "Keyboard RETURN",              _LC("InputEvent", "sends the entered text")},
    {"COMMON_HIDE_GUI",               EV_COMMON_HIDE_GUI,               "Keyboard EXPL+U",              _LC("InputEvent", "hide all GUI elements")},
    {"COMMON_TOGGLE_DASHBOARD",       EV_COMMON_TOGGLE_DASHBOARD,       "Keyboard EXPL+CTRL+U",         _LC("InputEvent", "display or hide the dashboard overlay")},
    {"COMMON_TOGGLE_DEBUG_VIEW",      EV_COMMON_TOGGLE_DEBUG_VIEW,      "Keyboard EXPL+K",              _LC("InputEvent", "toggle debug view")},
    {"COMMON_CYCLE_DEBUG_VIEWS",      EV_COMMON_CYCLE_DEBUG_VIEWS,      "Keyboard EXPL+CTRL+K",         _LC("InputEvent", "cycle debug views")},
    {"COMMON_TRUCK_INFO",             EV_COMMON_TRUCK_INFO,             "Keyboard EXPL+T",              _LC("InputEvent", "toggle truck HUD")},
    {"COMMON_TRUCK_DESCRIPTION",      EV_COMMON_TRUCK_DESCRIPTION,      "Keyboard EXPL+CTRL+T",         _LC("InputEvent", "toggle truck description")},
    {"COMMON_NETCHATDISPLAY",         EV_COMMON_NETCHATDISPLAY,         "Keyboard EXPL+SHIFT+U",        _LC("InputEvent", "display or hide net chat")},
    {"COMMON_NETCHATMODE",            EV_COMMON_NETCHATMODE,            "Keyboard EXPL+CTRL+U",         _LC("InputEvent", "toggle between net chat display modes")},
    {"COMMON_TOGGLE_STATS",           EV_COMMON_TOGGLE_STATS,           "Keyboard EXPL+F",              _LC("InputEvent", "toggle Ogre statistics (FPS etc.)")},

    // Common: savegames
    {"COMMON_QUICKSAVE_01",           EV_COMMON_QUICKSAVE_01,           "Keyboard EXPL+ALT+CTRL+1",     _LC("InputEvent", "save scene in slot 01")},
    {"COMMON_QUICKSAVE_02",           EV_COMMON_QUICKSAVE_02,           "Keyboard EXPL+ALT+CTRL+2",     _LC("InputEvent", "save scene in slot 02")},
    {"COMMON_QUICKSAVE_03",           EV_COMMON_QUICKSAVE_03,           "Keyboard EXPL+ALT+CTRL+3",     _LC("InputEvent", "save scene in slot 03")},
    {"COMMON_QUICKSAVE_04",           EV_COMMON_QUICKSAVE_04,           "Keyboard EXPL+ALT+CTRL+4",     _LC("InputEvent", "save scene in slot 04")},
    {"COMMON_QUICKSAVE_05",           EV_COMMON_QUICKSAVE_05,           "Keyboard EXPL+ALT+CTRL+5",     _LC("InputEvent", "save scene in slot 05")},
    {"COMMON_QUICKSAVE_06",           EV_COMMON_QUICKSAVE_06,           "Keyboard EXPL+ALT+CTRL+6",     _LC("InputEvent", "save scene in slot 06")},
    {"COMMON_QUICKSAVE_07",           EV_COMMON_QUICKSAVE_07,           "Keyboard EXPL+ALT+CTRL+7",     _LC("InputEvent", "save scene in slot 07")},
    {"COMMON_QUICKSAVE_08",           EV_COMMON_QUICKSAVE_08,           "Keyboard EXPL+ALT+CTRL+8",     _LC("InputEvent", "save scene in slot 08")},
    {"COMMON_QUICKSAVE_09",           EV_COMMON_QUICKSAVE_09,           "Keyboard EXPL+ALT+CTRL+9",     _LC("InputEvent", "save scene in slot 09")},
    {"COMMON_QUICKSAVE_10",           EV_COMMON_QUICKSAVE_10,           "Keyboard EXPL+ALT+CTRL+0",     _LC("InputEvent", "save scene in slot 10")},

    {"COMMON_QUICKLOAD_01",           EV_COMMON_QUICKLOAD_01,           "Keyboard EXPL+ALT+1",          _LC("InputEvent", "load scene from slot 01")},
    {"COMMON_QUICKLOAD_02",           EV_COMMON_QUICKLOAD_02,           "Keyboard EXPL+ALT+2",          _LC("InputEvent", "load scene from slot 02")},
    {"COMMON_QUICKLOAD_03",           EV_COMMON_QUICKLOAD_03,           "Keyboard EXPL+ALT+3",          _LC("InputEvent", "load scene from slot 03")},
    {"COMMON_QUICKLOAD_04",           EV_COMMON_QUICKLOAD_04,           "Keyboard EXPL+ALT+4",          _LC("InputEvent", "load scene from slot 04")},
    {"COMMON_QUICKLOAD_05",           EV_COMMON_QUICKLOAD_05,           "Keyboard EXPL+ALT+5",          _LC("InputEvent", "load scene from slot 05")},
    {"COMMON_QUICKLOAD_06",           EV_COMMON_QUICKLOAD_06,           "Keyboard EXPL+ALT+6",          _LC("InputEvent", "load scene from slot 06")},
    {"COMMON_QUICKLOAD_07",           EV_COMMON_QUICKLOAD_07,           "Keyboard EXPL+ALT+7",          _LC("InputEvent", "load scene from slot 07")},
    {"COMMON_QUICKLOAD_08",           EV_COMMON_QUICKLOAD_08,           "Keyboard EXPL+ALT+8",          _LC("InputEvent", "load scene from slot 08")},
    {"COMMON_QUICKLOAD_09",           EV_COMMON_QUICKLOAD_09,           "Keyboard EXPL+ALT+9",          _LC("InputEvent", "load scene from slot 09")},
    {"COMMON_QUICKLOAD_10",           EV_COMMON_QUICKLOAD_10,           "Keyboard EXPL+ALT+0",          _LC("InputEvent", "load scene from slot 10")},

    // Truck: generic
    {"TRUCK_ACCELERATE",              EV_TRUCK_ACCELERATE,              "Keyboard UP",                  _LC("InputEvent", "accelerate the truck")},
    {"TRUCK_ACCELERATE_MODIFIER_25",  EV_TRUCK_ACCELERATE_MODIFIER_25,  "Keyboard ALT+UP",              _LC("InputEvent", "accelerate with 25 percent pedal input")},
    {"TRUCK_ACCELERATE_MODIFIER_50",  EV_TRUCK_ACCELERATE_MODIFIER_50,  "Keyboard CTRL+UP",             _LC("InputEvent", "accelerate with 50 percent pedal input")},
    {"TRUCK_BLINK_LEFT",              EV_TRUCK_BLINK_LEFT,              "Keyboard EXPL+COMMA",          _LC("InputEvent", "toggle left direction indicator (blinker)")},
    {"TRUCK_BLINK_RIGHT",             EV_TRUCK_BLINK_RIGHT,             "Keyboard EXPL+PERIOD",         _LC("InputEvent", "toggle right direction indicator (blinker)")},
    {"TRUCK_BLINK_WARN",              EV_TRUCK_BLINK_WARN,              "Keyboard EXPL+MINUS",          _LC("InputEvent", "toggle all direction indicators")},
    {"TRUCK_BRAKE",                   EV_TRUCK_BRAKE,                   "Keyboard DOWN",                _LC("InputEvent", "brake")},
    {"TRUCK_BRAKE_MODIFIER_25",       EV_TRUCK_BRAKE_MODIFIER_25,       "Keyboard ALT+DOWN",            _LC("InputEvent", "brake with 25 percent pedal input")},
    {"TRUCK_BRAKE_MODIFIER_50",       EV_TRUCK_BRAKE_MODIFIER_50,       "Keyboard CTRL+DOWN",           _LC("InputEvent", "brake with 50 percent pedal input")},
    {"TRUCK_HORN",                    EV_TRUCK_HORN,                    "Keyboard H",                   _LC("InputEvent", "truck horn")},
    {"TRUCK_LIGHTTOGGLE1",            EV_TRUCK_LIGHTTOGGLE01,           "Keyboard EXPL+CTRL+1",         _LC("InputEvent", "toggle custom light 1")},
    {"TRUCK_LIGHTTOGGLE2",            EV_TRUCK_LIGHTTOGGLE02,           "Keyboard EXPL+CTRL+2",         _LC("InputEvent", "toggle custom light 2")},
    {"TRUCK_LIGHTTOGGLE3",            EV_TRUCK_LIGHTTOGGLE03,           "Keyboard EXPL+CTRL+3",         _LC("InputEvent", "toggle custom light 3")},
    {"TRUCK_LIGHTTOGGLE4",            EV_TRUCK_LIGHTTOGGLE04,           "Keyboard EXPL+CTRL+4",         _LC("InputEvent", "toggle custom light 4")},
    {"TRUCK_LIGHTTOGGLE5",            EV_TRUCK_LIGHTTOGGLE05,           "Keyboard EXPL+CTRL+5",         _LC("InputEvent", "toggle custom light 5")},
    {"TRUCK_LIGHTTOGGLE6",            EV_TRUCK_LIGHTTOGGLE06,           "Keyboard EXPL+CTRL+6",         _LC("InputEvent", "toggle custom light 6")},
    {"TRUCK_LIGHTTOGGLE7",            EV_TRUCK_LIGHTTOGGLE07,           "Keyboard EXPL+CTRL+7",         _LC("InputEvent", "toggle custom light 7")},
    {"TRUCK_LIGHTTOGGLE8",            EV_TRUCK_LIGHTTOGGLE08,           "Keyboard EXPL+CTRL+8",         _LC("InputEvent", "toggle custom light 8")},
    {"TRUCK_LIGHTTOGGLE9",            EV_TRUCK_LIGHTTOGGLE09,           "Keyboard EXPL+CTRL+9",         _LC("InputEvent", "toggle custom light 9")},
    {"TRUCK_LIGHTTOGGLE10",           EV_TRUCK_LIGHTTOGGLE10,           "Keyboard EXPL+CTRL+0",         _LC("InputEvent", "toggle custom light 10")},
    {"TRUCK_PARKING_BRAKE",           EV_TRUCK_PARKING_BRAKE,           "Keyboard P",                   _LC("InputEvent", "toggle parking brake")},
    {"TRUCK_TRAILER_PARKING_BRAKE",   EV_TRUCK_TRAILER_PARKING_BRAKE,   "Keyboard EXPL+CTRL+P",         _LC("InputEvent", "toggle trailer parking brake")},
    {"TRUCK_ANTILOCK_BRAKE",          EV_TRUCK_ANTILOCK_BRAKE,          "Keyboard EXPL+SHIFT+B",        _LC("InputEvent", "toggle antilock brake")},
    {"TRUCK_TOGGLE_VIDEOCAMERA",      EV_TRUCK_TOGGLE_VIDEOCAMERA,      "Keyboard EXPL+CTRL+V",         _LC("InputEvent", "toggle videocamera")},
    {"TRUCK_TRACTION_CONTROL",        EV_TRUCK_TRACTION_CONTROL,        "Keyboard EXPL+SHIFT+T",        _LC("InputEvent", "toggle traction control")},
    {"TRUCK_CRUISE_CONTROL",          EV_TRUCK_CRUISE_CONTROL,          "Keyboard EXPL+SPACE",          _LC("InputEvent", "toggle cruise control")},
    {"TRUCK_CRUISE_CONTROL_READJUST", EV_TRUCK_CRUISE_CONTROL_READJUST, "Keyboard EXPL+CTRL+SPACE",     _LC("InputEvent", "match target speed / rpm with current truck speed / rpm")},
    {"TRUCK_CRUISE_CONTROL_ACCL",     EV_TRUCK_CRUISE_CONTROL_ACCL,     "Keyboard EXPL+CTRL+R",         _LC("InputEvent", "increase target speed / rpm")},
    {"TRUCK_CRUISE_CONTROL_DECL",     EV_TRUCK_CRUISE_CONTROL_DECL,     "Keyboard EXPL+CTRL+F",         _LC("InputEvent", "decrease target speed / rpm")},
    {"TRUCK_STARTER",                 EV_TRUCK_STARTER,                 "Keyboard S",                   _LC("InputEvent", "hold to start the engine")},
    {"TRUCK_STEER_LEFT",              EV_TRUCK_STEER_LEFT,              "Keyboard LEFT",                _LC("InputEvent", "steer left")},
    {"TRUCK_STEER_RIGHT",             EV_TRUCK_STEER_RIGHT,             "Keyboard RIGHT",               _LC("InputEvent", "steer right")},
    {"TRUCK_TOGGLE_CONTACT",          EV_TRUCK_TOGGLE_CONTACT,          "Keyboard X",                   _LC("InputEvent", "toggle ignition")},
    {"TRUCK_TOGGLE_FORWARDCOMMANDS",  EV_TRUCK_TOGGLE_FORWARDCOMMANDS,  "Keyboard EXPL+CTRL+SHIFT+F",   _LC("InputEvent", "toggle forwardcommands")},
    {"TRUCK_TOGGLE_IMPORTCOMMANDS",   EV_TRUCK_TOGGLE_IMPORTCOMMANDS,   "Keyboard EXPL+CTRL+SHIFT+I",   _LC("InputEvent", "toggle importcommands")},
    {"TRUCK_TOGGLE_PHYSICS",          EV_TRUCK_TOGGLE_PHYSICS,          "Keyboard END",                 _LC("InputEvent", "toggle physics")},
    {"TRUCK_TOGGLE_INTER_AXLE_DIFF",  EV_TRUCK_TOGGLE_INTER_AXLE_DIFF,  "Keyboard EXPL+ALT+W",          _LC("InputEvent", "cycle between available inter axle differential modes")},
    {"TRUCK_TOGGLE_INTER_WHEEL_DIFF", EV_TRUCK_TOGGLE_INTER_WHEEL_DIFF, "Keyboard EXPL+W",              _LC("InputEvent", "cycle between available inter wheel differential modes")},
    {"TRUCK_TOGGLE_TCASE_4WD_MODE",   EV_TRUCK_TOGGLE_TCASE_4WD_MODE,   "Keyboard EXPL+CTRL+W",         _LC("InputEvent", "toggle transfer case mode")},
    {"TRUCK_TOGGLE_TCASE_GEAR_RATIO", EV_TRUCK_TOGGLE_TCASE_GEAR_RATIO, "Keyboard EXPL+SHIFT+W",        _LC("InputEvent", "toggle transfer case gear ratio")},
    {"TRUCK_LEFT_MIRROR_LEFT",        EV_TRUCK_LEFT_MIRROR_LEFT,        "Keyboard EXPL+SEMICOLON",      _LC("InputEvent", "move left mirror to the left")},
    {"TRUCK_LEFT_MIRROR_RIGHT",       EV_TRUCK_LEFT_MIRROR_RIGHT,       "Keyboard EXPL+CTRL+SEMICOLON", _LC("InputEvent", "move left mirror to the right")},
    {"TRUCK_RIGHT_MIRROR_LEFT",       EV_TRUCK_RIGHT_MIRROR_LEFT,       "Keyboard EXPL+COLON",          _LC("InputEvent", "more right mirror to the left")},
    {"TRUCK_RIGHT_MIRROR_RIGHT",      EV_TRUCK_RIGHT_MIRROR_RIGHT,      "Keyboard EXPL+CTRL+COLON",     _LC("InputEvent", "move right mirror to the right")},

    // Truck: transmission
    {"TRUCK_AUTOSHIFT_DOWN",          EV_TRUCK_AUTOSHIFT_DOWN,          "Keyboard PGDOWN",              _LC("InputEvent", "shift automatic transmission one gear down")},
    {"TRUCK_AUTOSHIFT_UP",            EV_TRUCK_AUTOSHIFT_UP,            "Keyboard PGUP",                _LC("InputEvent", "shift automatic transmission one gear up")},
    {"TRUCK_MANUAL_CLUTCH",           EV_TRUCK_MANUAL_CLUTCH,           "Keyboard LSHIFT",              _LC("InputEvent", "manual clutch (for manual transmission)")},
    {"TRUCK_SHIFT_DOWN",              EV_TRUCK_SHIFT_DOWN,              "Keyboard Z",                   _LC("InputEvent", "shift one gear down in manual transmission mode")},
    {"TRUCK_SHIFT_NEUTRAL",           EV_TRUCK_SHIFT_NEUTRAL,           "Keyboard D",                   _LC("InputEvent", "shift to neutral gear in manual transmission mode")},
    {"TRUCK_SHIFT_UP",                EV_TRUCK_SHIFT_UP,                "Keyboard A",                   _LC("InputEvent", "shift one gear up in manual transmission mode")},
    {"TRUCK_SHIFT_GEAR_REVERSE",      EV_TRUCK_SHIFT_GEAR_REVERSE,      "",                             _LC("InputEvent", "shift directly to reverse gear")},
    {"TRUCK_SHIFT_GEAR1",             EV_TRUCK_SHIFT_GEAR01,            "",                             _LC("InputEvent", "shift directly to first gear")},
    {"TRUCK_SHIFT_GEAR2",             EV_TRUCK_SHIFT_GEAR02,            "",                             _LC("InputEvent", "shift directly to second gear")},
    {"TRUCK_SHIFT_GEAR3",             EV_TRUCK_SHIFT_GEAR03,            "",                             _LC("InputEvent", "shift directly to third gear")},
    {"TRUCK_SHIFT_GEAR4",             EV_TRUCK_SHIFT_GEAR04,            "",                             _LC("InputEvent", "shift directly to fourth gear")},
    {"TRUCK_SHIFT_GEAR5",             EV_TRUCK_SHIFT_GEAR05,            "",                             _LC("InputEvent", "shift directly to 5th gear")},
    {"TRUCK_SHIFT_GEAR6",             EV_TRUCK_SHIFT_GEAR06,            "",                             _LC("InputEvent", "shift directly to 6th gear")},
    {"TRUCK_SHIFT_GEAR7",             EV_TRUCK_SHIFT_GEAR07,            "",                             _LC("InputEvent", "shift directly to 7th gear")},
    {"TRUCK_SHIFT_GEAR8",             EV_TRUCK_SHIFT_GEAR08,            "",                             _LC("InputEvent", "shift directly to 8th gear")},
    {"TRUCK_SHIFT_GEAR9",             EV_TRUCK_SHIFT_GEAR09,            "",                             _LC("InputEvent", "shift directly to 9th gear")},
    {"TRUCK_SHIFT_GEAR10",            EV_TRUCK_SHIFT_GEAR10,            "",                             _LC("InputEvent", "shift directly to 10th gear")},
    {"TRUCK_SHIFT_GEAR11",            EV_TRUCK_SHIFT_GEAR11,            "",                             _LC("InputEvent", "shift directly to 11th gear")},
    {"TRUCK_SHIFT_GEAR12",            EV_TRUCK_SHIFT_GEAR12,            "",                             _LC("InputEvent", "shift directly to 12th gear")},
    {"TRUCK_SHIFT_GEAR13",            EV_TRUCK_SHIFT_GEAR13,            "",                             _LC("InputEvent", "shift directly to 13th gear")},
    {"TRUCK_SHIFT_GEAR14",            EV_TRUCK_SHIFT_GEAR14,            "",                             _LC("InputEvent", "shift directly to 14th gear")},
    {"TRUCK_SHIFT_GEAR15",            EV_TRUCK_SHIFT_GEAR15,            "",                             _LC("InputEvent", "shift directly to 15th gear")},
    {"TRUCK_SHIFT_GEAR16",            EV_TRUCK_SHIFT_GEAR16,            "",                             _LC("InputEvent", "shift directly to 16th gear")},
    {"TRUCK_SHIFT_GEAR17",            EV_TRUCK_SHIFT_GEAR17,            "",                             _LC("InputEvent", "shift directly to 17th gear")},
    {"TRUCK_SHIFT_GEAR18",            EV_TRUCK_SHIFT_GEAR18,            "",                             _LC("InputEvent", "shift directly to 18th gear")},
    {"TRUCK_SHIFT_LOWRANGE",          EV_TRUCK_SHIFT_LOWRANGE,          "",                             _LC("InputEvent", "sets low range (1-6) for H-shaft")},
    {"TRUCK_SHIFT_MIDRANGE",          EV_TRUCK_SHIFT_MIDRANGE,          "",                             _LC("InputEvent", "sets middle range (7-12) for H-shaft")},
    {"TRUCK_SHIFT_HIGHRANGE",         EV_TRUCK_SHIFT_HIGHRANGE,         "",                             _LC("InputEvent", "sets high range (13-18) for H-shaft")},
    {"TRUCK_SWITCH_SHIFT_MODES",      EV_TRUCK_SWITCH_SHIFT_MODES,      "Keyboard Q",                   _LC("InputEvent", "toggle between transmission modes")},

    // Airplane
    {"AIRPLANE_STEER_RIGHT",          EV_AIRPLANE_STEER_RIGHT,          "Keyboard RIGHT",               _LC("InputEvent", "steer right")},
    {"AIRPLANE_BRAKE",                EV_AIRPLANE_BRAKE,                "Keyboard B",                   _LC("InputEvent", "normal brake for an aircraft")},
    {"AIRPLANE_ELEVATOR_DOWN",        EV_AIRPLANE_ELEVATOR_DOWN,        "Keyboard DOWN",                _LC("InputEvent", "pull the elevator down in an aircraft.")},
    {"AIRPLANE_ELEVATOR_UP",          EV_AIRPLANE_ELEVATOR_UP,          "Keyboard UP",                  _LC("InputEvent", "pull the elevator up in an aircraft.")},
    {"AIRPLANE_FLAPS_FULL",           EV_AIRPLANE_FLAPS_FULL,           "Keyboard CTRL+2",              _LC("InputEvent", "full flaps in an aircraft.")},
    {"AIRPLANE_FLAPS_LESS",           EV_AIRPLANE_FLAPS_LESS,           "Keyboard EXPL+1",              _LC("InputEvent", "one step less flaps.")},
    {"AIRPLANE_FLAPS_MORE",           EV_AIRPLANE_FLAPS_MORE,           "Keyboard EXPL+2",              _LC("InputEvent", "one step more flaps.")},
    {"AIRPLANE_FLAPS_NONE",           EV_AIRPLANE_FLAPS_NONE,           "Keyboard CTRL+1",              _LC("InputEvent", "no flaps.")},
    {"AIRPLANE_PARKING_BRAKE",        EV_AIRPLANE_PARKING_BRAKE,        "Keyboard P",                   _LC("InputEvent", "airplane parking brake.")},
    {"AIRPLANE_REVERSE",              EV_AIRPLANE_REVERSE,              "Keyboard R",                   _LC("InputEvent", "reverse the turboprops")},
    {"AIRPLANE_RUDDER_LEFT",          EV_AIRPLANE_RUDDER_LEFT,          "Keyboard Z",                   _LC("InputEvent", "rudder left")},
    {"AIRPLANE_RUDDER_RIGHT",         EV_AIRPLANE_RUDDER_RIGHT,         "Keyboard X",                   _LC("InputEvent", "rudder right")},
    {"AIRPLANE_STEER_LEFT",           EV_AIRPLANE_STEER_LEFT,           "Keyboard LEFT",                _LC("InputEvent", "steer left")},
    {"AIRPLANE_STEER_RIGHT",          EV_AIRPLANE_STEER_RIGHT,          "Keyboard RIGHT",               _LC("InputEvent", "steer right")},
    {"AIRPLANE_THROTTLE_AXIS",        EV_AIRPLANE_THROTTLE_AXIS,        "None",                         _LC("InputEvent", "throttle axis. Only use this if you have fitting hardware :) (i.e. a Slider)")},
    {"AIRPLANE_THROTTLE_DOWN",        EV_AIRPLANE_THROTTLE_DOWN,        "Keyboard EXPL+PGDOWN",         _LC("InputEvent", "decreases the airplane thrust")},
    {"AIRPLANE_THROTTLE_FULL",        EV_AIRPLANE_THROTTLE_FULL,        "Keyboard CTRL+PGUP",           _LC("InputEvent", "full thrust")},
    {"AIRPLANE_THROTTLE_NO",          EV_AIRPLANE_THROTTLE_NO,          "Keyboard CTRL+PGDOWN",         _LC("InputEvent", "no thrust")},
    {"AIRPLANE_THROTTLE_UP",          EV_AIRPLANE_THROTTLE_UP,          "Keyboard EXPL+PGUP",           _LC("InputEvent", "increase the airplane thrust")},
    {"AIRPLANE_TOGGLE_ENGINES",       EV_AIRPLANE_TOGGLE_ENGINES,       "Keyboard CTRL+HOME",           _LC("InputEvent", "switch all engines on / off")},
    {"AIRPLANE_AIRBRAKES_NONE",       EV_AIRPLANE_AIRBRAKES_NONE,       "Keyboard CTRL+3",              _LC("InputEvent", "no airbrakes")},
    {"AIRPLANE_AIRBRAKES_FULL",       EV_AIRPLANE_AIRBRAKES_FULL,       "Keyboard CTRL+4",              _LC("InputEvent", "full airbrakes")},
    {"AIRPLANE_AIRBRAKES_LESS",       EV_AIRPLANE_AIRBRAKES_LESS,       "Keyboard EXPL+3",              _LC("InputEvent", "less airbrakes")},
    {"AIRPLANE_AIRBRAKES_MORE",       EV_AIRPLANE_AIRBRAKES_MORE,       "Keyboard EXPL+4",              _LC("InputEvent", "more airbrakes")},
    {"AIRPLANE_THROTTLE",             EV_AIRPLANE_THROTTLE,             "",                             _LC("InputEvent", "airplane throttle")},

    // Boat
    {"BOAT_CENTER_RUDDER",            EV_BOAT_CENTER_RUDDER,            "Keyboard PGDOWN",              _LC("InputEvent", "center the rudder")},
    {"BOAT_REVERSE",                  EV_BOAT_REVERSE,                  "Keyboard PGUP",                _LC("InputEvent", "no thrust")},
    {"BOAT_STEER_LEFT",               EV_BOAT_STEER_LEFT,               "Keyboard LEFT",                _LC("InputEvent", "steer left a step")},
    {"BOAT_STEER_LEFT_AXIS",          EV_BOAT_STEER_LEFT_AXIS,          "None",                         _LC("InputEvent", "steer left (analog value!)")},
    {"BOAT_STEER_RIGHT",              EV_BOAT_STEER_RIGHT,              "Keyboard RIGHT",               _LC("InputEvent", "steer right a step")},
    {"BOAT_STEER_RIGHT_AXIS",         EV_BOAT_STEER_RIGHT_AXIS,         "None",                         _LC("InputEvent", "steer right (analog value!)")},
    {"BOAT_THROTTLE_AXIS",            EV_BOAT_THROTTLE_AXIS,            "None",                         _LC("InputEvent", "throttle axis. Only use this if you have fitting hardware :) (i.e. a Slider)")},
    {"BOAT_THROTTLE_DOWN",            EV_BOAT_THROTTLE_DOWN,            "Keyboard DOWN",                _LC("InputEvent", "decrease throttle")},
    {"BOAT_THROTTLE_UP",              EV_BOAT_THROTTLE_UP,              "Keyboard UP",                  _LC("InputEvent", "increase throttle")},

    // Commands
    {"COMMANDS_01",                   EV_COMMANDS_01,                   "Keyboard EXPL+F1",             _LC("InputEvent", "Command 1")},
    {"COMMANDS_02",                   EV_COMMANDS_02,                   "Keyboard EXPL+F2",             _LC("InputEvent", "Command 2")},
    {"COMMANDS_03",                   EV_COMMANDS_03,                   "Keyboard EXPL+F3",             _LC("InputEvent", "Command 3")},
    {"COMMANDS_04",                   EV_COMMANDS_04,                   "Keyboard EXPL+F4",             _LC("InputEvent", "Command 4")},
    {"COMMANDS_05",                   EV_COMMANDS_05,                   "Keyboard EXPL+F5",             _LC("InputEvent", "Command 5")},
    {"COMMANDS_06",                   EV_COMMANDS_06,                   "Keyboard EXPL+F6",             _LC("InputEvent", "Command 6")},
    {"COMMANDS_07",                   EV_COMMANDS_07,                   "Keyboard EXPL+F7",             _LC("InputEvent", "Command 7")},
    {"COMMANDS_08",                   EV_COMMANDS_08,                   "Keyboard EXPL+F8",             _LC("InputEvent", "Command 8")},
    {"COMMANDS_09",                   EV_COMMANDS_09,                   "Keyboard EXPL+F9",             _LC("InputEvent", "Command 9")},
    {"COMMANDS_10",                   EV_COMMANDS_10,                   "Keyboard EXPL+F10",            _LC("InputEvent", "Command 10")},
    {"COMMANDS_11",                   EV_COMMANDS_11,                   "Keyboard EXPL+F11",            _LC("InputEvent", "Command 11")},
    {"COMMANDS_12",                   EV_COMMANDS_12,                   "Keyboard EXPL+F12",            _LC("InputEvent", "Command 12")},
    {"COMMANDS_13",                   EV_COMMANDS_13,                   "Keyboard EXPL+CTRL+F1",        _LC("InputEvent", "Command 13")},
    {"COMMANDS_14",                   EV_COMMANDS_14,                   "Keyboard EXPL+CTRL+F2",        _LC("InputEvent", "Command 14")},
    {"COMMANDS_15",                   EV_COMMANDS_15,                   "Keyboard EXPL+CTRL+F3",        _LC("InputEvent", "Command 15")},
    {"COMMANDS_16",                   EV_COMMANDS_16,                   "Keyboard EXPL+CTRL+F4",        _LC("InputEvent", "Command 16")},
    {"COMMANDS_17",                   EV_COMMANDS_17,                   "Keyboard EXPL+CTRL+F5",        _LC("InputEvent", "Command 17")},
    {"COMMANDS_18",                   EV_COMMANDS_18,                   "Keyboard EXPL+CTRL+F6",        _LC("InputEvent", "Command 18")},
    {"COMMANDS_19",                   EV_COMMANDS_19,                   "Keyboard EXPL+CTRL+F7",        _LC("InputEvent", "Command 19")},
    {"COMMANDS_20",                   EV_COMMANDS_20,                   "Keyboard EXPL+CTRL+F8",        _LC("InputEvent", "Command 20")},
    {"COMMANDS_21",                   EV_COMMANDS_21,                   "Keyboard EXPL+CTRL+F9",        _LC("InputEvent", "Command 21")},
    {"COMMANDS_22",                   EV_COMMANDS_22,                   "Keyboard EXPL+CTRL+F10",       _LC("InputEvent", "Command 22")},
    {"COMMANDS_23",                   EV_COMMANDS_23,                   "Keyboard EXPL+CTRL+F11",       _LC("InputEvent", "Command 23")},
    {"COMMANDS_24",                   EV_COMMANDS_24,                   "Keyboard EXPL+CTRL+F12",       _LC("InputEvent", "Command 24")},
    {"COMMANDS_25",                   EV_COMMANDS_25,                   "Keyboard EXPL+SHIFT+F1",       _LC("InputEvent", "Command 25")},
    {"COMMANDS_26",                   EV_COMMANDS_26,                   "Keyboard EXPL+SHIFT+F2",       _LC("InputEvent", "Command 26")},
    {"COMMANDS_27",                   EV_COMMANDS_27,                   "Keyboard EXPL+SHIFT+F3",       _LC("InputEvent", "Command 27")},
    {"COMMANDS_28",                   EV_COMMANDS_28,                   "Keyboard EXPL+SHIFT+F4",       _LC("InputEvent", "Command 28")},
    {"COMMANDS_29",                   EV_COMMANDS_29,                   "Keyboard EXPL+SHIFT+F5",       _LC("InputEvent", "Command 29")},
    {"COMMANDS_30",                   EV_COMMANDS_30,                   "Keyboard EXPL+SHIFT+F6",       _LC("InputEvent", "Command 30")},
    {"COMMANDS_31",                   EV_COMMANDS_31,                   "Keyboard EXPL+SHIFT+F7",       _LC("InputEvent", "Command 31")},
    {"COMMANDS_32",                   EV_COMMANDS_32,                   "Keyboard EXPL+SHIFT+F8",       _LC("InputEvent", "Command 32")},
    {"COMMANDS_33",                   EV_COMMANDS_33,                   "Keyboard EXPL+SHIFT+F9",       _LC("InputEvent", "Command 33")},
    {"COMMANDS_34",                   EV_COMMANDS_34,                   "Keyboard EXPL+SHIFT+F10",      _LC("InputEvent", "Command 34")},
    {"COMMANDS_35",                   EV_COMMANDS_35,                   "Keyboard EXPL+SHIFT+F11",      _LC("InputEvent", "Command 35")},
    {"COMMANDS_36",                   EV_COMMANDS_36,                   "Keyboard EXPL+SHIFT+F12",      _LC("InputEvent", "Command 36")},
    {"COMMANDS_37",                   EV_COMMANDS_37,                   "Keyboard EXPL+ALT+F1",         _LC("InputEvent", "Command 37")},
    {"COMMANDS_38",                   EV_COMMANDS_38,                   "Keyboard EXPL+ALT+F2",         _LC("InputEvent", "Command 38")},
    {"COMMANDS_39",                   EV_COMMANDS_39,                   "Keyboard EXPL+ALT+F3",         _LC("InputEvent", "Command 39")},
    {"COMMANDS_40",                   EV_COMMANDS_40,                   "Keyboard EXPL+ALT+F4",         _LC("InputEvent", "Command 40")},
    {"COMMANDS_41",                   EV_COMMANDS_41,                   "Keyboard EXPL+ALT+F5",         _LC("InputEvent", "Command 41")},
    {"COMMANDS_42",                   EV_COMMANDS_42,                   "Keyboard EXPL+ALT+F6",         _LC("InputEvent", "Command 42")},
    {"COMMANDS_43",                   EV_COMMANDS_43,                   "Keyboard EXPL+ALT+F7",         _LC("InputEvent", "Command 43")},
    {"COMMANDS_44",                   EV_COMMANDS_44,                   "Keyboard EXPL+ALT+F8",         _LC("InputEvent", "Command 44")},
    {"COMMANDS_45",                   EV_COMMANDS_45,                   "Keyboard EXPL+ALT+F9",         _LC("InputEvent", "Command 45")},
    {"COMMANDS_46",                   EV_COMMANDS_46,                   "Keyboard EXPL+ALT+F10",        _LC("InputEvent", "Command 46")},
    {"COMMANDS_47",                   EV_COMMANDS_47,                   "Keyboard EXPL+ALT+F11",        _LC("InputEvent", "Command 47")},
    {"COMMANDS_48",                   EV_COMMANDS_48,                   "Keyboard EXPL+ALT+F12",        _LC("InputEvent", "Command 48")},
    {"COMMANDS_49",                   EV_COMMANDS_49,                   "Keyboard EXPL+CTRL+SHIFT+F1",  _LC("InputEvent", "Command 49")},
    {"COMMANDS_50",                   EV_COMMANDS_50,                   "Keyboard EXPL+CTRL+SHIFT+F2",  _LC("InputEvent", "Command 50")},
    {"COMMANDS_51",                   EV_COMMANDS_51,                   "Keyboard EXPL+CTRL+SHIFT+F3",  _LC("InputEvent", "Command 51")},
    {"COMMANDS_52",                   EV_COMMANDS_52,                   "Keyboard EXPL+CTRL+SHIFT+F4",  _LC("InputEvent", "Command 52")},
    {"COMMANDS_53",                   EV_COMMANDS_53,                   "Keyboard EXPL+CTRL+SHIFT+F5",  _LC("InputEvent", "Command 53")},
    {"COMMANDS_54",                   EV_COMMANDS_54,                   "Keyboard EXPL+CTRL+SHIFT+F6",  _LC("InputEvent", "Command 54")},
    {"COMMANDS_55",                   EV_COMMANDS_55,                   "Keyboard EXPL+CTRL+SHIFT+F7",  _LC("InputEvent", "Command 55")},
    {"COMMANDS_56",                   EV_COMMANDS_56,                   "Keyboard EXPL+CTRL+SHIFT+F8",  _LC("InputEvent", "Command 56")},
    {"COMMANDS_57",                   EV_COMMANDS_57,                   "Keyboard EXPL+CTRL+SHIFT+F9",  _LC("InputEvent", "Command 57")},
    {"COMMANDS_58",                   EV_COMMANDS_58,                   "Keyboard EXPL+CTRL+SHIFT+F10", _LC("InputEvent", "Command 58")},
    {"COMMANDS_59",                   EV_COMMANDS_59,                   "Keyboard EXPL+CTRL+SHIFT+F11", _LC("InputEvent", "Command 59")},
    {"COMMANDS_60",                   EV_COMMANDS_60,                   "Keyboard EXPL+CTRL+SHIFT+F12", _LC("InputEvent", "Command 60")},
    {"COMMANDS_61",                   EV_COMMANDS_61,                   "Keyboard EXPL+CTRL+ALT+F1",    _LC("InputEvent", "Command 61")},
    {"COMMANDS_62",                   EV_COMMANDS_62,                   "Keyboard EXPL+CTRL+ALT+F2",    _LC("InputEvent", "Command 62")},
    {"COMMANDS_63",                   EV_COMMANDS_63,                   "Keyboard EXPL+CTRL+ALT+F3",    _LC("InputEvent", "Command 63")},
    {"COMMANDS_64",                   EV_COMMANDS_64,                   "Keyboard EXPL+CTRL+ALT+F4",    _LC("InputEvent", "Command 64")},
    {"COMMANDS_65",                   EV_COMMANDS_65,                   "Keyboard EXPL+CTRL+ALT+F5",    _LC("InputEvent", "Command 65")},
    {"COMMANDS_66",                   EV_COMMANDS_66,                   "Keyboard EXPL+CTRL+ALT+F6",    _LC("InputEvent", "Command 66")},
    {"COMMANDS_67",                   EV_COMMANDS_67,                   "Keyboard EXPL+CTRL+ALT+F7",    _LC("InputEvent", "Command 67")},
    {"COMMANDS_68",                   EV_COMMANDS_68,                   "Keyboard EXPL+CTRL+ALT+F8",    _LC("InputEvent", "Command 68")},
    {"COMMANDS_69",                   EV_COMMANDS_69,                   "Keyboard EXPL+CTRL+ALT+F9",    _LC("InputEvent", "Command 69")},
    {"COMMANDS_70",                   EV_COMMANDS_70,                   "Keyboard EXPL+CTRL+ALT+F10",   _LC("InputEvent", "Command 70")},
    {"COMMANDS_71",                   EV_COMMANDS_71,                   "Keyboard EXPL+CTRL+ALT+F11",   _LC("InputEvent", "Command 71")},
    {"COMMANDS_72",                   EV_COMMANDS_72,                   "Keyboard EXPL+CTRL+ALT+F12",   _LC("InputEvent", "Command 72")},
    {"COMMANDS_73",                   EV_COMMANDS_73,                   "Keyboard EXPL+CTRL+SHIFT+ALT+F1",_LC("InputEvent", "Command 73")},
    {"COMMANDS_74",                   EV_COMMANDS_74,                   "Keyboard EXPL+CTRL+SHIFT+ALT+F2",_LC("InputEvent", "Command 74")},
    {"COMMANDS_75",                   EV_COMMANDS_75,                   "Keyboard EXPL+CTRL+SHIFT+ALT+F3",_LC("InputEvent", "Command 75")},
    {"COMMANDS_76",                   EV_COMMANDS_76,                   "Keyboard EXPL+CTRL+SHIFT+ALT+F4",_LC("InputEvent", "Command 76")},
    {"COMMANDS_77",                   EV_COMMANDS_77,                   "Keyboard EXPL+CTRL+SHIFT+ALT+F5",_LC("InputEvent", "Command 77")},
    {"COMMANDS_78",                   EV_COMMANDS_78,                   "Keyboard EXPL+CTRL+SHIFT+ALT+F6",_LC("InputEvent", "Command 78")},
    {"COMMANDS_79",                   EV_COMMANDS_79,                   "Keyboard EXPL+CTRL+SHIFT+ALT+F7",_LC("InputEvent", "Command 79")},
    {"COMMANDS_80",                   EV_COMMANDS_80,                   "Keyboard EXPL+CTRL+SHIFT+ALT+F8",_LC("InputEvent", "Command 80")},
    {"COMMANDS_81",                   EV_COMMANDS_81,                   "Keyboard EXPL+CTRL+SHIFT+ALT+F9",_LC("InputEvent", "Command 81")},
    {"COMMANDS_82",                   EV_COMMANDS_82,                   "Keyboard EXPL+CTRL+SHIFT+ALT+F10",_LC("InputEvent", "Command 82")},
    {"COMMANDS_83",                   EV_COMMANDS_83,                   "Keyboard EXPL+CTRL+SHIFT+ALT+F11",_LC("InputEvent", "Command 83")},
    {"COMMANDS_84",                   EV_COMMANDS_84,                   "Keyboard EXPL+CTRL+SHIFT+ALT+F12",_LC("InputEvent", "Command 84")},

    // Character
    {"CHARACTER_BACKWARDS",           EV_CHARACTER_BACKWARDS,           "Keyboard S",                   _LC("InputEvent", "step backwards with the character")},
    {"CHARACTER_FORWARD",             EV_CHARACTER_FORWARD,             "Keyboard W",                   _LC("InputEvent", "step forward with the character")},
    {"CHARACTER_JUMP",                EV_CHARACTER_JUMP,                "Keyboard SPACE",               _LC("InputEvent", "let the character jump")},
    {"CHARACTER_LEFT",                EV_CHARACTER_LEFT,                "Keyboard LEFT",                _LC("InputEvent", "rotate character left")},
    {"CHARACTER_RIGHT",               EV_CHARACTER_RIGHT,               "Keyboard RIGHT",               _LC("InputEvent", "rotate character right")},
    {"CHARACTER_RUN",                 EV_CHARACTER_RUN,                 "Keyboard SHIFT+W",             _LC("InputEvent", "let the character run")},
    {"CHARACTER_SIDESTEP_LEFT",       EV_CHARACTER_SIDESTEP_LEFT,       "Keyboard A",                   _LC("InputEvent", "sidestep to the left")},
    {"CHARACTER_SIDESTEP_RIGHT",      EV_CHARACTER_SIDESTEP_RIGHT,      "Keyboard D",                   _LC("InputEvent", "sidestep to the right")},
    {"CHARACTER_ROT_UP",              EV_CHARACTER_ROT_UP,              "Keyboard UP",                  _LC("InputEvent", "rotate view up")},
    {"CHARACTER_ROT_DOWN",            EV_CHARACTER_ROT_DOWN,            "Keyboard DOWN",                _LC("InputEvent", "rotate view down")},

    // Camera
    {"CAMERA_CHANGE",                 EV_CAMERA_CHANGE,                 "Keyboard EXPL+C",              _LC("InputEvent", "change camera mode")},
    {"CAMERA_LOOKBACK",               EV_CAMERA_LOOKBACK,               "Keyboard NUMPAD1",             _LC("InputEvent", "look back (toggles between normal and lookback)")},
    {"CAMERA_RESET",                  EV_CAMERA_RESET,                  "Keyboard NUMPAD5",             _LC("InputEvent", "reset the camera position")},
    {"CAMERA_ROTATE_DOWN",            EV_CAMERA_ROTATE_DOWN,            "Keyboard NUMPAD2",             _LC("InputEvent", "rotate camera down")},
    {"CAMERA_ROTATE_LEFT",            EV_CAMERA_ROTATE_LEFT,            "Keyboard NUMPAD4",             _LC("InputEvent", "rotate camera left")},
    {"CAMERA_ROTATE_RIGHT",           EV_CAMERA_ROTATE_RIGHT,           "Keyboard NUMPAD6",             _LC("InputEvent", "rotate camera right")},
    {"CAMERA_ROTATE_UP",              EV_CAMERA_ROTATE_UP,              "Keyboard NUMPAD8",             _LC("InputEvent", "rotate camera up")},
    {"CAMERA_ZOOM_IN",                EV_CAMERA_ZOOM_IN,                "Keyboard EXPL+NUMPAD9",        _LC("InputEvent", "zoom camera in")},
    {"CAMERA_ZOOM_IN_FAST",           EV_CAMERA_ZOOM_IN_FAST,           "Keyboard SHIFT+NUMPAD9",       _LC("InputEvent", "zoom camera in faster")},
    {"CAMERA_ZOOM_OUT",               EV_CAMERA_ZOOM_OUT,               "Keyboard EXPL+NUMPAD3",        _LC("InputEvent", "zoom camera out")},
    {"CAMERA_ZOOM_OUT_FAST",          EV_CAMERA_ZOOM_OUT_FAST,          "Keyboard SHIFT+NUMPAD3",       _LC("InputEvent", "zoom camera out faster")},
    {"CAMERA_FREE_MODE_FIX",          EV_CAMERA_FREE_MODE_FIX,          "Keyboard EXPL+ALT+C",          _LC("InputEvent", "fix the camera to a position")},
    {"CAMERA_FREE_MODE",              EV_CAMERA_FREE_MODE,              "Keyboard EXPL+SHIFT+C",        _LC("InputEvent", "enable / disable free camera mode")},
    {"CAMERA_UP",                     EV_CAMERA_UP,                     "Keyboard Q",                   _LC("InputEvent", "move camera up")},
    {"CAMERA_DOWN",                   EV_CAMERA_DOWN,                   "Keyboard Z",                   _LC("InputEvent", "move camera down")},

    // Sky
    {"SKY_DECREASE_TIME",             EV_SKY_DECREASE_TIME,             "Keyboard EXPL+SUBTRACT",       _LC("InputEvent", "decrease day-time")},
    {"SKY_DECREASE_TIME_FAST",        EV_SKY_DECREASE_TIME_FAST,        "Keyboard SHIFT+SUBTRACT",      _LC("InputEvent", "decrease day-time a lot faster")},
    {"SKY_INCREASE_TIME",             EV_SKY_INCREASE_TIME,             "Keyboard EXPL+ADD",            _LC("InputEvent", "increase day-time")},
    {"SKY_INCREASE_TIME_FAST",        EV_SKY_INCREASE_TIME_FAST,        "Keyboard SHIFT+ADD",           _LC("InputEvent", "increase day-time a lot faster")},

    // Grass
    {"GRASS_LESS",                    EV_GRASS_LESS,                    "",                             _LC("InputEvent", "EXPERIMENTAL: remove some grass")},
    {"GRASS_MORE",                    EV_GRASS_MORE,                    "",                             _LC("InputEvent", "EXPERIMENTAL: add some grass")},
    {"GRASS_MOST",                    EV_GRASS_MOST,                    "",                             _LC("InputEvent", "EXPERIMENTAL: set maximum amount of grass")},
    {"GRASS_NONE",                    EV_GRASS_NONE,                    "",                             _LC("InputEvent", "EXPERIMENTAL: remove grass completely")},
    {"GRASS_SAVE",                    EV_GRASS_SAVE,                    "",                             _LC("InputEvent", "EXPERIMENTAL: save changes to the grass density image")},

    // Survey map
    {"SURVEY_MAP_TOGGLE_ICONS",       EV_SURVEY_MAP_TOGGLE_ICONS,       "Keyboard EXPL+CTRL+SHIFT+ALT+TAB",_LC("InputEvent", "toggle map icons")},
    {"SURVEY_MAP_TOGGLE",             EV_SURVEY_MAP_TOGGLE,             "Keyboard EXPL+CTRL+SHIFT+TAB", _LC("InputEvent", "toggle map")},
    {"SURVEY_MAP_CYCLE",              EV_SURVEY_MAP_CYCLE,              "Keyboard EXPL+TAB",            _LC("InputEvent", "cycle map modes")},
    {"SURVEY_MAP_ZOOM_IN",            EV_SURVEY_MAP_ZOOM_IN,            "Keyboard EXPL+CTRL+TAB",       _LC("InputEvent", "zoom in")},
    {"SURVEY_MAP_ZOOM_OUT",           EV_SURVEY_MAP_ZOOM_OUT,           "Keyboard EXPL+SHIFT+TAB",      _LC("InputEvent", "zoom out")},

    // Menu
    {"MENU_DOWN",                     EV_MENU_DOWN,                     "Keyboard DOWN",                _LC("InputEvent", "select next element in current category")},
    {"MENU_LEFT",                     EV_MENU_LEFT,                     "Keyboard LEFT",                _LC("InputEvent", "select previous category")},
    {"MENU_RIGHT",                    EV_MENU_RIGHT,                    "Keyboard RIGHT",               _LC("InputEvent", "select next category")},
    {"MENU_SELECT",                   EV_MENU_SELECT,                   "Keyboard EXPL+RETURN",         _LC("InputEvent", "select focussed item and close menu")},
    {"MENU_UP",                       EV_MENU_UP,                       "Keyboard UP",                  _LC("InputEvent", "select previous element in current category")},

    // Truck editing
    {"TRUCKEDIT_RELOAD",              EV_TRUCKEDIT_RELOAD,              "Keyboard EXPL+SHIFT+CTRL+R",   _LC("InputEvent", "reload truck")},

    // Road editing
    {"ROAD_EDITOR_POINT_INSERT",      EV_ROAD_EDITOR_POINT_INSERT,      "Keyboard EXPL+INSERT",         _LC("InputEvent", "insert road point") },
    {"ROAD_EDITOR_POINT_GOTO",        EV_ROAD_EDITOR_POINT_GOTO,        "Keyboard EXPL+G",              _LC("InputEvent", "go to road point") },
    {"ROAD_EDITOR_POINT_SET_POS",     EV_ROAD_EDITOR_POINT_SET_POS,     "Keyboard EXPL+M",              _LC("InputEvent", "set road point position") },
    {"ROAD_EDITOR_POINT_DELETE",      EV_ROAD_EDITOR_POINT_DELETE,      "Keyboard EXPL+DELETE",         _LC("InputEvent", "delete road point") },
    {"ROAD_EDITOR_REBUILD_MESH",      EV_ROAD_EDITOR_REBUILD_MESH,      "Keyboard EXPL+B",              _LC("InputEvent", "regenerate road mesh") },

    // The end, DO NOT MODIFY
    {"", -1, "", ""},
};

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#define strnlen(str,len) strlen(str)
#endif

using namespace std;
using namespace Ogre;

const std::string InputEngine::DEFAULT_MAPFILE = "input.map";

InputEngine::InputEngine() :
     free_joysticks(0)
    , uniqueCounter(0)
{
    initAllKeys();
    setup();
}

void InputEngine::setup()
{


#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    if (App::io_input_grab_mode->getEnum<IoInputGrabMode>() != IoInputGrabMode::ALL)
    {
        ShowCursor(FALSE);
    }
#endif


    // load default mappings
    this->loadConfigFile(-1);

    // then load device (+OS) specific mappings
    for (int i = 0; i < free_joysticks; ++i)
    {
        this->loadConfigFile(i);
    }
    completeMissingEvents();
}

String InputEngine::getKeyNameForKeyCode(SDL_Keycode keycode)
{
    if (keycode & KMOD_SHIFT)
        return "SHIFT";
    if (keycode & KMOD_CTRL)
        return "CTRL";
    if (keycode & KMOD_ALT)
        return "ALT";
    for (allit = allkeys.begin(); allit != allkeys.end(); allit++)
    {
        if (allit->second == keycode)
            return allit->first;
    }
    return "unknown";
}

void InputEngine::Capture()
{
    App::GetAppContext()->pollEvents();
}

/* --- Joystick Events ------------------------------------------ */
void InputEngine::ProcessJoystickEvent(const OgreBites::AxisEvent& arg)
{
    /* FIXME-SDL
    int i = arg.device->getID();
    if (i < 0 || i >= MAX_JOYSTICKS)
        i = 0;
    joyState[i] = arg.state;
    */
}

/* --- Key Events ------------------------------------------ */
void InputEngine::ProcessKeyPress(const OgreBites::KeyboardEvent& arg)
{
    keyState[arg.keysym.sym] = true;
}

void InputEngine::ProcessKeyRelease(const OgreBites::KeyboardEvent& arg)
{
    keyState[arg.keysym.sym] = false;
}



/* --- Custom Methods ------------------------------------------ */
void InputEngine::resetKeys()
{
    for (auto iter = keyState.begin(); iter != keyState.end(); ++iter)
    {
        iter->second = false;
    }
}

bool InputEngine::getEventBoolValue(int eventID)
{
    return (getEventValue(eventID) > 0.5f);
}

bool InputEngine::getEventBoolValueBounce(int eventID, float time)
{
    if (event_times[eventID] > 0)
        return false;
    else
    {
        bool res = getEventBoolValue(eventID);
        if (res)
            event_times[eventID] = time;
        return res;
    }
}

float InputEngine::getEventBounceTime(int eventID)
{
    return event_times[eventID];
}

void InputEngine::updateKeyBounces(float dt)
{
    for (std::map<int, float>::iterator it = event_times.begin(); it != event_times.end(); it++)
    {
        if (it->second > 0)
            it->second -= dt;
        //else
        // erase element?
    }
}

float InputEngine::deadZone(float axisValue, float dz)
{
    // no deadzone?
    if (dz < 0.0001f)
        return axisValue;

    // check for deadzone
    if (fabs(axisValue) < dz)
    // dead zone case
        return 0.0f;
    else
    // non-deadzone, remap the remaining part
        return (axisValue - dz) * (1.0f / (1.0f - dz));
}

float InputEngine::axisLinearity(float axisValue, float linearity)
{
    return (axisValue * linearity);
}

float InputEngine::logval(float val)
{
    if (val > 0)
        return log10(1.0 / (1.1 - val)) / 1.0;
    if (val == 0)
        return 0;
    return -log10(1.0 / (1.1 + val)) / 1.0;
}

String InputEngine::getEventCommand(int eventID)
{
    std::vector<event_trigger_t>& t_vec = events[eventID];
    if (t_vec.size() > 0)
    {
        // TODO: handle multiple mappings for one event code - currently we only check the first one.
        return this->composeEventCommandString(t_vec[0]);
    }
    return "";
}

std::string InputEngine::getEventCommandTrimmed(int eventID)
{
    std::string result = regex_replace(App::GetInputEngine()->getEventCommand(eventID).c_str(), std::regex("EXPL\\+"), "");
    return result;
}

String InputEngine::getTriggerCommand(event_trigger_t const& evt)
{
    return this->composeEventCommandString(evt);
}

String InputEngine::getEventConfig(int eventID)
{
    std::vector<event_trigger_t>& t_vec = events[eventID];
    if (t_vec.size() > 0)
    {
        // TODO: handle multiple mappings for one event code - currently we only check the first one.
        return this->composeEventConfigString(t_vec[0]);
    }
    return "";
}

String InputEngine::getEventDefaultConfig(int eventID)
{
    for (int i = 0; i < EV_MODE_LAST; i++)
    {
        if (eventInfo[i].eventID == eventID)
        {
            return eventInfo[i].defaultKey;
        }
    }
    return "";
}

std::string InputEngine::composeEventCommandString(event_trigger_t const& trig)
{
    switch (trig.eventtype)
    {
    case ET_Keyboard:
    case ET_JoystickButton:
        return trig.configline; // Configline contains the key/button

    case ET_JoystickPov:
        return fmt::format("{} {}", trig.joystickPovNumber, trig.configline); // Configline contains the direction - display it.

    case ET_JoystickSliderX:
        return fmt::format("X {}", trig.joystickSliderNumber); // Must show axis separately. Configline contains just the options.

    case ET_JoystickSliderY:
        return fmt::format("Y {}", trig.joystickSliderNumber); // Must show axis separately. Configline contains just the options.

    default: // Joystick axes
        return fmt::format("{}", trig.joystickAxisNumber); // Configline contains just the options.
    }
}

std::string InputEngine::composeEventConfigString(event_trigger_t const& trig)
{
    switch (trig.eventtype)
    {
    case ET_Keyboard:
    case ET_JoystickButton:
    case ET_JoystickPov:
        return this->composeEventCommandString(trig);

    // Sliders: Include redundant X/Y and options.
    case ET_JoystickSliderX:
        return fmt::format("X {} {}", trig.joystickSliderNumber, trig.configline); // Configline contains just the options.
    case ET_JoystickSliderY:
        return fmt::format("Y {} {}", trig.joystickSliderNumber, trig.configline); // Configline contains just the options.

    // Joystick axes and sliders - include options.
    default:
        return fmt::format("{} {}", trig.joystickAxisNumber, trig.configline); // Configline contains just the options.
    }
}

bool InputEngine::isEventDefined(int eventID)
{
    std::vector<event_trigger_t> t_vec = events[eventID];
    if (t_vec.size() > 0)
    {
        if (t_vec[0].eventtype != ET_NONE) // TODO: handle multiple mappings for one event code - currently we only check the first one.
            return true;
    }
    return false;
}

int InputEngine::getKeboardKeyForCommand(int eventID)
{
    std::vector<event_trigger_t> t_vec = events[eventID];
    for (std::vector<event_trigger_t>::iterator i = t_vec.begin(); i != t_vec.end(); i++)
    {
        event_trigger_t t = *i;
        if (t.eventtype == ET_Keyboard)
            return t.keyCode;
    }
    return -1;
}

bool InputEngine::isEventAnalog(int eventID)
{
    std::vector<event_trigger_t> t_vec = events[eventID];
    if (t_vec.size() > 0)
    {
        //loop through all eventtypes, because we want to find a analog device wether it is the first device or not
        //this means a analog device is always preferred over a digital one
        for (unsigned int i = 0; i < t_vec.size(); i++)
        {
            if ((t_vec[i].eventtype == ET_MouseAxisX
                    || t_vec[i].eventtype == ET_MouseAxisY
                    || t_vec[i].eventtype == ET_MouseAxisZ
                    || t_vec[i].eventtype == ET_JoystickAxisAbs
                    || t_vec[i].eventtype == ET_JoystickAxisRel
                    || t_vec[i].eventtype == ET_JoystickSliderX
                    || t_vec[i].eventtype == ET_JoystickSliderY)
                //check if value comes from analog device
                //this way, only valid events (e.g. joystick mapped, but unplugged) are recognized as analog events
                && getEventValue(eventID, true, InputSourceType::IST_ANALOG) != 0.0)
            {
                return true;
            }
        }
    }
    return false;
}

float InputEngine::getEventValue(int eventID, bool pure, InputSourceType valueSource /*= InputSourceType::IST_ANY*/)
{
    float returnValue = 0;
    std::vector<event_trigger_t> t_vec = events[eventID];
    float value = 0;
    for (std::vector<event_trigger_t>::iterator i = t_vec.begin(); i != t_vec.end(); i++)
    {
        event_trigger_t t = *i;

        if (valueSource == InputSourceType::IST_DIGITAL || valueSource == InputSourceType::IST_ANY)
        {
            switch (t.eventtype)
            {
            case ET_NONE:
                break;
            case ET_Keyboard:
                if (!keyState[t.keyCode])
                    break;

                // only use explicite mapping, if two keys with different modifiers exist, i.e. F1 and SHIFT+F1.
                // check for modificators
                if (t.explicite)
                {
                    if (t.ctrl != (keyState[SDLK_LCTRL] || keyState[SDLK_RCTRL]))
                        break;
                    if (t.shift != (keyState[SDLK_LSHIFT] || keyState[SDLK_RSHIFT]))
                        break;
                    if (t.alt != (keyState[SDLK_LALT] || keyState[SDLK_RALT]))
                        break;
                }
                else
                {
                    if (t.ctrl && !(keyState[SDLK_LCTRL] || keyState[SDLK_RCTRL]))
                        break;
                    if (t.shift && !(keyState[SDLK_LSHIFT] || keyState[SDLK_RSHIFT]))
                        break;
                    if (t.alt && !(keyState[SDLK_LALT] || keyState[SDLK_RALT]))
                        break;
                }
                value = 1;
                break;
            case ET_MouseButton:
                value = this->isMouseButtonDown(OgreBites::BUTTON_LEFT) || this->isMouseButtonDown(OgreBites::BUTTON_RIGHT);
                break;
            case ET_JoystickButton:
                {/* FIXME-SDL
                    if (t.joystickNumber > free_joysticks || !mJoy[t.joystickNumber])
                    {
                        value = 0;
                        continue;
                    }
                    if (t.joystickButtonNumber >= (int)mJoy[t.joystickNumber]->getNumberOfComponents(OIS_Button))
                    {
#ifndef NOOGRE
                        LOG("*** Joystick has not enough buttons for mapping: need button "+TOSTRING(t.joystickButtonNumber) + ", availabe buttons: "+TOSTRING(mJoy[t.joystickNumber]->getNumberOfComponents(OIS_Button)));
#endif
                        value = 0;
                        continue;
                    }
                    value = joyState[t.joystickNumber].mButtons[t.joystickButtonNumber];
                    */
                }
                break;
            case ET_JoystickPov:
                { /*FIXME-SDL
                    if (t.joystickNumber > free_joysticks || !mJoy[t.joystickNumber])
                    {
                        value = 0;
                        continue;
                    }
                    if (t.joystickPovNumber >= (int)mJoy[t.joystickNumber]->getNumberOfComponents(OIS_POV))
                    {
#ifndef NOOGRE
                        LOG("*** Joystick has not enough POVs for mapping: need POV "+TOSTRING(t.joystickPovNumber) + ", availabe POVs: "+TOSTRING(mJoy[t.joystickNumber]->getNumberOfComponents(OIS_POV)));
#endif
                        value = 0;
                        continue;
                    }
                    if (joyState[t.joystickNumber].mPOV[t.joystickPovNumber].direction & t.joystickPovDirection)
                        value = 1;
                    else
                        value = 0;
                        */
                }
                break;
            }
        }
        if (valueSource == InputSourceType::IST_ANALOG || valueSource == InputSourceType::IST_ANY)
        {
            switch (t.eventtype)
            {
                /* FIXME-SDL,  unused??
            case ET_MouseAxisX:
                value = mouseState.X.abs / 32767;
                break;
            case ET_MouseAxisY:
                value = mouseState.Y.abs / 32767;
                break;
            case ET_MouseAxisZ:
                value = mouseState.Z.abs / 32767;
                break;
                */

            case ET_JoystickAxisRel:
            case ET_JoystickAxisAbs:
                {
                    /* FIXME-SDL
                    if (t.joystickNumber > free_joysticks || !mJoy[t.joystickNumber])
                    {
                        value = 0;
                        continue;
                    }
                    if (t.joystickAxisNumber >= (int)joyState[t.joystickNumber].mAxes.size())
                    {
#ifndef NOOGRE
                        LOG("*** Joystick has not enough axis for mapping: need axe "+TOSTRING(t.joystickAxisNumber) + ", availabe axis: "+TOSTRING(joyState[t.joystickNumber].mAxes.size()));
#endif
                        value = 0;
                        continue;
                    }
                    Axis axe = joyState[t.joystickNumber].mAxes[t.joystickAxisNumber];

                    if (t.eventtype == ET_JoystickAxisRel)
                    {
                        value = (float)axe.rel / (float)mJoy[t.joystickNumber]->MAX_AXIS;
                    }
                    else
                    {
                        value = (float)axe.abs / (float)mJoy[t.joystickNumber]->MAX_AXIS;
                        switch (t.joystickAxisRegion)
                        {
                        case 0:
                            // normal case, full axis used
                            value = (value + 1) / 2;
                            break;
                        case -1:
                            // lower range used
                            if (value > 0)
                                value = 0;
                            else
                                value = -value;
                            break;
                        case 1:
                            // upper range used
                            if (value < 0)
                                value = 0;
                            break;
                        }

                        if (t.joystickAxisHalf)
                        {
                            // XXX: TODO: write this
                            //float a = (double)((value+1.0)/2.0);
                            //float b = (double)(1.0-(value+1.0)/2.0);
                            //LOG("half: "+TOSTRING(value)+" / "+TOSTRING(a)+" / "+TOSTRING(b));
                            //no dead zone in half axis
                            value = (1.0 + value) / 2.0;
                            if (t.joystickAxisReverse)
                                value = 1.0 - value;
                            if (!pure)
                                value = axisLinearity(value, t.joystickAxisLinearity);
                        }
                        else
                        {
                            //LOG("not half: "+TOSTRING(value)+" / "+TOSTRING(deadZone(value, t.joystickAxisDeadzone)) +" / "+TOSTRING(t.joystickAxisDeadzone) );
                            if (t.joystickAxisReverse)
                                value = 1 - value;
                            if (!pure)
                            // no deadzone when using oure value
                                value = deadZone(value, t.joystickAxisDeadzone);
                            if (!pure)
                                value = axisLinearity(value, t.joystickAxisLinearity);
                        }
                        // digital mapping of analog axis
                        if (t.joystickAxisUseDigital)
                            if (value >= 0.5)
                                value = 1;
                            else
                                value = 0;
                    }
                    */
                }
                break;
            case ET_JoystickSliderX:
            case ET_JoystickSliderY:
                {
                    /* FIXME-SDL
                    if (t.joystickNumber > free_joysticks || !mJoy[t.joystickNumber])
                    {
                        value = 0;
                        continue;
                    }
                    if (t.eventtype == ET_JoystickSliderX)
                        value = (float)joyState[t.joystickNumber].mSliders[t.joystickSliderNumber].abX / (float)mJoy[t.joystickNumber]->MAX_AXIS;
                    else if (t.eventtype == ET_JoystickSliderY)
                        value = (float)joyState[t.joystickNumber].mSliders[t.joystickSliderNumber].abY / (float)mJoy[t.joystickNumber]->MAX_AXIS;
                    value = (value + 1) / 2; // full axis
                    if (t.joystickSliderReverse)
                        value = 1.0 - value; // reversed
                    */
                }
                break;
            }
        }
        // only return if grater zero, otherwise check all other bombinations
        if (value > returnValue)
            returnValue = value;
    }
    return returnValue;
}

bool InputEngine::isKeyDown(OgreBites::Keycode mod)
{
    return (SDL_GetModState() & mod) != 0;
}

bool InputEngine::isMouseButtonDown(OgreBites::ButtonType btn)
{
    return (SDL_GetMouseState(nullptr, nullptr) & btn) != 0;
}

bool InputEngine::isKeyDownEffective(SDL_Keycode mod)
{
    return this->keyState[mod];
}

bool InputEngine::isKeyDownValueBounce(SDL_Keycode mod, float time)
{
    if (event_times[-mod] > 0)
        return false;
    else
    {
        bool res = isKeyDown(mod);
        if (res)
            event_times[-mod] = time;
        return res;
    }
}

String InputEngine::getDeviceName(event_trigger_t const& evt)
{
    switch (evt.eventtype)
    {
    case ET_NONE:
        return "None";
    case ET_Keyboard:
        return "Keyboard";
    case ET_MouseButton:
    case ET_MouseAxisX:
    case ET_MouseAxisY:
    case ET_MouseAxisZ:
        return "Mouse";
    case ET_JoystickButton:
    case ET_JoystickAxisAbs:
    case ET_JoystickAxisRel:
    case ET_JoystickPov:
    case ET_JoystickSliderX:
    case ET_JoystickSliderY:
        return "Joystick: " + getJoyVendor(evt.joystickNumber);
    }
    return "unknown";
}

// static
const char* InputEngine::getEventTypeName(eventtypes type)
{
    switch (type)
    {
    case ET_NONE: return "None";
    case ET_Keyboard: return "Keyboard";
    case ET_MouseButton: return "MouseButton";
    case ET_MouseAxisX: return "MouseAxisX";
    case ET_MouseAxisY: return "MouseAxisY";
    case ET_MouseAxisZ: return "MouseAxisZ";
    case ET_JoystickButton: return "JoystickButton";
    case ET_JoystickAxisAbs:
    case ET_JoystickAxisRel: return "JoystickAxis"; // Default is 'abs', 'rel' is created by option RELATIVE.
    case ET_JoystickPov: return "JoystickPov";
    case ET_JoystickSliderX:
    case ET_JoystickSliderY: return "JoystickSlider"; // Axis is given by special parameter.
    default: return "unknown";
    }
}

void InputEngine::addEvent(int eventID, event_trigger_t& t)
{
    uniqueCounter++;

    if (eventID == -1)
    //unknown event, discard
        return;
    if (events.find(eventID) == events.end())
    {
        events[eventID] = std::vector<event_trigger_t>();
        events[eventID].clear();
    }
    events[eventID].push_back(t);
}

void InputEngine::addEventDefault(int eventID, int deviceID /*= -1*/)
{
    if (eventID == -1)
    //unknown event, discard
        return;

    std::string line = fmt::format("{} {}", this->eventIDToName(eventID), this->getEventDefaultConfig(eventID));
    this->processLine(line.c_str(), deviceID);
}

void InputEngine::updateEvent(int eventID, const event_trigger_t& t)
{
    if (eventID == -1)
    //unknown event, discard
        return;
    if (events.find(eventID) == events.end())
    {
        events[eventID] = std::vector<event_trigger_t>();
        events[eventID].clear();
    }
    events[eventID].push_back(t);
}

void InputEngine::eraseEvent(int eventID, const event_trigger_t* t)
{
    if (events.find(eventID) != events.end())
    {
        TriggerVec& triggers = events[eventID];
        for (size_t i = 0; i < triggers.size(); i++)
        {
            if (t == &triggers[i])
            {
                triggers.erase(triggers.begin() + i);
                return;
            }
        }
    }
}

void InputEngine::clearEvents(int eventID)
{
    if (events.find(eventID) != events.end())
    {
        events[eventID].clear();
    }
}

void InputEngine::clearEventsByDevice(int deviceID)
{
    for (auto& ev_pair: events)
    {
        if (ev_pair.second.size() > 0)
        {
            auto itor = ev_pair.second.begin();
            while (itor != ev_pair.second.end())
            {
                if (itor->configDeviceID == deviceID)
                    itor = ev_pair.second.erase(itor);
                else
                    itor++;
            }
        }
    }
}

void InputEngine::clearAllEvents()
{
    events.clear(); // remove all bindings
    this->resetKeys(); // reset input states
}

bool InputEngine::processLine(const char* line, int deviceID)
{
    static String cur_comment = "";

    char eventName[256] = "", evtype[256] = "";
    const char delimiters[] = "+";
    size_t linelen = strnlen(line, 1024);
    enum eventtypes eventtype = ET_NONE;

    int joyNo = 0;
    float defaultDeadzone = 0.1f;
    float defaultLinearity = 1.0f;
    if (line[0] == ';' || linelen < 5)
    {
        cur_comment += line;;
        return false;
    }
    sscanf(line, "%s %s", eventName, evtype);
    if (strnlen(eventName, 255) == 0 || strnlen(evtype, 255) == 0)
        return false;

    if (!strncmp(evtype, "Keyboard", 8))
        eventtype = ET_Keyboard;
    else if (!strncmp(evtype, "MouseButton", 10))
        eventtype = ET_MouseButton;
    else if (!strncmp(evtype, "MouseAxisX", 9))
        eventtype = ET_MouseAxisX;
    else if (!strncmp(evtype, "MouseAxisY", 9))
        eventtype = ET_MouseAxisY;
    else if (!strncmp(evtype, "MouseAxisZ", 9))
        eventtype = ET_MouseAxisZ;
    else if (!strncmp(evtype, "JoystickButton", 14))
        eventtype = ET_JoystickButton;
    else if (!strncmp(evtype, "JoystickAxis", 12))
        eventtype = ET_JoystickAxisAbs;
    //else if (!strncmp(evtype, "JoystickAxis", 250)) eventtype = ET_JoystickAxisRel;
    else if (!strncmp(evtype, "JoystickPov", 11))
        eventtype = ET_JoystickPov;
    else if (!strncmp(evtype, "JoystickSlider", 14))
        eventtype = ET_JoystickSliderX;
    else if (!strncmp(evtype, "JoystickSliderX", 15))
        eventtype = ET_JoystickSliderX;
    else if (!strncmp(evtype, "JoystickSliderY", 15))
        eventtype = ET_JoystickSliderY;
    else if (!strncmp(evtype, "None", 4))
        eventtype = ET_NONE;

    switch (eventtype)
    {
    case ET_Keyboard:
        {
            bool alt = false;
            bool shift = false;
            bool ctrl = false;
            bool expl = false;

            char* keycode = 0;
            char keycodes[256] = {};
            char keycodes_work[256] = {};

            SDL_Keycode key = SDLK_UNKNOWN;

            sscanf(line, "%s %s %s", eventName, evtype, keycodes);
            // separate all keys and construct the key combination
            //LOG("try to add key: " + String(eventName)+","+ String(evtype)+","+String(keycodes));
            std::strncpy(keycodes_work, keycodes, 255);
            keycodes_work[255] = '\0';
            char* token = strtok(keycodes_work, delimiters);

            while (token != NULL)
            {
                if (strncmp(token, "SHIFT", 5) == 0)
                    shift = true;
                else if (strncmp(token, "CTRL", 4) == 0)
                    ctrl = true;
                else if (strncmp(token, "ALT", 3) == 0)
                    alt = true;
                else if (strncmp(token, "EXPL", 4) == 0)
                    expl = true;
                keycode = token;
                token = strtok(NULL, delimiters);
            }

            allit = allkeys.find(keycode);
            if (allit == allkeys.end())
            {
#ifndef NOOGRE
                LOG("unknown key: " + string(keycodes));
#endif
                key = SDLK_UNKNOWN;
            }
            else
            {
                //LOG("found key: " + string(keycode) + " = " + TOSTRING((int)key));
                key = allit->second;
            }
            int eventID = resolveEventName(String(eventName));
            if (eventID == -1)
            {
                LOG("Error while processing input config: Unknown Event: "+String(eventName));
                return false;
            }
            event_trigger_t t_key = newEvent();
            t_key.configDeviceID = deviceID;
            t_key.eventtype = ET_Keyboard;
            t_key.shift = shift;
            t_key.ctrl = ctrl;
            t_key.alt = alt;
            t_key.keyCode = key;
            t_key.explicite = expl;

            std::strncpy(t_key.configline, keycodes, 128);
            std::strncpy(t_key.group, getEventGroup(eventName).c_str(), 128);
            std::strncpy(t_key.tmp_eventname, eventName, 128);

            std::strncpy(t_key.comments, cur_comment.c_str(), 1024);
            addEvent(eventID, t_key);

            return true;
        }
    case ET_JoystickButton:
        {
            int buttonNo = 0;
            char tmp2[256] = {};
            sscanf(line, "%s %s %d %d %s", eventName, evtype, &joyNo, &buttonNo, tmp2);
            event_trigger_t t_joy = newEvent();
            t_joy.configDeviceID = deviceID;
            int eventID = resolveEventName(String(eventName));
            if (eventID == -1)
                return false;
            t_joy.eventtype = ET_JoystickButton;
            t_joy.joystickNumber = (deviceID == -1 ? joyNo : deviceID);
            t_joy.joystickButtonNumber = buttonNo;
            if (!strcmp(tmp2, "!NEW!"))
            {
                std::strncpy(t_joy.configline, tmp2, 128);
            }
            else
            {
                char tmp[256] = {};
                sprintf(tmp, "%d", buttonNo);
                std::strncpy(t_joy.configline, tmp, 128);
            }
            std::strncpy(t_joy.group, getEventGroup(eventName).c_str(), 128);
            std::strncpy(t_joy.tmp_eventname, eventName, 128);
            std::strncpy(t_joy.comments, cur_comment.c_str(), 1024);
            cur_comment = "";
            addEvent(eventID, t_joy);
            return true;
        }
    case ET_JoystickAxisRel:
    case ET_JoystickAxisAbs:
        {
            int axisNo = 0;
            char options[256] = {};
            sscanf(line, "%s %s %d %d %s", eventName, evtype, &joyNo, &axisNo, options);
            int eventID = resolveEventName(String(eventName));
            if (eventID == -1)
                return false;

            bool half = false;
            bool reverse = false;
            bool linear = false;
            bool relative = false;
            bool usedigital = false;
            float deadzone = defaultDeadzone;
            float linearity = defaultLinearity;
            int jAxisRegion = 0;
            //  0 = all
            // -1 = lower
            //  1 = upper
            char tmp[256] = {};
            strcpy(tmp, options);
            tmp[255] = '\0';
            char* token = strtok(tmp, delimiters);
            while (token != NULL)
            {
                if (strncmp(token, "HALF", 4) == 0)
                    half = true;
                else if (strncmp(token, "REVERSE", 7) == 0)
                    reverse = true;
                else if (strncmp(token, "LINEAR", 6) == 0)
                    linear = true;
                else if (strncmp(token, "UPPER", 5) == 0)
                    jAxisRegion = 1;
                else if (strncmp(token, "LOWER", 5) == 0)
                    jAxisRegion = -1;
                else if (strncmp(token, "RELATIVE", 8) == 0)
                    relative = true;
                else if (strncmp(token, "DIGITAL", 7) == 0)
                    usedigital = true;
                else if (strncmp(token, "DEADZONE", 8) == 0 && strnlen(token, 250) > 9)
                {
                    char tmp2[256] = {};
                    strcpy(tmp2, token + 9);
                    deadzone = atof(tmp2);
                    //LOG("got deadzone: " + TOSTRING(deadzone)+", "+String(tmp2));
                }
                else if (strncmp(token, "LINEARITY", 9) == 0 && strnlen(token, 250) > 10)
                {
                    char tmp2[256] = {};
                    strcpy(tmp2, token + 10);
                    linearity = atof(tmp2);
                }
                token = strtok(NULL, delimiters);
            }

            if (relative)
                eventtype = ET_JoystickAxisRel;

            event_trigger_t t_joy = newEvent();
            t_joy.configDeviceID = deviceID;
            t_joy.eventtype = eventtype;
            t_joy.joystickAxisRegion = jAxisRegion;
            t_joy.joystickAxisUseDigital = usedigital;
            t_joy.joystickAxisDeadzone = deadzone;
            t_joy.joystickAxisHalf = half;
            t_joy.joystickAxisLinearity = linearity;
            t_joy.joystickAxisReverse = reverse;
            t_joy.joystickAxisNumber = axisNo;
            t_joy.joystickNumber = (deviceID == -1 ? joyNo : deviceID);
            std::strncpy(t_joy.configline, options, 128);
            std::strncpy(t_joy.group, getEventGroup(eventName).c_str(), 128);
            std::strncpy(t_joy.tmp_eventname, eventName, 128);
            std::strncpy(t_joy.comments, cur_comment.c_str(), 1024);
            cur_comment = "";
            addEvent(eventID, t_joy);
            //LOG("added axis: " + TOSTRING(axisNo));
            return true;
        }
    case ET_NONE:
        {
            int eventID = resolveEventName(String(eventName));
            if (eventID == -1)
                return false;
            // Insert event with no trigger
            events.insert(std::make_pair(eventID, std::vector<event_trigger_t>()));
            return true;
        }
    case ET_MouseButton:
    case ET_MouseAxisX:
    case ET_MouseAxisY:
    case ET_MouseAxisZ:
        // no mouse support D:
        return false;
    case ET_JoystickPov:
        {
            int povNumber = 0;
            char dir[256] = {};
            sscanf(line, "%s %s %d %d %s", eventName, evtype, &joyNo, &povNumber, dir);
            int eventID = resolveEventName(String(eventName));
            if (eventID == -1)
                return false;

            /* FIXME-SDL
            int direction = OIS::Pov::Centered;
            if (!strcmp(dir, "North"))
                direction = OIS::Pov::North;
            if (!strcmp(dir, "South"))
                direction = OIS::Pov::South;
            if (!strcmp(dir, "East"))
                direction = OIS::Pov::East;
            if (!strcmp(dir, "West"))
                direction = OIS::Pov::West;
            if (!strcmp(dir, "NorthEast"))
                direction = OIS::Pov::NorthEast;
            if (!strcmp(dir, "SouthEast"))
                direction = OIS::Pov::SouthEast;
            if (!strcmp(dir, "NorthWest"))
                direction = OIS::Pov::NorthWest;
            if (!strcmp(dir, "SouthWest"))
                direction = OIS::Pov::SouthWest;

            event_trigger_t t_pov = newEvent();
            t_pov.configDeviceID = deviceID;
            t_pov.eventtype = eventtype;
            t_pov.joystickNumber = (deviceID == -1 ? joyNo : deviceID);
            t_pov.joystickPovNumber = povNumber;
            t_pov.joystickPovDirection = direction;

            std::strncpy(t_pov.group, getEventGroup(eventName).c_str(), 128);
            std::strncpy(t_pov.tmp_eventname, eventName, 128);
            std::strncpy(t_pov.comments, cur_comment.c_str(), 1024);
            std::strncpy(t_pov.configline, dir, 128);
            cur_comment = "";
            addEvent(eventID, t_pov);
            //LOG("added axis: " + TOSTRING(axisNo));
            */
            return true;
        }
    case ET_JoystickSliderX:
    case ET_JoystickSliderY:
        {
            int sliderNumber = 0;
            char options[256] = {};
            char type;
            sscanf(line, "%s %s %d %c %d %s", eventName, evtype, &joyNo, &type, &sliderNumber, options);
            int eventID = resolveEventName(String(eventName));
            if (eventID == -1)
                return false;

            bool reverse = false;
            char tmp[256] = {};
            std::strncpy(tmp, options, 255);
            tmp[255] = '\0';
            char* token = strtok(tmp, delimiters);
            while (token != NULL)
            {
                if (strncmp(token, "REVERSE", 7) == 0)
                    reverse = true;

                token = strtok(NULL, delimiters);
            }

            event_trigger_t t_slider = newEvent();
            t_slider.configDeviceID = deviceID;

            if (type == 'Y' || type == 'y')
                eventtype = ET_JoystickSliderY;
            else if (type == 'X' || type == 'x')
                eventtype = ET_JoystickSliderX;

            t_slider.eventtype = eventtype;
            t_slider.joystickNumber = (deviceID == -1 ? joyNo : deviceID);
            t_slider.joystickSliderNumber = sliderNumber;
            t_slider.joystickSliderReverse = reverse;
            // TODO: add region support to sliders!
            t_slider.joystickSliderRegion = 0;
            std::strncpy(t_slider.configline, options, 128);
            std::strncpy(t_slider.group, getEventGroup(eventName).c_str(), 128);
            std::strncpy(t_slider.tmp_eventname, eventName, 128);
            std::strncpy(t_slider.comments, cur_comment.c_str(), 1024);
            cur_comment = "";
            addEvent(eventID, t_slider);
            //LOG("added axis: " + TOSTRING(axisNo));
            return true;
        }
    default:
        return false;
    }
}

int InputEngine::getCurrentJoyButton(int& joystickNumber, int& button)
{
    /* FIXME-SDL
    for (int j = 0; j < free_joysticks; j++)
    {
        for (int i = 0; i < (int)joyState[j].mButtons.size(); i++)
        {
            if (joyState[j].mButtons[i])
            {
                joystickNumber = j;
                button = i;
                return 1;
            }
        }
    }*/
    return 0;
}

int InputEngine::getCurrentPovValue(int& joystickNumber, int& pov, int& povdir)
{
    /* FIXME-SDL
    for (int j = 0; j < free_joysticks; j++)
    {
        for (int i = 0; i < MAX_JOYSTICK_POVS; i++)
        {
            if (joyState[j].mPOV[i].direction != Pov::Centered)
            {
                joystickNumber = j;
                pov = i;
                povdir = joyState[j].mPOV[i].direction;
                return 1;
            }
        }
    }*/
    return 0;
}

Ogre::Vector2 InputEngine::getMouseNormalizedScreenPos()
{
    int x = 0, y = 0;
    SDL_GetMouseState(&x, &y);
    return Ogre::Vector2(
        static_cast<float>(x) / ImGui::GetIO().DisplaySize.x,
        static_cast<float>(y) / ImGui::GetIO().DisplaySize.y);
}

event_trigger_t InputEngine::newEvent()
{
    event_trigger_t res;
    memset(&res, 0, sizeof(event_trigger_t));
    return res;
}

std::string InputEngine::getJoyVendor(int joystickNumber)
{
    // FIXME
        return "unknown";

}

int InputEngine::getCurrentKeyCombo(String* combo)
{
    int keyCounter = 0;
    int modCounter = 0;

    // list all modificators first
    for (auto i = keyState.begin(); i != keyState.end(); i++)
    {
        if (i->second)
        {
            if (i->first != SDLK_LSHIFT && i->first != SDLK_RSHIFT && i->first != SDLK_LCTRL && i->first != SDLK_RCTRL && i->first != SDLK_LALT && i->first != SDLK_RALT)
                continue;
            modCounter++;
            String keyName = getKeyNameForKeyCode(i->first);
            if (*combo == "")
                *combo = keyName;
            else
                *combo = *combo + "+" + keyName;
        }
    }

    // now list all keys
    for (auto i = keyState.begin(); i != keyState.end(); i++)
    {
        if (i->second)
        {
            if (i->first == SDLK_LSHIFT || i->first == SDLK_RSHIFT || i->first == SDLK_LCTRL || i->first == SDLK_RCTRL || i->first == SDLK_LALT || i->first == SDLK_RALT)
                continue;
            String keyName = getKeyNameForKeyCode(i->first);
            if (*combo == "")
                *combo = keyName;
            else
                *combo = *combo + "+" + keyName;
            keyCounter++;
        }
    }

    //
    if (modCounter > 0 && keyCounter == 0)
    {
        return -modCounter;
    }
    else if (keyCounter == 0 && modCounter == 0)
    {
        *combo = _L("(Please press a key)");
        return 0;
    }
    return keyCounter;
}

String InputEngine::getEventGroup(String eventName)
{
    const char delimiters[] = "_";
    char tmp[256] = {};
    std::strncpy(tmp, eventName.c_str(), 255);
    tmp[255] = '\0';
    char* token = strtok(tmp, delimiters);
    while (token != NULL)
    {
        return String(token);
    }
    return "";
}

bool InputEngine::updateConfigline(event_trigger_t* t)
{
    if (t->eventtype != ET_JoystickAxisAbs && t->eventtype != ET_JoystickSliderX && t->eventtype != ET_JoystickSliderY)
        return false;
    bool isSlider = (t->eventtype == ET_JoystickSliderX || t->eventtype == ET_JoystickSliderY);
    strcpy(t->configline, "");

    if (isSlider)
    {
        if (t->joystickSliderReverse && !strlen(t->configline))
            strcat(t->configline, "REVERSE");
        else if (t->joystickSliderReverse && strlen(t->configline))
            strcat(t->configline, "+REVERSE");

        if (t->joystickSliderRegion == 1 && !strlen(t->configline))
            strcat(t->configline, "UPPER");
        else if (t->joystickSliderRegion == 1 && strlen(t->configline))
            strcat(t->configline, "+UPPER");
        else if (t->joystickSliderRegion == -1 && !strlen(t->configline))
            strcat(t->configline, "LOWER");
        else if (t->joystickSliderRegion == -1 && strlen(t->configline))
            strcat(t->configline, "+LOWER");

        // is this is a slider, ignore the rest
        return true;
    }

    if (t->joystickAxisReverse && !strlen(t->configline))
        strcat(t->configline, "REVERSE");
    else if (t->joystickAxisReverse && strlen(t->configline))
        strcat(t->configline, "+REVERSE");

    if (t->joystickAxisRegion == 1 && !strlen(t->configline))
        strcat(t->configline, "UPPER");
    else if (t->joystickAxisRegion == 1 && strlen(t->configline))
        strcat(t->configline, "+UPPER");
    else if (t->joystickAxisRegion == -1 && !strlen(t->configline))
        strcat(t->configline, "LOWER");
    else if (t->joystickAxisRegion == -1 && strlen(t->configline))
        strcat(t->configline, "+LOWER");

    if (fabs(t->joystickAxisDeadzone - 0.1) > 0.0001f)
    {
        char tmp[256] = {};
        sprintf(tmp, "DEADZONE=%0.2f", t->joystickAxisDeadzone);
        if (strlen(t->configline))
        {
            strcat(t->configline, "+");
            strcat(t->configline, tmp);
        }
        else
            strcat(t->configline, tmp);
    }
    if (fabs(1.0f - t->joystickAxisLinearity) > 0.01f)
    {
        char tmp[256] = {};
        sprintf(tmp, "LINEARITY=%0.2f", t->joystickAxisLinearity);
        if (strlen(t->configline))
        {
            strcat(t->configline, "+");
            strcat(t->configline, tmp);
        }
        else
            strcat(t->configline, tmp);
    }
    return true;
}

void InputEngine::completeMissingEvents()
{
    for (int i = 0; i < EV_MODE_LAST; i++)
    {
        if (events.find(eventInfo[i].eventID) == events.end())
        {
            // not existing, insert default

            char tmp[256] = "";
            sprintf(tmp, "%s %s", eventInfo[i].name.c_str(), eventInfo[i].defaultKey.c_str());

            processLine(tmp, BUILTIN_MAPPING_DEVICEID);
        }
    }
}

// Loads config file specific to a device and OS (or default config if deviceID is -1).
bool InputEngine::loadConfigFile(int deviceID)
{
    String fileName;
    if (deviceID == -1)
    {
        fileName = DEFAULT_MAPFILE;
    }
    else
    {
        ROR_ASSERT(deviceID < free_joysticks);

        String deviceStr; // FIXME-SDL = mJoy[deviceID]->vendor();

        // care about unsuitable chars
        String repl = "\\/ #@?!$%^&*()+=-><.:'|\";";
        for (unsigned int c = 0; c < repl.size(); c++)
        {
            deviceStr = StringUtil::replaceAll(deviceStr, repl.substr(c, 1), "_");
        }

        // Look for OS-specific device mapping
        String osSpecificMapFile;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
        osSpecificMapFile = deviceStr + ".windows.map";
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
        osSpecificMapFile = deviceStr + ".linux.map";
#endif
        if (osSpecificMapFile != "" &&
            ResourceGroupManager::getSingleton().resourceExists(RGN_CONFIG, osSpecificMapFile))
        {
            fileName = osSpecificMapFile;
        }
        else
        {
            // Load generic device mapping
            fileName = deviceStr + ".map";
        }

        m_loaded_configs[deviceID] = fileName;
    }

    return this->loadMapping(fileName, deviceID);
}

bool InputEngine::saveConfigFile(int deviceID)
{
    ROR_ASSERT(deviceID < free_joysticks);

    if (deviceID == -1)
        return this->saveMapping(DEFAULT_MAPFILE, deviceID);
    else
        return this->saveMapping(m_loaded_configs[deviceID], deviceID);
}

std::string const& InputEngine::getLoadedConfigFile(int deviceID /*= -1*/)
{
    ROR_ASSERT(deviceID < free_joysticks);
    if (deviceID == -1)
        return DEFAULT_MAPFILE;
    else
        return m_loaded_configs[deviceID];
}

bool InputEngine::loadMapping(String fileName, int deviceID)
{
    char line[1025] = "";
    int oldState = uniqueCounter;

    LOG(" * Loading input mapping " + fileName);
    {
        DataStreamPtr ds;
        try
        {
            ds = ResourceGroupManager::getSingleton().openResource(fileName, RGN_CONFIG);
        }
        catch (...)
        {
            return false;
        }
        while (!ds->eof())
        {
            size_t size = 1024;
            if (ds->tell() + size >= ds->size())
                size = ds->size() - ds->tell();
            if (ds->tell() >= ds->size())
                break;
            size_t readnum = ds->readLine(line, size);
            if (readnum > 5)
                processLine(line, deviceID);
        }
    }

    int newEvents = uniqueCounter - oldState;
    LOG(" * Input map successfully loaded: " + TOSTRING(newEvents) + " entries");
    return true;
}

bool InputEngine::saveMapping(String fileName, int deviceID)
{
    const int COL1_WIDTH = 34; // total 35
    const int COL2_WIDTH = 19; // total 20

    try
    {
        // Open file for writing (overwrite existing).
        Ogre::DataStreamPtr ds = Ogre::ResourceGroupManager::getSingleton().createResource(
            fileName, RGN_CONFIG, /*overwrite:*/true);

        // Loop events and filter by device.
        for (auto& ev_pair: events)
        {
            for (event_trigger_t& ev: ev_pair.second)
            {
                if (ev.configDeviceID == deviceID)
                {
                    // We found a matching event - compose config line and write it.

                    string conf_string = this->composeEventConfigString(ev);
                    switch (ev.eventtype)
                    {
                    case ET_JoystickButton:
                    case ET_JoystickAxisAbs:
                    case ET_JoystickAxisRel:
                    case ET_JoystickPov:
                    case ET_JoystickSliderX:
                    case ET_JoystickSliderY:
                        conf_string = fmt::format("0 {}", conf_string); // Dummy device number (unused)
                        break;
                    default:;
                    }

                    std::string line = fmt::format("{:<{}} {:<{}} {}\n", // padding: ("{:<{}}", "abc", 10) writes "abc       "
                        this->eventIDToName(ev_pair.first), COL1_WIDTH,
                        this->getEventTypeName(ev.eventtype), COL2_WIDTH,
                        conf_string);
                    ds->write(line.c_str(), line.size());
                }
            }
        }
        return true; // Closes `ds`
    }
    catch (Ogre::Exception& e) // Already logged by OGRE
    {
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR,
            fmt::format("Failed to write '{}', {}", fileName, e.getDescription()));
        return false;
    }
}

int InputEngine::resolveEventName(String eventName)
{
    int i = 0;
    while (i != EV_MODE_LAST)
    {
        if (eventInfo[i].name == eventName)
            return eventInfo[i].eventID;
        i++;
    }
    LOG("unknown event (ignored): " + eventName);
    return -1;
}

String InputEngine::eventIDToName(int eventID)
{
    int i = 0;
    while (i != EV_MODE_LAST)
    {
        if (eventInfo[i].eventID == eventID)
            return eventInfo[i].name;
        i++;
    }
    return "unknown";
}

String InputEngine::eventIDToDescription(int eventID)
{
    int i = 0;
    while (i != EV_MODE_LAST)
    {
        if (eventInfo[i].eventID == eventID)
            return eventInfo[i].description;
        i++;
    }
    return "unknown";
}

void InputEngine::initAllKeys()
{
    // see SDL_keycode.h

    allkeys["0"] = SDLK_0;
    allkeys["1"] = SDLK_1;
    allkeys["2"] = SDLK_2;
    allkeys["3"] = SDLK_3;
    allkeys["4"] = SDLK_4;
    allkeys["5"] = SDLK_5;
    allkeys["6"] = SDLK_6;
    allkeys["7"] = SDLK_7;
    allkeys["8"] = SDLK_8;
    allkeys["9"] = SDLK_9;
    allkeys["A"] = SDLK_a;
    allkeys["ABNT_C1"] = SDLK_APP1; // ??
    allkeys["ABNT_C2"] = SDLK_APP2; // ??
    allkeys["ADD"] = SDLK_PLUS;
    allkeys["APOSTROPHE"] = SDLK_QUOTE;
    allkeys["APPS"] = SDLK_APPLICATION;
    allkeys["AT"] = SDLK_AT;
    allkeys["AX"] = SDLK_HASH; // ??
    allkeys["B"] = SDLK_b;
    allkeys["BACK"] = SDLK_BACKSPACE;
    allkeys["BACKSLASH"] = SDLK_BACKSLASH;
    allkeys["C"] = SDLK_c;
    allkeys["CALCULATOR"] = SDLK_CALCULATOR;
    //allkeys["CAPITAL"] = KC_CAPITAL;
    allkeys["COLON"] = SDLK_COLON;
    allkeys["COMMA"] = SDLK_COMMA;
    //allkeys["CONVERT"] = KC_CONVERT;
    allkeys["D"] = SDLK_d;
    //allkeys["DECIMAL"] = KC_DECIMAL;
    allkeys["DELETE"] = SDLK_DELETE;
    allkeys["DIVIDE"] = SDLK_SLASH;
    allkeys["DOWN"] = SDLK_DOWN;
    allkeys["E"] = SDLK_e;
    allkeys["END"] = SDLK_END;
    allkeys["EQUALS"] = SDLK_EQUALS;
    allkeys["ESCAPE"] = SDLK_ESCAPE;
    allkeys["F"] = SDLK_f;
    allkeys["F1"] = SDLK_F1;
    allkeys["F10"] = SDLK_F10;
    allkeys["F11"] = SDLK_F11;
    allkeys["F12"] = SDLK_F12;
    allkeys["F13"] = SDLK_F13;
    allkeys["F14"] = SDLK_F14;
    allkeys["F15"] = SDLK_F15;
    allkeys["F2"] = SDLK_F2;
    allkeys["F3"] = SDLK_F3;
    allkeys["F4"] = SDLK_F4;
    allkeys["F5"] = SDLK_F5;
    allkeys["F6"] = SDLK_F6;
    allkeys["F7"] = SDLK_F7;
    allkeys["F8"] = SDLK_F8;
    allkeys["F9"] = SDLK_F9;
    allkeys["G"] = SDLK_g;
    allkeys["GRAVE"] = SDLK_BACKQUOTE;
    allkeys["H"] = SDLK_h;
    allkeys["HOME"] = SDLK_HOME;
    allkeys["I"] = SDLK_i;
    allkeys["INSERT"] = SDLK_INSERT;
    allkeys["J"] = SDLK_j;
    allkeys["K"] = SDLK_k;
    //allkeys["KANA"]  - no SDL KeyCode, should we use scancodes? -  SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LANG3); // See SDL_scancode.h
    //allkeys["KANJI"]  - no SDL KeyCode, should we use scancodes? -  SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LANG2); // ?? - See SDL_scancode.h
    allkeys["L"] = SDLK_l;
    allkeys["LBRACKET"] = SDLK_LEFTBRACKET;
    allkeys["LCONTROL"] = SDLK_LCTRL;
    allkeys["LEFT"] = SDLK_LEFT;
    allkeys["LMENU"] = SDLK_LALT;
    allkeys["LSHIFT"] = SDLK_LSHIFT;
    allkeys["LWIN"] = SDLK_LGUI; // ??
    allkeys["M"] = SDLK_m;
    allkeys["MAIL"] = SDLK_MAIL;
    allkeys["MEDIASELECT"] = SDLK_MEDIASELECT;
    allkeys["MEDIASTOP"] = SDLK_AUDIOSTOP;
    allkeys["MINUS"] = SDLK_MINUS;
    allkeys["MULTIPLY"] = SDLK_ASTERISK;
    allkeys["MUTE"] = SDLK_MUTE;
    allkeys["MYCOMPUTER"] = SDLK_COMPUTER;
    allkeys["N"] = SDLK_n;
    allkeys["NEXTTRACK"] = SDLK_AUDIONEXT;
    //allkeys["NOCONVERT"] = KC_NOCONVERT;
    allkeys["NUMLOCK"] = SDLK_NUMLOCKCLEAR;
    allkeys["NUMPAD0"] = SDLK_KP_0;
    allkeys["NUMPAD1"] = SDLK_KP_1;
    allkeys["NUMPAD2"] = SDLK_KP_2;
    allkeys["NUMPAD3"] = SDLK_KP_3;
    allkeys["NUMPAD4"] = SDLK_KP_4;
    allkeys["NUMPAD5"] = SDLK_KP_5;
    allkeys["NUMPAD6"] = SDLK_KP_6;
    allkeys["NUMPAD7"] = SDLK_KP_7;
    allkeys["NUMPAD8"] = SDLK_KP_8;
    allkeys["NUMPAD9"] = SDLK_KP_9;
    allkeys["NUMPADCOMMA"] = SDLK_KP_PERIOD;
    allkeys["NUMPADENTER"] = SDLK_KP_ENTER;
    allkeys["NUMPADEQUALS"] = SDLK_KP_EQUALS;
    allkeys["O"] = SDLK_o;
    //allkeys["OEM_102"] = KC_OEM_102;
    allkeys["P"] = SDLK_p;
    allkeys["PAUSE"] = SDLK_PAUSE;
    allkeys["PERIOD"] = SDLK_PERIOD;
    allkeys["PGDOWN"] = SDLK_PAGEDOWN;
    allkeys["PGUP"] = SDLK_PAGEUP;
    allkeys["PLAYPAUSE"] = SDLK_PAUSE;
    allkeys["POWER"] = SDLK_POWER;
    allkeys["PREVTRACK"] = SDLK_AUDIOPREV;
    allkeys["Q"] = SDLK_q;
    allkeys["R"] = SDLK_r;
    allkeys["RBRACKET"] = SDLK_RIGHTBRACKET;
    allkeys["RCONTROL"] = SDLK_RCTRL;
    allkeys["RETURN"] = SDLK_RETURN;
    allkeys["RIGHT"] = SDLK_RIGHT;
    allkeys["RMENU"] = SDLK_RALT;
    allkeys["RSHIFT"] = SDLK_RSHIFT;
    allkeys["RWIN"] = SDLK_RGUI; // ??
    allkeys["S"] = SDLK_s;
    allkeys["SCROLL"] = SDLK_SCROLLLOCK;
    allkeys["SEMICOLON"] = SDLK_SEMICOLON;
    allkeys["SLASH"] = SDLK_SLASH;
    allkeys["SLEEP"] = SDLK_SLEEP;
    allkeys["SPACE"] = SDLK_SPACE;
    allkeys["STOP"] = SDLK_STOP;
    allkeys["SUBTRACT"] = SDLK_MINUS;
    allkeys["SYSRQ"] = SDLK_PRINTSCREEN;
    allkeys["T"] = SDLK_t;
    allkeys["TAB"] = SDLK_TAB;
    allkeys["U"] = SDLK_u;
    allkeys["UNDERLINE"] = SDLK_UNDERSCORE;
    //allkeys["UNLABELED"] = KC_UNLABELED;
    allkeys["UP"] = SDLK_UP;
    allkeys["V"] = SDLK_v;
    allkeys["VOLUMEDOWN"] = SDLK_VOLUMEDOWN;
    allkeys["VOLUMEUP"] = SDLK_VOLUMEUP;
    allkeys["W"] = SDLK_w;
    allkeys["WAKE"] = SDLK_POWER;
    allkeys["WEBBACK"] = SDLK_AC_BACK;
    allkeys["WEBFAVORITES"] = SDLK_AC_BOOKMARKS;
    allkeys["WEBFORWARD"] = SDLK_AC_FORWARD;
    allkeys["WEBHOME"] = SDLK_AC_HOME;
    allkeys["WEBREFRESH"] = SDLK_AC_REFRESH;
    allkeys["WEBSEARCH"] = SDLK_AC_SEARCH;
    allkeys["WEBSTOP"] = SDLK_AC_STOP;
    allkeys["X"] = SDLK_x;
    allkeys["Y"] = SDLK_y;
    //allkeys["YEN"] - no SDL KeyCode, should we use scancodes? - SDL_SCANCODE_INTERNATIONAL3 // See SDL_scancode.h
    allkeys["Z"] = SDLK_z;
}

String InputEngine::getKeyForCommand(int eventID)
{
    EventMap::iterator it = events.find(eventID);

    if (it == events.end())
        return String();
    if (it->second.empty())
        return String();

    return this->getKeyNameForKeyCode(it->second.front().keyCode);
}

Ogre::String InputEngine::getModifierKeyName(OIS::KeyCode key)
{
    switch (key)
    {
    case OIS::KC_LMENU: return _LC("ModifierKey", "Left Alt");
    case OIS::KC_LSHIFT: return _LC("ModifierKey", "Left Shift");
    case OIS::KC_LCONTROL: return _LC("ModifierKey", "Left Ctrl");

    case OIS::KC_RMENU: return _LC("ModifierKey", "Right Alt");
    case OIS::KC_RSHIFT: return _LC("ModifierKey", "Right Shift");
    case OIS::KC_RCONTROL: return _LC("ModifierKey", "Right Ctrl");

    default: return "";
    }
}

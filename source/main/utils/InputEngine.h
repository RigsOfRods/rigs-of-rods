/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2021 Petr Ohlidal

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

///    @file
///    @brief Handles controller inputs from player.
///           Defines input events and binding mechanism, see `eventtypes`, `events`.
///           Reads bindings from "*.map" file(s).

#pragma once

#include "Application.h"
#include "ForceFeedback.h"

#include <OgreUTFString.h>
#include "OISEvents.h"
#include "OISForceFeedback.h"
#include "OISInputManager.h"
#include "OISJoyStick.h"
#include "OISKeyboard.h"
#include "OISMouse.h"

#define MAX_JOYSTICKS 10
#define MAX_JOYSTICK_POVS 4
#define MAX_JOYSTICK_SLIDERS 4
#define MAX_JOYSTICK_AXIS 32

namespace RoR {

enum eventtypes
{
    ET_NONE=0,
    ET_Keyboard,
    ET_MouseButton,
    ET_MouseAxisX,
    ET_MouseAxisY,
    ET_MouseAxisZ,
    ET_JoystickButton,
    ET_JoystickAxisAbs,
    ET_JoystickAxisRel,
    ET_JoystickPov,
    ET_JoystickSliderX,
    ET_JoystickSliderY,
    ET_END
};

enum class InputSourceType
{
    IST_ANY,
    IST_DIGITAL,
    IST_ANALOG
};

enum events
{
    // has to start at zero, since we iterate over it at times
    EV_AIRPLANE_AIRBRAKES_FULL=0,
    EV_AIRPLANE_AIRBRAKES_LESS,
    EV_AIRPLANE_AIRBRAKES_MORE,
    EV_AIRPLANE_AIRBRAKES_NONE,
    EV_AIRPLANE_BRAKE, //!< normal brake for an aircraft.
    EV_AIRPLANE_ELEVATOR_DOWN, //!< pull the elevator down in an aircraft.
    EV_AIRPLANE_ELEVATOR_UP, //!< pull the elevator up in an aircraft.
    EV_AIRPLANE_FLAPS_FULL, //!< full flaps in an aircraft.
    EV_AIRPLANE_FLAPS_LESS, //!< one step less flaps.
    EV_AIRPLANE_FLAPS_MORE, //!< one step more flaps.
    EV_AIRPLANE_FLAPS_NONE, //!< no flaps.
    EV_AIRPLANE_PARKING_BRAKE, //!< airplane parking brake.
    EV_AIRPLANE_REVERSE, //!< reverse the turboprops
    EV_AIRPLANE_RUDDER_LEFT, //!< rudder left
    EV_AIRPLANE_RUDDER_RIGHT, //!< rudder right
    EV_AIRPLANE_STEER_LEFT, //!< steer left
    EV_AIRPLANE_STEER_RIGHT, //!< steer right
    EV_AIRPLANE_THROTTLE,
    EV_AIRPLANE_THROTTLE_AXIS, //!< throttle axis. Only use this if you have fitting hardware :) (i.e. a Slider)
    EV_AIRPLANE_THROTTLE_DOWN, //!< decreases the airplane thrust
    EV_AIRPLANE_THROTTLE_FULL, //!< full thrust
    EV_AIRPLANE_THROTTLE_NO, //!< no thrust
    EV_AIRPLANE_THROTTLE_UP, //!< increase the airplane thrust
    EV_AIRPLANE_TOGGLE_ENGINES, //!< switch all engines on / off
    EV_BOAT_CENTER_RUDDER, //!< center the rudder
    EV_BOAT_REVERSE, //!< no thrust
    EV_BOAT_STEER_LEFT, //!< steer left a step
    EV_BOAT_STEER_LEFT_AXIS, //!< steer left (analog value!)
    EV_BOAT_STEER_RIGHT, //!< steer right a step
    EV_BOAT_STEER_RIGHT_AXIS, //!< steer right (analog value!)
    EV_BOAT_THROTTLE_AXIS, //!< throttle axis. Only use this if you have fitting hardware :) (i.e. a Slider)
    EV_BOAT_THROTTLE_DOWN, //!< decrease throttle
    EV_BOAT_THROTTLE_UP, //!< increase throttle
    EV_SKY_DECREASE_TIME, //!< decrease day-time
    EV_SKY_DECREASE_TIME_FAST, //!< decrease day-time a lot faster
    EV_SKY_INCREASE_TIME, //!< increase day-time
    EV_SKY_INCREASE_TIME_FAST, //!< increase day-time a lot faster
    EV_CAMERA_CHANGE, //!< change camera mode
    EV_CAMERA_DOWN,
    EV_CAMERA_FREE_MODE,
    EV_CAMERA_FREE_MODE_FIX,
    EV_CAMERA_LOOKBACK, //!< look back (toggles between normal and lookback)
    EV_CAMERA_RESET, //!< reset the camera position
    EV_CAMERA_ROTATE_DOWN, //!< rotate camera down
    EV_CAMERA_ROTATE_LEFT, //!< rotate camera left
    EV_CAMERA_ROTATE_RIGHT, //!< rotate camera right
    EV_CAMERA_ROTATE_UP, //!< rotate camera up
    EV_CAMERA_UP,
    EV_CAMERA_ZOOM_IN, //!< zoom camera in
    EV_CAMERA_ZOOM_IN_FAST, //!< zoom camera in faster
    EV_CAMERA_ZOOM_OUT, //!< zoom camera out
    EV_CAMERA_ZOOM_OUT_FAST, //!< zoom camera out faster
    EV_CHARACTER_BACKWARDS, //!< step backwards with the character
    EV_CHARACTER_FORWARD, //!< step forward with the character
    EV_CHARACTER_JUMP, //!< let the character jump
    EV_CHARACTER_LEFT, //!< rotate character left
    EV_CHARACTER_RIGHT, //!< rotate character right
    EV_CHARACTER_ROT_DOWN,
    EV_CHARACTER_ROT_UP,
    EV_CHARACTER_RUN, //!< let the character run
    EV_CHARACTER_SIDESTEP_LEFT, //!< sidestep to the left
    EV_CHARACTER_SIDESTEP_RIGHT, //!< sidestep to the right
    EV_COMMANDS_01, //!< Command 1
    EV_COMMANDS_02, //!< Command 2
    EV_COMMANDS_03, //!< Command 3
    EV_COMMANDS_04, //!< Command 4
    EV_COMMANDS_05, //!< Command 5
    EV_COMMANDS_06, //!< Command 6
    EV_COMMANDS_07, //!< Command 7
    EV_COMMANDS_08, //!< Command 8
    EV_COMMANDS_09, //!< Command 9
    EV_COMMANDS_10, //!< Command 10
    EV_COMMANDS_11, //!< Command 11
    EV_COMMANDS_12, //!< Command 12
    EV_COMMANDS_13, //!< Command 13
    EV_COMMANDS_14, //!< Command 14
    EV_COMMANDS_15, //!< Command 15
    EV_COMMANDS_16, //!< Command 16
    EV_COMMANDS_17, //!< Command 17
    EV_COMMANDS_18, //!< Command 18
    EV_COMMANDS_19, //!< Command 19
    EV_COMMANDS_20, //!< Command 20
    EV_COMMANDS_21, //!< Command 21
    EV_COMMANDS_22, //!< Command 22
    EV_COMMANDS_23, //!< Command 23
    EV_COMMANDS_24, //!< Command 24
    EV_COMMANDS_25, //!< Command 25
    EV_COMMANDS_26, //!< Command 26
    EV_COMMANDS_27, //!< Command 27
    EV_COMMANDS_28, //!< Command 28
    EV_COMMANDS_29, //!< Command 29
    EV_COMMANDS_30, //!< Command 30
    EV_COMMANDS_31, //!< Command 31
    EV_COMMANDS_32, //!< Command 32
    EV_COMMANDS_33, //!< Command 33
    EV_COMMANDS_34, //!< Command 34
    EV_COMMANDS_35, //!< Command 35
    EV_COMMANDS_36, //!< Command 36
    EV_COMMANDS_37, //!< Command 37
    EV_COMMANDS_38, //!< Command 38
    EV_COMMANDS_39, //!< Command 39
    EV_COMMANDS_40, //!< Command 40
    EV_COMMANDS_41, //!< Command 41
    EV_COMMANDS_42, //!< Command 42
    EV_COMMANDS_43, //!< Command 43
    EV_COMMANDS_44, //!< Command 44
    EV_COMMANDS_45, //!< Command 45
    EV_COMMANDS_46, //!< Command 46
    EV_COMMANDS_47, //!< Command 47
    EV_COMMANDS_48, //!< Command 48
    EV_COMMANDS_49, //!< Command 49
    EV_COMMANDS_50, //!< Command 50
    EV_COMMANDS_51, //!< Command 51
    EV_COMMANDS_52, //!< Command 52
    EV_COMMANDS_53, //!< Command 53
    EV_COMMANDS_54, //!< Command 54
    EV_COMMANDS_55, //!< Command 55
    EV_COMMANDS_56, //!< Command 56
    EV_COMMANDS_57, //!< Command 57
    EV_COMMANDS_58, //!< Command 58
    EV_COMMANDS_59, //!< Command 59
    EV_COMMANDS_60, //!< Command 50
    EV_COMMANDS_61, //!< Command 61
    EV_COMMANDS_62, //!< Command 62
    EV_COMMANDS_63, //!< Command 63
    EV_COMMANDS_64, //!< Command 64
    EV_COMMANDS_65, //!< Command 65
    EV_COMMANDS_66, //!< Command 66
    EV_COMMANDS_67, //!< Command 67
    EV_COMMANDS_68, //!< Command 68
    EV_COMMANDS_69, //!< Command 69
    EV_COMMANDS_70, //!< Command 70
    EV_COMMANDS_71, //!< Command 71
    EV_COMMANDS_72, //!< Command 72
    EV_COMMANDS_73, //!< Command 73
    EV_COMMANDS_74, //!< Command 74
    EV_COMMANDS_75, //!< Command 75
    EV_COMMANDS_76, //!< Command 76
    EV_COMMANDS_77, //!< Command 77
    EV_COMMANDS_78, //!< Command 78
    EV_COMMANDS_79, //!< Command 79
    EV_COMMANDS_80, //!< Command 80
    EV_COMMANDS_81, //!< Command 81
    EV_COMMANDS_82, //!< Command 82
    EV_COMMANDS_83, //!< Command 83
    EV_COMMANDS_84, //!< Command 84
    EV_COMMON_ACCELERATE_SIMULATION, //!< accelerate the simulation speed
    EV_COMMON_DECELERATE_SIMULATION, //!< decelerate the simulation speed
    EV_COMMON_RESET_SIMULATION_PACE, //!< reset the simulation speed
    EV_COMMON_AUTOLOCK, //!< unlock autolock hook node
    EV_COMMON_CONSOLE_TOGGLE, //!< show / hide the console
    EV_COMMON_ENTER_CHATMODE, //!< enter the chat mode
    EV_COMMON_ENTER_OR_EXIT_TRUCK, //!< enter or exit a truck
    EV_COMMON_ENTER_NEXT_TRUCK, //!< enter next truck
    EV_COMMON_ENTER_PREVIOUS_TRUCK, //!< enter previous truck
    EV_COMMON_REMOVE_CURRENT_TRUCK, //!< remove current truck
    EV_COMMON_RESPAWN_LAST_TRUCK, //!< respawn last truck
    EV_COMMON_FOV_LESS, //!<decreases the current FOV value
    EV_COMMON_FOV_MORE, //!<increases the current FOV value
    EV_COMMON_FOV_RESET, //!<reset the FOV value
    EV_COMMON_FULLSCREEN_TOGGLE,
    EV_COMMON_HIDE_GUI, //!< hide all GUI elements
    EV_COMMON_TOGGLE_DASHBOARD, //!< display or hide the dashboard overlay
    EV_COMMON_LOCK, //!< connect hook node to a node in close proximity
    EV_COMMON_NETCHATDISPLAY,
    EV_COMMON_NETCHATMODE,
    EV_COMMON_OUTPUT_POSITION, //!< write current position to log (you can open the logfile and reuse the position)
    EV_COMMON_GET_NEW_VEHICLE, //!< get new vehicle
    EV_COMMON_PRESSURE_LESS, //!< decrease tire pressure (note: only very few trucks support this)
    EV_COMMON_PRESSURE_MORE, //!< increase tire pressure (note: only very few trucks support this)
    EV_COMMON_QUICKLOAD, //!< quickload scene from the disk
    EV_COMMON_QUICKSAVE, //!< quicksave scene to the disk
    EV_COMMON_QUIT_GAME, //!< exit the game
    EV_COMMON_REPAIR_TRUCK, //!< repair truck to original condition
    EV_COMMON_REPLAY_BACKWARD,
    EV_COMMON_REPLAY_FAST_BACKWARD,
    EV_COMMON_REPLAY_FAST_FORWARD,
    EV_COMMON_REPLAY_FORWARD,
    EV_COMMON_RESCUE_TRUCK, //!< teleport to rescue truck
    EV_COMMON_RESET_TRUCK, //!< reset truck to original starting position
    EV_COMMON_TOGGLE_RESET_MODE, //!< toggle truck reset truck mode (soft vs. hard)
    EV_COMMON_ROPELOCK, //!< connect hook node to a node in close proximity
    EV_COMMON_SAVE_TERRAIN, //!< save terrain mesh to file
    EV_COMMON_SCREENSHOT, //!< take a screenshot
    EV_COMMON_SCREENSHOT_BIG, //!< take a BIG screenshot
    EV_COMMON_SECURE_LOAD, //!< tie a load to the truck
    EV_COMMON_SEND_CHAT, //!< send the chat text
    EV_COMMON_TOGGLE_DEBUG_VIEW, //!< toggle debug view mode
    EV_COMMON_CYCLE_DEBUG_VIEWS, //!< cycle debug view mode
    EV_COMMON_TOGGLE_TERRAIN_EDITOR, //!< toggle terrain editor
    EV_COMMON_TOGGLE_CUSTOM_PARTICLES, //!< toggle particle cannon
    EV_COMMON_TOGGLE_MAT_DEBUG, //!< debug purpose - dont use (currently not used)
    EV_COMMON_TOGGLE_RENDER_MODE, //!< toggle render mode (solid, wireframe and points)
    EV_COMMON_TOGGLE_REPLAY_MODE, //!< toggle replay mode
    EV_COMMON_TOGGLE_PHYSICS, //!< toggle physics on/off
    EV_COMMON_TOGGLE_STATS, //!< toggle Ogre statistics (FPS etc.)
    EV_COMMON_TOGGLE_TRUCK_BEACONS, //!< toggle truck beacons
    EV_COMMON_TOGGLE_TRUCK_LIGHTS, //!< toggle truck front lights
    EV_COMMON_TRUCK_INFO, //!< toggle truck HUD
    EV_COMMON_TRUCK_DESCRIPTION, //!< toggle truck description
    EV_COMMON_TRUCK_REMOVE,
    EV_GRASS_LESS, //!< EXPERIMENTAL: remove some grass
    EV_GRASS_MORE, //!< EXPERIMENTAL: add some grass
    EV_GRASS_MOST, //!< EXPERIMENTAL: set maximum amount of grass
    EV_GRASS_NONE, //!< EXPERIMENTAL: remove grass completely
    EV_GRASS_SAVE, //!< EXPERIMENTAL: save changes to the grass density image
    EV_MENU_DOWN, //!< select next element in current category
    EV_MENU_LEFT, //!< select previous category
    EV_MENU_RIGHT, //!< select next category
    EV_MENU_SELECT, //!< select focussed item and close menu
    EV_MENU_UP, //!< select previous element in current category
    EV_SURVEY_MAP_TOGGLE_ICONS, //!< toggle map icons
    EV_SURVEY_MAP_CYCLE, //!< cycle overview-map mode
    EV_SURVEY_MAP_TOGGLE, //!< toggle overview-map mode
    EV_SURVEY_MAP_ZOOM_IN, //!< increase survey map scale
    EV_SURVEY_MAP_ZOOM_OUT, //!< decrease survey map scale

    EV_TRUCK_ACCELERATE, //!< accelerate the truck
    EV_TRUCK_ACCELERATE_MODIFIER_25, //!< accelerate with 25 percent pedal input
    EV_TRUCK_ACCELERATE_MODIFIER_50, //!< accelerate with 50 percent pedal input
    EV_TRUCK_ANTILOCK_BRAKE, //!< toggle antilockbrake system
    EV_TRUCK_AUTOSHIFT_DOWN, //!< shift automatic transmission one gear down
    EV_TRUCK_AUTOSHIFT_UP, //!< shift automatic transmission one gear up
    EV_TRUCK_BLINK_LEFT, //!< toggle left direction indicator (blinker)
    EV_TRUCK_BLINK_RIGHT, //!< toggle right direction indicator (blinker)
    EV_TRUCK_BLINK_WARN, //!< toggle all direction indicators
    EV_TRUCK_BRAKE, //!< brake
    EV_TRUCK_BRAKE_MODIFIER_25, //!< brake with 25 percent pedal input
    EV_TRUCK_BRAKE_MODIFIER_50, //!< brake with 50 percent pedal input
    EV_TRUCK_CRUISE_CONTROL, //!< toggle cruise control
    EV_TRUCK_CRUISE_CONTROL_ACCL,//!< increase target speed / rpm
    EV_TRUCK_CRUISE_CONTROL_DECL,//!< decrease target speed / rpm
    EV_TRUCK_CRUISE_CONTROL_READJUST, //!< match target speed / rpm with current truck speed / rpm
    EV_TRUCK_HORN, //!< truck horn
    EV_TRUCK_LEFT_MIRROR_LEFT,
    EV_TRUCK_LEFT_MIRROR_RIGHT,
    EV_TRUCK_LIGHTTOGGLE01, //!< toggle custom light 1
    EV_TRUCK_LIGHTTOGGLE02, //!< toggle custom light 2
    EV_TRUCK_LIGHTTOGGLE03, //!< toggle custom light 3
    EV_TRUCK_LIGHTTOGGLE04, //!< toggle custom light 4
    EV_TRUCK_LIGHTTOGGLE05, //!< toggle custom light 5
    EV_TRUCK_LIGHTTOGGLE06, //!< toggle custom light 6
    EV_TRUCK_LIGHTTOGGLE07, //!< toggle custom light 7
    EV_TRUCK_LIGHTTOGGLE08, //!< toggle custom light 8
    EV_TRUCK_LIGHTTOGGLE09, //!< toggle custom light 9
    EV_TRUCK_LIGHTTOGGLE10, //!< toggle custom light 10
    EV_TRUCK_MANUAL_CLUTCH, //!< manual clutch (for manual transmission)
    EV_TRUCK_PARKING_BRAKE, //!< toggle parking brake
    EV_TRUCK_TRAILER_PARKING_BRAKE, //!< toggle trailer parking brake
    EV_TRUCK_RIGHT_MIRROR_LEFT,
    EV_TRUCK_RIGHT_MIRROR_RIGHT,
    EV_TRUCK_SHIFT_DOWN, //!< shift one gear down in manual transmission mode
    EV_TRUCK_SHIFT_GEAR01, //!< shift directly into this gear
    EV_TRUCK_SHIFT_GEAR02, //!< shift directly into this gear
    EV_TRUCK_SHIFT_GEAR03, //!< shift directly into this gear
    EV_TRUCK_SHIFT_GEAR04, //!< shift directly into this gear
    EV_TRUCK_SHIFT_GEAR05, //!< shift directly into this gear
    EV_TRUCK_SHIFT_GEAR06, //!< shift directly into this gear
    EV_TRUCK_SHIFT_GEAR07, //!< shift directly into this gear
    EV_TRUCK_SHIFT_GEAR08, //!< shift directly into this gear
    EV_TRUCK_SHIFT_GEAR09, //!< shift directly into this gear
    EV_TRUCK_SHIFT_GEAR10, //!< shift directly into this gear
    EV_TRUCK_SHIFT_GEAR11, //!< shift directly into this gear
    EV_TRUCK_SHIFT_GEAR12, //!< shift directly into this gear
    EV_TRUCK_SHIFT_GEAR13, //!< shift directly into this gear
    EV_TRUCK_SHIFT_GEAR14, //!< shift directly into this gear
    EV_TRUCK_SHIFT_GEAR15, //!< shift directly into this gear
    EV_TRUCK_SHIFT_GEAR16, //!< shift directly into this gear
    EV_TRUCK_SHIFT_GEAR17, //!< shift directly into this gear
    EV_TRUCK_SHIFT_GEAR18, //!< shift directly into this gear
    EV_TRUCK_SHIFT_GEAR_REVERSE, //!< shift directly into this gear
    EV_TRUCK_SHIFT_HIGHRANGE, //!< select high range (13-18) for H-shaft
    EV_TRUCK_SHIFT_LOWRANGE, //!< select low range (1-6) for H-shaft
    EV_TRUCK_SHIFT_MIDRANGE, //!< select middle range (7-12) for H-shaft
    EV_TRUCK_SHIFT_NEUTRAL, //!< shift to neutral gear in manual transmission mode
    EV_TRUCK_SHIFT_UP, //!< shift one gear up in manual transmission mode
    EV_TRUCK_STARTER, //!< hold to start the engine
    EV_TRUCK_STEER_LEFT, //!< steer left
    EV_TRUCK_STEER_RIGHT, //!< steer right
    EV_TRUCK_SWITCH_SHIFT_MODES, //!< toggle between transmission modes
    EV_TRUCK_TOGGLE_CONTACT, //!< toggle ignition
    EV_TRUCK_TOGGLE_FORWARDCOMMANDS, //!< toggle forwardcommands
    EV_TRUCK_TOGGLE_IMPORTCOMMANDS, //!< toggle importcommands
    EV_TRUCK_TOGGLE_INTER_AXLE_DIFF, //!< toggle the inter axle differential mode
    EV_TRUCK_TOGGLE_INTER_WHEEL_DIFF, //!< toggle the inter wheel differential mode
    EV_TRUCK_TOGGLE_PHYSICS, //!< toggle physics simulation
    EV_TRUCK_TOGGLE_TCASE_4WD_MODE, //!< toggle the transfer case 4wd mode
    EV_TRUCK_TOGGLE_TCASE_GEAR_RATIO, //!< toggle the transfer case gear ratio
    EV_TRUCK_TOGGLE_VIDEOCAMERA, //!< toggle videocamera update
    EV_TRUCK_TRACTION_CONTROL, //!< toggle antilockbrake system

    // Savegames
    EV_COMMON_QUICKSAVE_01,
    EV_COMMON_QUICKSAVE_02,
    EV_COMMON_QUICKSAVE_03,
    EV_COMMON_QUICKSAVE_04,
    EV_COMMON_QUICKSAVE_05,
    EV_COMMON_QUICKSAVE_06,
    EV_COMMON_QUICKSAVE_07,
    EV_COMMON_QUICKSAVE_08,
    EV_COMMON_QUICKSAVE_09,
    EV_COMMON_QUICKSAVE_10,

    EV_COMMON_QUICKLOAD_01,
    EV_COMMON_QUICKLOAD_02,
    EV_COMMON_QUICKLOAD_03,
    EV_COMMON_QUICKLOAD_04,
    EV_COMMON_QUICKLOAD_05,
    EV_COMMON_QUICKLOAD_06,
    EV_COMMON_QUICKLOAD_07,
    EV_COMMON_QUICKLOAD_08,
    EV_COMMON_QUICKLOAD_09,
    EV_COMMON_QUICKLOAD_10,

    EV_TRUCKEDIT_RELOAD,

    // the end, DO NOT MODIFY
    EV_MODE_BEFORELAST,
    EV_MODE_LAST
};

struct InputEvent
{
    Ogre::String name;
    int eventID;
    Ogre::String defaultKey;
    Ogre::UTFString description;
};

struct event_trigger_t
{
    // general
    enum eventtypes eventtype;
    int configDeviceID;            //!< For which device (which config file) was this binding defined?
    // keyboard
    int keyCode;
    bool explicite;
    bool ctrl;
    bool shift;
    bool alt;
    //mouse
    int mouseButtonNumber;
    //joystick buttons
    int joystickNumber;
    int joystickButtonNumber;
    // joystick axis
    int joystickAxisNumber;
    float joystickAxisDeadzone;
    float joystickAxisLinearity;
    int joystickAxisRegion;
    bool joystickAxisReverse;
    bool joystickAxisHalf;
    bool joystickAxisUseDigital;

    // POVs
    int joystickPovNumber;
    int joystickPovDirection;

    // Sliders
    int joystickSliderNumber;
    int joystickSliderReverse;
    int joystickSliderRegion;

    //others
    char configline[256];
    char group[128];
    char tmp_eventname[128];
    char comments[1024];
};

/// Manages controller configuration, evaluates input events
class InputEngine : public ZeroedMemoryAllocator
{
public:
    typedef std::vector<event_trigger_t> TriggerVec;
    typedef std::map<int, TriggerVec> EventMap;

    std::string DEFAULT_MAPFILE = "input.map";

    InputEngine();
    ~InputEngine();

        // Setup

    void                SetKeyboardListener(OIS::KeyListener* obj);
    OIS::Keyboard*      GetOisKeyboard() { return mKeyboard; }
    void                SetMouseListener(OIS::MouseListener* obj);
    void                SetJoystickListener(OIS::JoyStickListener* obj);
    void                destroy();

        // Input processing

    void                Capture();
    void                updateKeyBounces(float dt);
    void                ProcessMouseEvent(const OIS::MouseEvent& arg);
    void                ProcessKeyPress(const OIS::KeyEvent& arg);
    void                ProcessKeyRelease(const OIS::KeyEvent& arg);
    void                ProcessJoystickEvent(const OIS::JoyStickEvent& arg);
    void                resetKeys();

        // Event info

    Ogre::String        getKeyForCommand(int eventID);
    Ogre::String        getDeviceName(event_trigger_t const& evt);
    Ogre::String        getEventCommand(int eventID);
    bool                isEventDefined(int eventID);
    int                 getKeboardKeyForCommand(int eventID);               //!< Returns -1 if not Keyboard
    int                 getJoyComponentCount(OIS::ComponentType type, int joystickNumber);
    std::string         getJoyVendor(int joystickNumber);
    int                 getNumJoysticks() { return free_joysticks; }
    EventMap&           getEvents() { return events; };

        // Event config files

    bool                loadConfigFile(int deviceID = -1);                  //!< Loads config file specific to a device and OS (or default config if deviceID is -1).
    bool                saveConfigFile(int deviceID = -1);                  //!< Wites events with matching deviceID to loaded file with matching deviceID (or default file if deviceID is -1).
    std::string const&  getLoadedConfigFile(int deviceID = -1);             //!< Returns filename from `loadConfigFile()` call.
    bool                processLine(const char* line, int deviceID = -1);
    bool                updateConfigline(event_trigger_t* t);

        // Event management

    void                addEvent(int eventID, event_trigger_t& t);
    void                updateEvent(int eventID, const event_trigger_t& t);
    void                eraseEvent(int eventID, const event_trigger_t* t);
    void                clearEvents(int eventID);                           //!< Clears all bindings for given event.
    void                clearEventsByDevice(int deviceID);                  //!< Clears all bindings with given deviceID (-1 is no exception).
    void                clearAllEvents();                                   //!< Purges all configured bindings.

        // Event states

                        ///valueSource: IST_ANY=digital and analog devices, IST_DIGITAL=only digital, IST_ANALOG=only analog
    float               getEventValue(int eventID, bool pure = false, InputSourceType valueSource = InputSourceType::IST_ANY);
    bool                getEventBoolValue(int eventID);
    bool                isEventAnalog(int eventID);
    bool                getEventBoolValueBounce(int eventID, float time = 0.2f);
    float               getEventBounceTime(int eventID);
    bool                isKeyDownEffective(OIS::KeyCode mod);               //!< Reads RoR internal buffer
    bool                isKeyDownValueBounce(OIS::KeyCode mod, float time = 0.2f);

        // Direct input device states

    OIS::JoyStickState* getCurrentJoyState(int joystickNumber);
    OIS::MouseState     getMouseState();
    bool                isKeyDown(OIS::KeyCode mod);                        //!< Asks OIS directly
    int                 getCurrentKeyCombo(Ogre::String* combo);
    int                 getCurrentJoyButton(int& joystickNumber, int& button);
    int                 getCurrentPovValue(int& joystickNumber, int& pov, int& povdir);

        // Event utils

    static const char*  getEventTypeName(eventtypes type);                  //!< Enum to string helper.
    static int          resolveEventName(Ogre::String eventName);
    static Ogre::String eventIDToName(int eventID);
    static Ogre::String eventIDToDescription(int eventID);
    std::string         getKeyNameForKeyCode(OIS::KeyCode keycode);


        // Misc

    void windowResized(Ogre::RenderWindow* rw);
    OIS::ForceFeedback* getForceFeedbackDevice() { return mForceFeedback; };

protected:

    //OIS Input devices
    OIS::InputManager* mInputManager;
    OIS::Mouse* mMouse;
    OIS::Keyboard* mKeyboard;
    OIS::JoyStick* mJoy[MAX_JOYSTICKS];
    int free_joysticks; //!< Number of detected game controllers
    OIS::ForceFeedback* mForceFeedback;
    int uniqueCounter;

    // this stores the key/button/axis values
    std::map<int, bool> keyState;
    OIS::JoyStickState joyState[MAX_JOYSTICKS];
    OIS::MouseState mouseState;

    // define event aliases
    std::map<int, std::vector<event_trigger_t>> events;
    std::map<int, float> event_times;
    std::string m_loaded_configs[MAX_JOYSTICKS];
    bool loadMapping(Ogre::String fileName, int deviceID);
    bool saveMapping(Ogre::String fileName, int deviceID);
    void completeMissingEvents();

    void initAllKeys();
    void setup();
    std::map<std::string, OIS::KeyCode> allkeys;
    std::map<std::string, OIS::KeyCode>::iterator allit;

    float deadZone(float axis, float dz);
    float axisLinearity(float axisValue, float linearity);

    float logval(float val);
    std::string getEventGroup(Ogre::String eventName);

    event_trigger_t newEvent();
};

} // namespace RoR

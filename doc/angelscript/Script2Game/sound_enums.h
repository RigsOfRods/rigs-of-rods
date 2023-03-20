/*
This source file is part of Rigs of Rods

For more information, see http://www.rigsofrods.org/

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

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */   

namespace Script2Game {

/**
* Binding of `RoR::SoundTriggers`
*/
enum SoundTriggers {
    SS_TRIG_NONE = -1,
    SS_TRIG_ENGINE = 0,
    SS_TRIG_AEROENGINE1,
    SS_TRIG_AEROENGINE2,
    SS_TRIG_AEROENGINE3,
    SS_TRIG_AEROENGINE4,
    SS_TRIG_HORN,
    SS_TRIG_BRAKE,
    SS_TRIG_PUMP,
    SS_TRIG_STARTER,
    SS_TRIG_TURBOBOV,
    SS_TRIG_TURBOWASTEGATE,
    SS_TRIG_TURBOBACKFIRE,
    SS_TRIG_ALWAYSON,
    SS_TRIG_REPAIR,
    SS_TRIG_AIR,
    SS_TRIG_GPWS_APDISCONNECT,
    SS_TRIG_GPWS_10,
    SS_TRIG_GPWS_20,
    SS_TRIG_GPWS_30,
    SS_TRIG_GPWS_40,
    SS_TRIG_GPWS_50,
    SS_TRIG_GPWS_100,
    SS_TRIG_GPWS_PULLUP,
    SS_TRIG_GPWS_MINIMUMS,
    SS_TRIG_AIR_PURGE,
    SS_TRIG_SHIFT,
    SS_TRIG_GEARSLIDE,
    SS_TRIG_CREAK,
    SS_TRIG_BREAK,
    SS_TRIG_SCREETCH,
    SS_TRIG_PARK,
    SS_TRIG_AFTERBURNER1,
    SS_TRIG_AFTERBURNER2,
    SS_TRIG_AFTERBURNER3,
    SS_TRIG_AFTERBURNER4,
    SS_TRIG_AFTERBURNER5,
    SS_TRIG_AFTERBURNER6,
    SS_TRIG_AFTERBURNER7,
    SS_TRIG_AFTERBURNER8,
    SS_TRIG_AEROENGINE5,
    SS_TRIG_AEROENGINE6,
    SS_TRIG_AEROENGINE7,
    SS_TRIG_AEROENGINE8,
    SS_TRIG_AOA,
    SS_TRIG_IGNITION,
    SS_TRIG_REVERSE_GEAR,
    SS_TRIG_TURN_SIGNAL,
    SS_TRIG_TURN_SIGNAL_TICK,
    SS_TRIG_TURN_SIGNAL_WARN_TICK,
    SS_TRIG_ALB_ACTIVE,
    SS_TRIG_TC_ACTIVE,
    SS_TRIG_AVICHAT01,
    SS_TRIG_AVICHAT02,
    SS_TRIG_AVICHAT03,
    SS_TRIG_AVICHAT04,
    SS_TRIG_AVICHAT05,
    SS_TRIG_AVICHAT06,
    SS_TRIG_AVICHAT07,
    SS_TRIG_AVICHAT08,
    SS_TRIG_AVICHAT09,
    SS_TRIG_AVICHAT10,
    SS_TRIG_AVICHAT11,
    SS_TRIG_AVICHAT12,
    SS_TRIG_AVICHAT13,
    SS_TRIG_LINKED_COMMAND,
    SS_TRIG_MAIN_MENU
};

/**
* Binding of `RoR::ModulationSources`
*/
enum ModulationSources {
    SS_MOD_NONE,
    SS_MOD_ENGINE,
    SS_MOD_TURBO,
    SS_MOD_AEROENGINE1,
    SS_MOD_AEROENGINE2,
    SS_MOD_AEROENGINE3,
    SS_MOD_AEROENGINE4,
    SS_MOD_WHEELSPEED,
    SS_MOD_INJECTOR,
    SS_MOD_TORQUE,
    SS_MOD_GEARBOX,
    SS_MOD_CREAK,
    SS_MOD_BREAK,
    SS_MOD_SCREETCH,
    SS_MOD_PUMP,
    SS_MOD_THROTTLE1,
    SS_MOD_THROTTLE2,
    SS_MOD_THROTTLE3,
    SS_MOD_THROTTLE4,
    SS_MOD_THROTTLE5,
    SS_MOD_THROTTLE6,
    SS_MOD_THROTTLE7,
    SS_MOD_THROTTLE8,
    SS_MOD_AEROENGINE5,
    SS_MOD_AEROENGINE6,
    SS_MOD_AEROENGINE7,
    SS_MOD_AEROENGINE8,
    SS_MOD_AIRSPEED,
    SS_MOD_AOA,
    SS_MOD_LINKED_COMMANDRATE,
    SS_MOD_MUSIC_VOLUME
};

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

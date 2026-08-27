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

#include "Actor.h"
#include "../AngelScriptBindings.h"
#include "ScriptEngine.h"
#include "ScriptUtils.h"
#include "SimData.h"
#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterActorCommon(asIScriptEngine* engine)
{
    int result;

    // enum truckStates
    result = engine->RegisterEnum("TruckState"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("TruckState", "TS_SIMULATED", static_cast<int>(ActorState::LOCAL_SIMULATED)); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("TruckState", "TS_SLEEPING", static_cast<int>(ActorState::LOCAL_SLEEPING)); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("TruckState", "TS_NETWORKED", static_cast<int>(ActorState::NETWORKED_OK)); ROR_ASSERT(result >= 0);

    // enum truckTypes
    result = engine->RegisterEnum("truckTypes"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("truckTypes", "TT_NOT_DRIVEABLE", NOT_DRIVEABLE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("truckTypes", "TT_TRUCK", TRUCK); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("truckTypes", "TT_AIRPLANE", AIRPLANE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("truckTypes", "TT_BOAT", BOAT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("truckTypes", "TT_MACHINE", MACHINE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("truckTypes", "TT_AI", AI); ROR_ASSERT(result >= 0);

    // enum FlareType
    result = engine->RegisterEnum("FlareType"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_NONE", (int)FlareType::NONE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_HEADLIGHT", (int)FlareType::HEADLIGHT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_HIGH_BEAM", (int)FlareType::HIGH_BEAM); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_FOG_LIGHT", (int)FlareType::FOG_LIGHT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_TAIL_LIGHT", (int)FlareType::TAIL_LIGHT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_BRAKE_LIGHT", (int)FlareType::BRAKE_LIGHT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_REVERSE_LIGHT", (int)FlareType::REVERSE_LIGHT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_SIDELIGHT", (int)FlareType::SIDELIGHT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_BLINKER_LEFT", (int)FlareType::BLINKER_LEFT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_BLINKER_RIGHT", (int)FlareType::BLINKER_RIGHT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_USER", (int)FlareType::USER); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("FlareType", "FLARE_TYPE_DASHBOARD", (int)FlareType::DASHBOARD); ROR_ASSERT(result >= 0);

    // enum BlinkType
    result = engine->RegisterEnum("BlinkType"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("BlinkType", "BLINK_NONE", (int)BLINK_NONE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("BlinkType", "BLINK_LEFT", (int)BLINK_LEFT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("BlinkType", "BLINK_RIGHT", (int)BLINK_RIGHT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("BlinkType", "BLINK_WARN", (int)BLINK_WARN); ROR_ASSERT(result >= 0);

    // enum ActorModifyRequestType
    result = engine->RegisterEnum("ActorModifyRequestType"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorModifyRequestType", "ACTOR_MODIFY_REQUEST_INVALID", (int)ActorModifyRequest::Type::INVALID); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorModifyRequestType", "ACTOR_MODIFY_REQUEST_RELOAD", (int)ActorModifyRequest::Type::RELOAD); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorModifyRequestType", "ACTOR_MODIFY_REQUEST_RESET_ON_INIT_POS", (int)ActorModifyRequest::Type::RESET_ON_INIT_POS); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorModifyRequestType", "ACTOR_MODIFY_REQUEST_RESET_ON_SPOT", (int)ActorModifyRequest::Type::RESET_ON_SPOT); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorModifyRequestType", "ACTOR_MODIFY_REQUEST_SOFT_RESET", (int)ActorModifyRequest::Type::SOFT_RESET); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorModifyRequestType", "ACTOR_MODIFY_REQUEST_RESTORE_SAVED", (int)ActorModifyRequest::Type::RESTORE_SAVED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorModifyRequestType", "ACTOR_MODIFY_REQUEST_WAKE_UP", (int)ActorModifyRequest::Type::WAKE_UP); ROR_ASSERT(result >= 0);

    // enum ActorSimAttr
    result = engine->RegisterEnum("ActorSimAttr"); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_NONE", (int)ACTORSIMATTR_NONE); ROR_ASSERT(result >= 0);
    // ... TractionControl
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_TC_RATIO", (int)ACTORSIMATTR_TC_RATIO); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_TC_PULSE_TIME", (int)ACTORSIMATTR_TC_PULSE_TIME); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_TC_WHEELSLIP_CONSTANT", (int)ACTORSIMATTR_TC_WHEELSLIP_CONSTANT); ROR_ASSERT(result >= 0);
    // ... Engine
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGINE_SHIFTDOWN_RPM", (int)ACTORSIMATTR_ENGINE_SHIFTDOWN_RPM); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGINE_SHIFTUP_RPM", (int)ACTORSIMATTR_ENGINE_SHIFTUP_RPM); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGINE_TORQUE", (int)ACTORSIMATTR_ENGINE_TORQUE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGINE_DIFF_RATIO", (int)ACTORSIMATTR_ENGINE_DIFF_RATIO); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGINE_GEAR_RATIOS_ARRAY", (int)ACTORSIMATTR_ENGINE_GEAR_RATIOS_ARRAY); ROR_ASSERT(result >= 0);
    // ... Engoption 
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_ENGINE_INERTIA", (int)ACTORSIMATTR_ENGOPTION_ENGINE_INERTIA); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_ENGINE_TYPE", (int)ACTORSIMATTR_ENGOPTION_ENGINE_TYPE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_CLUTCH_FORCE", (int)ACTORSIMATTR_ENGOPTION_CLUTCH_FORCE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_SHIFT_TIME", (int)ACTORSIMATTR_ENGOPTION_SHIFT_TIME); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_CLUTCH_TIME", (int)ACTORSIMATTR_ENGOPTION_CLUTCH_TIME); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_POST_SHIFT_TIME", (int)ACTORSIMATTR_ENGOPTION_POST_SHIFT_TIME); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_STALL_RPM", (int)ACTORSIMATTR_ENGOPTION_STALL_RPM); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_IDLE_RPM", (int)ACTORSIMATTR_ENGOPTION_IDLE_RPM); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_MAX_IDLE_MIXTURE", (int)ACTORSIMATTR_ENGOPTION_MAX_IDLE_MIXTURE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_MIN_IDLE_MIXTURE", (int)ACTORSIMATTR_ENGOPTION_MIN_IDLE_MIXTURE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGOPTION_BRAKING_TORQUE", (int)ACTORSIMATTR_ENGOPTION_BRAKING_TORQUE); ROR_ASSERT(result >= 0);
    // ... Engturbo2 (actually'engturbo'withtype=2)
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_INERTIA_FACTOR", (int)ACTORSIMATTR_ENGTURBO2_INERTIA_FACTOR); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_NUM_TURBOS", (int)ACTORSIMATTR_ENGTURBO2_NUM_TURBOS); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_MAX_RPM", (int)ACTORSIMATTR_ENGTURBO2_MAX_RPM); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_ENGINE_RPM_OP", (int)ACTORSIMATTR_ENGTURBO2_ENGINE_RPM_OP); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_BOV_ENABLED", (int)ACTORSIMATTR_ENGTURBO2_BOV_ENABLED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_BOV_MIN_PSI", (int)ACTORSIMATTR_ENGTURBO2_BOV_MIN_PSI); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_WASTEGATE_ENABLED", (int)ACTORSIMATTR_ENGTURBO2_WASTEGATE_ENABLED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_WASTEGATE_MAX_PSI", (int)ACTORSIMATTR_ENGTURBO2_WASTEGATE_MAX_PSI); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_WASTEGATE_THRESHOLD_N", (int)ACTORSIMATTR_ENGTURBO2_WASTEGATE_THRESHOLD_N); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_WASTEGATE_THRESHOLD_P", (int)ACTORSIMATTR_ENGTURBO2_WASTEGATE_THRESHOLD_P); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_ANTILAG_ENABLED", (int)ACTORSIMATTR_ENGTURBO2_ANTILAG_ENABLED); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_ANTILAG_CHANCE", (int)ACTORSIMATTR_ENGTURBO2_ANTILAG_CHANCE); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_ANTILAG_MIN_RPM", (int)ACTORSIMATTR_ENGTURBO2_ANTILAG_MIN_RPM); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_ENGTURBO2_ANTILAG_POWER", (int)ACTORSIMATTR_ENGTURBO2_ANTILAG_POWER); ROR_ASSERT(result >= 0);
    // ... Turbojets
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_TURBOJET_MAX_DRY_THRUST", (int)ACTORSIMATTR_TURBOJET_MAX_DRY_THRUST); ROR_ASSERT(result >= 0);
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_TURBOJET_MAX_WET_THRUST", (int)ACTORSIMATTR_TURBOJET_MAX_WET_THRUST); ROR_ASSERT(result >= 0);
    // ... Turboprops/Pistonprops
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_PROPENGINE_MAX_POWER", (int)ACTORSIMATTR_PROPENGINE_MAX_POWER); ROR_ASSERT(result >= 0);
    // ... Screwprops
    result = engine->RegisterEnumValue("ActorSimAttr", "ACTORSIMATTR_SCREWPROP_MAX_POWER", (int)ACTORSIMATTR_SCREWPROP_MAX_POWER); ROR_ASSERT(result >= 0);

}

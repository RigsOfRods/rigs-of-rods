
  // =================================================== //
  // THIS IS NOT A C++ HEADER! Only a dummy for Doxygen. //
  // =================================================== //

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */   

namespace Script2Game {

///  Parameter to `Script2Game::BeamClass::setSimAttribute()` and `Script2Game::BeamClass::getSimAttribute()`; allows advanced users to tweak physics internals via script.
///  Each value represents a C++ variable, either directly in `RoR::Actor` or a subsystem, i.e. `RoR::Engine`.
///  PAY ATTENTION to the 'safe value' limits below - those may not be checked when setting attribute values!
enum ActorSimAttr
{
    ACTORSIMATTR_NONE,

    // TractionControl
    ACTORSIMATTR_TC_RATIO, //!< Regulating force, safe values: <1 - 20>
    ACTORSIMATTR_TC_PULSE_TIME, //!< Pulse duration in seconds, safe values <0.005 - 1>
    ACTORSIMATTR_TC_WHEELSLIP_CONSTANT //!< Minimum wheel slip threshold, safe value = 0.25
    
    // Engine
    ACTORSIMATTR_ENGINE_SHIFTDOWN_RPM, //!< Automatic transmission - Param #1 of 'engine'
    ACTORSIMATTR_ENGINE_SHIFTUP_RPM, //!< Automatic transmission - Param #2 of 'engine'
    ACTORSIMATTR_ENGINE_TORQUE, //!< Engine torque in newton-meters (N/m) - Param #3 of 'engine'
    ACTORSIMATTR_ENGINE_DIFF_RATIO, //!< Differential ratio (aka global gear ratio) - Param #4 of 'engine'
    ACTORSIMATTR_ENGINE_GEAR_RATIOS_ARRAY, //!< Gearbox - Format: "<reverse_gear> <neutral_gear> <forward_gear 1> [<forward gear 2>]..."; Param #5 and onwards of 'engine'.

    // Engoption
    ACTORSIMATTR_ENGOPTION_ENGINE_INERTIA, //!< Param #1 of 'engoption'
    ACTORSIMATTR_ENGOPTION_ENGINE_TYPE, //!< Param #2 of 'engoption'
    ACTORSIMATTR_ENGOPTION_CLUTCH_FORCE, //!< Param #3 of 'engoption'
    ACTORSIMATTR_ENGOPTION_SHIFT_TIME, //!< Param #4 of 'engoption'
    ACTORSIMATTR_ENGOPTION_CLUTCH_TIME, //!< Param #5 of 'engoption'
    ACTORSIMATTR_ENGOPTION_POST_SHIFT_TIME, //!< Time (in seconds) until full torque is transferred - Param #6 of 'engoption'
    ACTORSIMATTR_ENGOPTION_STALL_RPM, //!< RPM where engine stalls - Param #7 of 'engoption'
    ACTORSIMATTR_ENGOPTION_IDLE_RPM, //!< Target idle RPM - Param #8 of 'engoption'
    ACTORSIMATTR_ENGOPTION_MAX_IDLE_MIXTURE, //!< Max throttle to maintain idle RPM - Param #9 of 'engoption'
    ACTORSIMATTR_ENGOPTION_MIN_IDLE_MIXTURE, //!< Min throttle to maintain idle RPM - Param #10 of 'engoption'
    ACTORSIMATTR_ENGOPTION_BRAKING_TORQUE, //!< How much engine brakes on zero throttle - Param #11 of 'engoption'

    // Engturbo2 (actually 'engturbo' with Param #1 [type] set to "2" - the recommended variant)
    ACTORSIMATTR_ENGTURBO2_INERTIA_FACTOR, //!< Time to spool up - Param #2 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_NUM_TURBOS, //!< Number of turbos - Param #3 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_MAX_RPM, //!< MaxPSI * 10000 ~ calculated from Param #4 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_ENGINE_RPM_OP, //!< Engine RPM threshold for turbo to operate - Param #5 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_BOV_ENABLED, //!< Blow-off valve - Param #6 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_BOV_MIN_PSI, //!< Blow-off valve PSI threshold - Param #7 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_WASTEGATE_ENABLED, //!<  Param #8 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_WASTEGATE_MAX_PSI, //!<  Param #9 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_WASTEGATE_THRESHOLD_N, //!< 1 - WgThreshold ~ calculated from  Param #10 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_WASTEGATE_THRESHOLD_P, //!< 1 + WgThreshold ~ calculated from  Param #10 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_ANTILAG_ENABLED, //!<  Param #11 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_ANTILAG_CHANCE, //!<  Param #12 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_ANTILAG_MIN_RPM, //!<  Param #13 of 'engturbo2'
    ACTORSIMATTR_ENGTURBO2_ANTILAG_POWER, //!<  Param #14 of 'engturbo2'    
};

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
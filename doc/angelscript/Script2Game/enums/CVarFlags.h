
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

/**
 * @brief Types and special attributes of cvars; see Script2Game::CVarClass
 * @note Default cvar type is string - To create a string cvar, enter '0' as flags.
 */
enum CVarFlags
{
    CVAR_TYPE_BOOL    = BITMASK(1),
    CVAR_TYPE_INT     = BITMASK(2),
    CVAR_TYPE_FLOAT   = BITMASK(3),
    CVAR_ARCHIVE      = BITMASK(4),    //!< Will be written to RoR.cfg
    CVAR_NO_LOG       = BITMASK(5)     //!< Will not be written to RoR.log
};

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
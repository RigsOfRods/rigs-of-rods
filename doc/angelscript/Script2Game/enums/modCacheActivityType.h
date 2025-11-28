
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

/// Argument #1 of script event `RoR::SE_GENERIC_MODCACHE_ACTIVITY`
enum modCacheActivityType
{
    MODCACHEACTIVITY_NONE,

    MODCACHEACTIVITY_ENTRY_ADDED,      //!< Args of `RoR::SE_GENERIC_MODCACHE_NOTIFICATION`: #1 type, #2 entry number, --, --, #5 fname, #6 fext
    MODCACHEACTIVITY_ENTRY_DELETED,    //!< Flagged as `deleted`, remains in memory until shared pointers expire; Args of `RoR::SE_GENERIC_MODCACHE_NOTIFICATION`: #1 type, #2 entry number, --, --, #5 fname, #6 fext

    MODCACHEACTIVITY_BUNDLE_LOADED,    //!< Args of `RoR::SE_GENERIC_MODCACHE_NOTIFICATION`: #1 type, #2 entry number, --, --, #5 rg name
    MODCACHEACTIVITY_BUNDLE_RELOADED,  //!< Args of `RoR::SE_GENERIC_MODCACHE_NOTIFICATION`: #1 type, #2 entry number, --, --, #5 rg name
    MODCACHEACTIVITY_BUNDLE_UNLOADED,  //!< Args of `RoR::SE_GENERIC_MODCACHE_NOTIFICATION`: #1 type, #2 entry number
    MODCACHEACTIVITY_BUNDLE_DELETED,   //!< Args of `RoR::SE_GENERIC_MODCACHE_NOTIFICATION`: $5 filename (for each associated entry, separate ENTRY_DELETED event is issued)
};

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
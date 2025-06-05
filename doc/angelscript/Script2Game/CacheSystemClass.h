
  // =================================================== //
  // THIS IS NOT A C++ HEADER! Only a dummy for Doxygen. //
  // =================================================== //

namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */    

/**
 * @brief Binding of RoR::CacheSystem; A database of all installed game content, allowing search and live updating.
 * @note This object is created automatically as global variable `modcache`.
 */
class CacheSystemClass
{
    /**
    * Returns null if none found; "Bundle-qualified" format also specifies the ZIP/directory in modcache, i.e. "mybundle.zip:myactor.truck"
    */
    CacheEntryClass @findEntryByFilename(LoaderType type, bool partial, const string &in _filename_maybe_bundlequalified);
    
    /**
    * Returns null if such number doesn't exist.
    */
    CacheEntryClass @getEntryByNumber(int);
    
    /**
    * Searches the installed content - syntax is exactly the same as used by the Loader UI.
    * @param query A dictionary representing the Query Parameters of `RoR::CacheQuery`; required params are "filter_type" (LoaderType), "filter_category_id" (int), "filter_guid" (string) and "search_expr" (string).
    * @returns A dictionary representing the Query Results of `RoR::CacheQuery` 
    */
    dictionary@ query(dictionary@ query);
}

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game
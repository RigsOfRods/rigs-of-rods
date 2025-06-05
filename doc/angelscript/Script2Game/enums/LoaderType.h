
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
* Binding of RoR::LoaderType; Search mode for `CacheSystemClass::Query()` & Operation mode for `RoR::GUI::MainSelector`
*/
enum LoaderType
{
    LOADER_TYPE_NONE,     
    LOADER_TYPE_TERRAIN,   //!< Invocable from GUI; No script alias, used in main menu
    LOADER_TYPE_VEHICLE,   //!< Script "vehicle",   ext: truck car  
    LOADER_TYPE_TRUCK,     //!< Script "truck",     ext: truck car
    LOADER_TYPE_CAR,       //!< Script "car",       ext: car
    LOADER_TYPE_BOAT,      //!< Script "boat",      ext: boat
    LOADER_TYPE_AIRPLANE,  //!< Script "airplane",  ext: airplane
    LOADER_TYPE_TRAILER,   //!< Script "trailer",   ext: trailer
    LOADER_TYPE_TRAIN,     //!< Script "train",     ext: train
    LOADER_TYPE_LOAD,      //!< Script "load",      ext: load
    LOADER_TYPE_EXTENSION, //!< Script "extension", ext: trailer load
    LOADER_TYPE_SKIN,      //!< No script alias, invoked automatically
    LOADER_TYPE_ALLBEAM,   //!< Invocable from GUI; Script "all",  ext: truck car boat airplane train load
    LOADER_TYPE_ADDONPART, //!< No script alias, invoked manually, ext: addonpart
    LOADER_TYPE_TUNEUP,    //!< No script alias, invoked manually, ext: tuneup
    LOADER_TYPE_ASSETPACK, //!< No script alias, invoked manually, ext: assetpack
    LOADER_TYPE_DASHBOARD, //!< No script alias, invoked manually, ext: dashboard
    LOADER_TYPE_GADGET,    //!< No script alias, invoked manually, ext: gadget
} 

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
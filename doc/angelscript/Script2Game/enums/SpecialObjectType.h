
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
* Binding of RoR::TObjSpecialObject, for use with TerrainEditorObjectClass
*/
enum SpecialObjectType
{
    // PLEASE maintain the same ordering as 'Application.h' 'scripting/bindings/TerrainAngelscript.cpp'

    SPECIAL_OBJECT_NONE,
    SPECIAL_OBJECT_TRUCK //!< auto-adjusted to fit terrain or water surface
    SPECIAL_OBJECT_LOAD,
    SPECIAL_OBJECT_MACHINE,
    SPECIAL_OBJECT_BOAT,
    SPECIAL_OBJECT_TRUCK2, //!< Free position (not auto-adjusted to fit terrain or water surface)
}

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
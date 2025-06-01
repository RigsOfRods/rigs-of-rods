
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
* Binding of RoR::CacheEntry; properties are used as variables, i.e. `string mypath = myentry.fpath;`.
*/
class CacheEntryClass
{
    // (Please maintain the same order as in 'CacheSystem.h' and 'CacheSystemAngelscript.cpp')
    
    const string& get_fpath() const property;
    const string& get_fname() const property";
    const string& get_fext() const property";                
    const string& get_dname() const property";               
    int           get_categoryid() const property";          
    const string& get_categoryname() const property";        
    const string& get_resource_bundle_type() const property";
    const string& get_resource_bundle_path() const property";
    int           get_number() const property";              
    bool          get_deleted() const property";             
    const string& get_filecachename() const property";       
    const string& get_resource_group() const property";      
}

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
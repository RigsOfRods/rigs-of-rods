namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */   

/**
 * @brief Binding of RoR::GenericDocument; Parses TRUCK/TOBJ/ODEF/CHARACTER file formats.
 */
class GenericDocumentClass
{
    /**
    * Loads and parses a document from OGRE resource system.
    */
    bool loadFromResource(string resource_name, string resource_group_name, int options = 0);
    
    /**
    * Saves the document to OGRE resource system.
    */
    bool saveToResource(string resource_name, string resource_group_name);
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game

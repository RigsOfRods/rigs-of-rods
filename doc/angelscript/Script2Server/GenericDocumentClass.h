namespace Script2Server {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Server
 *  @{
 */   

/**
 * @brief Binding of RoR::GenericDocument; Parses TRUCK/TOBJ/ODEF/CHARACTER file formats.
 */
class GenericDocumentClass
{
    /**
    * Loads and parses a document from dedicated server script directory.
    */
    bool loadFromFile(string filename, int options = 0);
    
    /**
    * Saves the document to dedicated server script directory.
    */
    bool saveToFile(string filename);
};

/// @}    //addtogroup Script2Server
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Server

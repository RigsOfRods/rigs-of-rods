namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */   

enum GenericDocumentOptions
{
    GENERIC_DOCUMENT_OPTION_ALLOW_NAKED_STRINGS, //!< Allow strings without quotes, for backwards compatibility.
    GENERIC_DOCUMENT_OPTION_ALLOW_SLASH_COMMENTS, //!< Allow comments starting with `//`. 
    GENERIC_DOCUMENT_OPTION_FIRST_LINE_IS_TITLE, //!< First non-empty & non-comment line is a naked string with spaces. 
    GENERIC_DOCUMENT_OPTION_ALLOW_SEPARATOR_COLON, //!< Allow ':' as separator between tokens.
    GENERIC_DOCUMENT_OPTION_PARENTHESES_CAPTURE_SPACES //!< If non-empty NAKED string encounters '(', following spaces will be captured until matching ')' is found.    
};

/**
 * @brief Binding of RoR::GenericDocument; Parses TRUCK/TOBJ/ODEF/CHARACTER file formats.
 */
class GenericDocumentClass
{
    /**
    * Loads and parses a document from OGRE resource system.
    */
    bool LoadFromResource(string resource_name, string resource_group_name, int options = 0);
    
    /**
    * Saves the document to OGRE resource system.
    */
    bool SaveToResource(string resource_name, string resource_group_name);
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game


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
* Binding of RoR::GenericDocumentOptions, used with Script2Game::GenericDocumentClass
*/
enum GenericDocumentOptions
{
    GENERIC_DOCUMENT_OPTION_ALLOW_NAKED_STRINGS, //!< Allow strings without quotes, for backwards compatibility.
    GENERIC_DOCUMENT_OPTION_ALLOW_SLASH_COMMENTS, //!< Allow comments starting with `//`. 
    GENERIC_DOCUMENT_OPTION_FIRST_LINE_IS_TITLE, //!< First non-empty & non-comment line is a naked string with spaces. 
    GENERIC_DOCUMENT_OPTION_ALLOW_SEPARATOR_COLON, //!< Allow ':' as separator between tokens.
    GENERIC_DOCUMENT_OPTION_PARENTHESES_CAPTURE_SPACES, //!< If non-empty NAKED string encounters '(', following spaces will be captured until matching ')' is found.    
    GENERIC_DOCUMENT_OPTION_ALLOW_BRACED_KEYWORDS, //!< Allow INI-like '[keyword]' tokens.
    GENERIC_DOCUMENT_OPTION_ALLOW_SEPARATOR_EQUALS, //!< Allow '=' as separator between tokens.
    GENERIC_DOCUMENT_OPTION_ALLOW_HASH_COMMENTS //!< Allow comments starting with `#`.     
};

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
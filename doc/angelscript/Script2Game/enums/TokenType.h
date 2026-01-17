
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
* Binding of RoR::TokenType, for use with Script2Game::GenericDocContextClass
*/
enum TokenType
{
    TOKEN_TYPE_NONE,
    TOKEN_TYPE_LINEBREAK,    //!< Input: LF (CR is ignored); Output: platform-specific.
    TOKEN_TYPE_COMMENT,      //!< Line starting with ; (skipping whitespace).
    TOKEN_TYPE_STRING,       //!< Quoted string.
    TOKEN_TYPE_FLOAT,        //!< Numbers with or without a decimal point.
    TOKEN_TYPE_INT,          //!< Only numbers without decimal point.
    TOKEN_TYPE_BOOL,         //!< Lowercase 'true'/'false'.
    TOKEN_TYPE_KEYWORD,      //!< Unquoted string at start of line (skipping whitespace).
};

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
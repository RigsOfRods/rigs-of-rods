
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
    TOKEN_TYPE_LINEBREAK,
    TOKEN_TYPE_COMMENT,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_FLOAT,
	TOKEN_TYPE_INT,
    TOKEN_TYPE_BOOL,
    TOKEN_TYPE_KEYWORD
};

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
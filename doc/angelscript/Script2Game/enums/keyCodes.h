
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
* Binding of RoR::keyCodes - used with Script2Game::InputEngineClass.
*/
enum keyCodes
{
    // PLEASE maintain the same order as in 'InputEngine.h' and 'InputEngineAngelscript.cpp'

    // Numpad
    KC_NUMPAD1,
    KC_NUMPAD2,
    KC_NUMPAD3,
    KC_NUMPAD4,
    KC_NUMPAD5,
    KC_NUMPAD6,
    KC_NUMPAD7,
    KC_NUMPAD8,
    KC_NUMPAD9,
    KC_NUMPAD0,

    // Number keys (not the numpad)
    KC_1,
    KC_2,
    KC_3,
    KC_4,
    KC_5,
    KC_6,
    KC_7,
    KC_8,
    KC_9,
    KC_0,

    // Function keys
    KC_F1 ,
    KC_F2 ,
    KC_F3 ,
    KC_F4 ,
    KC_F5 ,
    KC_F6 ,
    KC_F7 ,
    KC_F8 ,
    KC_F9 ,
    KC_F10,
    KC_F11,
    KC_F12,

    // Edit keys
    KC_INSERT,
    KC_DELETE,
    KC_BACKSPACE,
    KC_CAPSLOCK,
    KC_NUMLOCK,
    KC_SCROLLLOCK,
    KC_TAB,

    // Navigation keys
    KC_ESCAPE,
    KC_RETURN,
    KC_LEFT  ,
    KC_RIGHT ,
    KC_HOME  ,
    KC_UP    ,
    KC_PGUP  ,
    KC_END   ,
    KC_DOWN  ,
    KC_PGDOWN,
    KC_PAUSE ,

    // Modifiers
    KC_LCTRL ,
    KC_RCTRL ,
    KC_LSHIFT,
    KC_RSHIFT,
    KC_LALT  ,
    KC_RALT  ,
    KC_LWIN  ,
    KC_RWIN  ,

    // Special characters
    KC_MINUS     ,
    KC_EQUALS    ,
    KC_LBRACKET  ,
    KC_RBRACKET  ,
    KC_SEMICOLON ,
    KC_APOSTROPHE,
    KC_GRAVE     ,
    KC_BACKSLASH ,
    KC_COMMA     ,
    KC_PERIOD    ,
    KC_SLASH     ,
    KC_MULTIPLY  ,
    KC_SPACE     ,
    KC_SUBTRACT  ,
    KC_ADD       ,
}

} // namespace Script2Game

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
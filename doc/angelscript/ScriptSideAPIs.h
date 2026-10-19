
/// @defgroup ScriptSideAPIs Script-side APIs
/// ^AngelScript^ All features provided to/offered by scripting.

/// @defgroup Game2Script Game-to-script
/// ^AngelScript^ Documents script functions (callbacks) invoked by the game.
/// @{

/// @brief Pseudo-namespace; it doesn't exist in code or script runtime, only in this documentation.
namespace Game2Script {}

/// @}

/// @defgroup Script2Game Script-to-game
/// ^AngelScript^ Documents game functions which scripts can invoke to control it.
/// @{
    
/// @brief Pseudo-namespace; it doesn't exist in code or script runtime, only in this documentation.
namespace Script2Game {}

/// @brief Pseudo-namespace; it doesn't exist in code or script runtime, only in this documentation.
namespace ScriptValueTypes {}

/// @brief OGRE-AngelScript bindings; Actually named `Ogre` in the scripts, just changed for docs to separate things; Each of these objects is a loose subset of it's OGRE counterpart, see [OGRE API Docs](https://ogrecave.github.io/ogre/api/1.11/).
namespace AngelOgre {}    

/// @brief DearIMGUI-AngelScript bindings; Actually named `ImGui` in the scripts, just changed for docs to separate things; Note we use somewhat outdated version (v1.73) with hand-rolled bindings and some source tweaks.
namespace AngelImGui {}    

/// @}

/// @defgroup Script2Script Script-to-script
/// ^AngelScript^ Documents built-in scripts of the game.
/// @{
    
/// @brief Pseudo-namespace; it doesn't exist in code or script runtime, only in this documentation.
namespace Script2Script {}

/// @}

/// @defgroup Server2Script Server-to-script
/// ^AngelScript^ Documents script functions (callbacks) invoked by the server.
/// You can register them either by using a global function of the same name or by manually invoking [`server.setCallback()`](@ref Script2Server::ServerScriptClass::setCallback).
/// Note that some callbacks allow multiple handler functions ([`server.setCallback()`](@ref Script2Server::ServerScriptClass::setCallback) adds another) 
/// while other can only have a single handler function ([`server.setCallback()`](@ref Script2Server::ServerScriptClass::setCallback) replaces the previous). 
/// @{

/// @brief Pseudo-namespace; it doesn't exist in code or script runtime, only in this documentation.
namespace Server2Script {}

/// @}

/// @defgroup Script2Server Script-to-Server
/// ^AngelScript^ Documents server functions which scripts can invoke to control it.
/// @{
    
/// @brief Pseudo-namespace; it doesn't exist in code or script runtime, only in this documentation.
namespace Script2Server {}

/// @}

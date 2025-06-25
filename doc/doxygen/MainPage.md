Rigs of Rods Scripting (and Developer) Manual                         {#mainpage}
=============================================

Welcome to the developer portal of Rigs of Rods!
Rigs of Rods is a flexible physics simulation environment, supporting a multitude of mods (terrains, vehicles and loads, addon parts, skins, HUDs, gadgets) and offering extensive scripting capabilities using the [AngelScript](https://www.angelcode.com/angelscript/) language.
This documentation covers scripting features of both the game (client) and the server, from friendly introduction pages to full technical reference.
It also covers the full game codebase (written in C++) language, as it very closely matches the scripting realm and can help gaining deeper understanding.
To ask questions and suggest extensions, visit our [Discord #development](https://discord.com/channels/136544456244461568/189904947649708032) channel.


> **Quick links** (for regulars)
> * Script doc modules: [ScriptrSideAPIs](@ref ScriptSideAPIs) | [Script2Game](@ref Script2Game), [Game2Script](@ref Game2Script), [Script2Script](@ref Script2Script) |  @ref Script2Server @ref Server2Script
> * Client global objects: [`game`](@ref Script2Game::GameScriptClass), [`inputs`](@ref Script2Game::InputEngineClass), [`modcache`](@ref Script2Game::CacheSystemClass), [`console`](@ref Script2Game::CacheSystemClass)
> * Client principal enums: [`scriptEvents`](@ref Script2Game::scriptEvents), [`MsgType`](@ref Script2Game::MsgType)
> * Server global object: [`server`](@ref Script2Server::ServerScriptClass)
> * Server principal enums: [`broadcastType`](@ref Script2Server::broadcastType), [`authType`](@ref Script2Server::authType), [`serverSayType`](@ref Script2Server::authType)


Scripting introduction
----------------------

The single best way to get familiar is with the bundled **(in-game) Script editor gadget**. 
To open it, use top menubar (hover mouse near top to see it), menu 'Tools', item 'Browse gadgets ...' and in the selection window find 'Script editor'.
It always opens with pre-made script Tutorial.as - you can simply hit the RUN button to see it in action, and then try making some edits and see what happens.
![image](https://github.com/user-attachments/assets/dd537685-39ba-4c9e-9d45-3ac03816cda8)

To get familiar with the [AngelScript](https://www.angelcode.com/angelscript/) language, visit it's [reference manual on AngelCode.com](https://www.angelcode.com/angelscript/sdk/docs/manual/doc_script.html).


Scripting features
------------------

For a friendly introduction, read [Scripting features overview](@ref ScriptingFeaturesOverviewPage). Note  Below are pages highlighting specific topics:

* [Actor Sim Attributes (live simulation tinkering)](@ref ActorSimAttributesPage)
* [FreeBeams (ad-hoc physics elements)](@ref FreeBeamsPage)
* [FreeForces (ad-hoc physics actuators)](@ref FreeForcesPage)
* [Generic File Format (unified parser for any game file)](@ref GenericFileFormatPage)
* [OGRE 3D renderer bindings (direct access to visuals)](@ref Ogre3DBindingsPage)
* [Procedural roads (in-game editable geometry)](@ref ProceduralRoadsPage)
* [Gadget system (self-contained script mods)](@ref GadgetSystemPage)


Scripting reference manual
--------------------------

The [Script Side APIs](@ref ScriptSideAPIs) Doxygen page covers both the game (client) and server, each with distinct features.
Both are also documented from "both angles", meaning they describe both the API available to scripts and the callback functions invoked by the applications.
Finally, also game subsystems implemented as scripts (race system) are included.
To help user navigate all these topics the reference is split into categories 'Who2Who', where Who is Server/Game/Script.

Additionally there is [Server scripting reference on GitHub Wiki](https://github.com/RigsOfRods/ror-server/wiki/Scripting-reference) which nicely sums up the API on one page.
See also [Server example script on GitHub repo](https://github.com/RigsOfRods/ror-server/blob/master/contrib/example-script.as).



Game developer resources
------------------------

The source is [hosted on GitHub](https://github.com/RigsOfRods).

To report bugs, go to [rigs-of-rods Issue tracker](https://github.com/RigsOfRods/rigs-of-rods/issues).

Please see [rigs-of-rods Wiki](https://github.com/RigsOfRods/rigs-of-rods/wiki) for developer setup & building guides.

For introduction to the game development, see [codebase overview](@ref CodebaseOverviewPage) page.

For technical reference, use the 'Topics' link on the top of this page - it shows the entire codebase separated to categories (loosely matching the source tree).





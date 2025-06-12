AngelScript: Features overview                   {#ScriptingFeaturesOverviewPage}
==============================

[TOC]

The scripting subsystem is robust and extensive.
You're allowed to access all the configuration and game state, observe and change the simulation, monitor the simulated objects in detail and make changes on the go.
You can download pre-made 'gadgets', create your own script files or just run snippets in the console or via network.

Quick note: By convention, script class names have 'Class' suffix and live in global namespace - the namespace 'Script2Game' is dummy, used just in this doc to separate AngelScript/C++.


High-level capabilities
-----------------------

There are several ways of interacting with the game, each with it's own powers and limitations.
Each method can perform multitude of different tasks that are detailed in further sections.

### Read/write console variables

These represent settings in RoR.cfg, most of them take effect when changed live.
Use global variable [`console`](@ref Script2Game::ConsoleClass) to create/set/delete [cvars](@ref Script2Game::CVarClass) by name.
Recognized cvars can be found [on GitHub Wiki](https://github.com/RigsOfRods/rigs-of-rods/wiki/CVars-(console-variables)), an authoritaive list is in [CVar.cpp](CVar.cpp)

### Browse/search/add/remove installed mods

The game supports many kinds of downloadable contents: maps, vehicles, skins, dashboard HUDs, addonparts, gadgets.
For an overview, see [enum LoaderType](@ref Script2Game::LoaderType) which serves both as search filter and operation mode of RoR::GUI::MainSelector aka 'Loader UI'.
Each mod is represented by Script2Game::CacheEntryClass managed by global object [`modcache`](@ref Script2Game::CacheSystemClass).
To retrieve mods, use [`modcache.findEntryByFilename(...)`](@ref Script2Game::CacheSystemClass::findEntryByFilename)
    or  [`modcache.query({...})`](Script2Game::CacheSystemClass::query).

### Push requests to message queue

The game's main message loop synchronizes any state changes to keep the program stable.
These include app state (menu/sim/mulitiplayer), gameplay (spawn/modify actors, apply free forces), editing (roads/terrain objects/vehicle tuning).
Available requests and their parameters: Script2Game::MsgType; submit them using [`game.pushMessage()`](Script2Game::GameScriptClass::pushMessage)

### Receive game events

Game sends events on various occasions (mostly gameplay/simulation things but also responses to messages), see Script2Game::scriptEvents.
To start receiving an event, use [game.registerForEvent(EV_***)](@ref GameScriptClass::registerForEvent).
Events are received via callback Game2Script::EventCallbackEx()
    - see the Script2Game::scriptEvents comments for parameters and links to additional event-specific enums.
    
### Read and simulate controller inputs

All inputs are managed via global object [`inputs`](@ref Script2Game::InputEngineClass).
Game input events are enumerated in Script2Game::inputEvents, use [`inputs.getEventBoolValueBounce(EV_***)`](@ref Script2Game::InputEngineClass::getEventBoolValueBounce) to query event state (toggle-type).
You can also query directly for individual keys Script2Game::keyCodes with [`inputs.isKeyDownValueBounce(KC_***)`](@ref Script2Game::InputEngineClass::isKeyDownValueBounce ) - useful for modifier keys.
To inspect controller bindings for events, use [`inputs.getEventCommandTrimmed(EV_***)`](@ref Script2Game::InputEngineClass::getEventCommandTrimmed).
Finally you can simulate controller inputs using [`inputs.setEventSimulatedValue(EV_***, float)`](@ref Script2Game::InputEngineClass::setEventSimulatedValue)
    - remember this value is persistent, you need to also reset it!

### Live-adjust simulation params (experimental)

The game allows you to read/modify some live simulation attributes of the vehicles (actors).
This system is called [ActorSimAttributes](ActorSimAttributes.md) and it's meant to be expanded in the future.
See available settings in Game2Script::ActorSimAttr enum.
Use Script2Game::BeamClass::getSimAttribute() and Script2Game::BeamClass::setSimAttribute().


Gameplay and simulation features
--------------------------------

### Add and manage actors (vehicles / loads)

Note that all actors (even non-driven ones) are mostly referred as 'trucks' for historical reasons.
To spawn an actor, submit Script2Game::MSG_SIM_ACTOR_SPAWN_REQUESTED using [`game.pushMessage()`](@ref Script2Game::GameScriptClass::pushMessage()) - see 'Message queue' section above.
To inspect existing actors, use Script2Game::GameScriptClass::getAllTrucks() (type `game.getAllTrucks()`)
    or Script2Game::GameScriptClass::getCurrentTruck() to get your currently driven vehicle.
    
### Link (hook/tie/rope) actors together

These actions can be done by user (i.e. EV_COMMON_LOCK, EV_COMMON_SECURE_LOAD) or triggered automatically if configured so.
To do it directly, submit Script2Game::MSG_SIM_ACTOR_LINKING_REQUESTED using [`game.pushMessage()`](@ref Script2Game::GameScriptClass::pushMessage()) - see 'Message queue' section above.

### Enter/exit vehicles and move character

To enter or exit vehicles, submit Script2Game::MSG_SIM_SEAT_PLAYER_REQUESTED using [`game.pushMessage()`](@ref Script2Game::GameScriptClass::pushMessage()) - see 'Message queue' section above.
You can move the character anywhere on the map using MSG_SIM_TELEPORT_PLAYER_REQUESTED.
To walk the character, you can simulate controller inputs using [`inputs.setEventSimulatedValue()`](@ref Script2Game::InputEngineClass::setEventSimulatedValue),
    see EV_CHARACTER_FORWARD, EV_CHARACTER_LEFT etc...
    
### Set up and manage FreeForces

[FreeForces](FreeForces.md) are persistent (they don't fade out until you modify them) effects which apply force to one node each.
To create one, submit Script2Game::MSG_SIM_ADD_FREEFORCE_REQUESTED using [`game.pushMessage()`](@ref Script2Game::GameScriptClass::pushMessage());
    available types are Script2Game::FreeForceTypes.

### Create ad-hoc beams between any nodes (even between actors)

This is an extension of the FreeForces system described above, called [FreeBeams](FreeBeams.md).
These feel exactly the same as 'beams' in the rig-def fileformat (and can also look the same if you choose).


Graphics and GUI/HUD features
-----------------------------

### Dynamically create GUI using DearIMGUI

This [popular GUI and 2D drawing toolkit](https://github.com/ocornut/imgui) is used exactly as from C++ (`ImGui::` functions).
To separate things in the documentation, the namespace is spelled as AngelImGui.
Note we use somewhat outdated version (v1.73) with some minor source tweaks.

### Draw freely on screen using DearIMGUI

DearIMGUI's free 2D drawing is always confined to it's parent window, but we provide a hack:
`#include <imgui_utils.as>`, call `imgui_utils::ImGetDummyFullscreenWindow()` and use the retrieved Script2Game::ImDrawList object to draw anything anywhere.
To paint with mouse, use [`game.getMouseScreenPosition()`](@ref Script2Game::GameScriptClass::getMouseScreenPosition).
To draw around objects in scene, use [`game.getScreenPosFromWorldPos()`](@ref Script2Game::GameScriptClass::getScreenPosFromWorldPos)
or [`game.getMouseTerrainPosition()`](@ref Script2Game::GameScriptClass::getMouseTerrainPosition).

### Access the underlying 3D renderer (OGRE)

[OGRE](http://www.ogre3d.org) is a robust 3D renderer with a built in resource management.
The bindings are made to best mimic [it's C++ API](https://ogrecave.github.io/ogre/api/1.11/).
You can traverse the scene graph, add/move/show&hide objects, control skeletal animation, even create custom meshes and manipulate textures!
For details see [OGRE bindings](Ogre3DBindings.md) page.
Note that to distinguish C++ from AngelScript in this documentation, the namespace is spelled AngelOgre.


Resource discovery and manipulation
-----------------------------------

The game uses [OGRE's resource management](https://ogrecave.github.io/ogre/api/latest/_resource-_management.html) for all file access. 
It's important to know OGRE doesn't access disk files directly; instead you need to set up 'Resource groups' which may be one or more disk location together, including ZIP archives, and optionally with a filter applied.
This system is robust as it allows tracking and reloading data belonging to various elements (actors, terrains, gadgets...).
Resource groups are set up automatically by the game, you cannot browse disk freely.

### Locate resources

To list all resources (filenames) matching a pattern, use [`game.findResourceFileInfo()`](@ref Script2Game::GameScriptClass::findResourceFileInfo)
which is a proxy to [`Ogre::ResourceGroupManager::findResourceFileInfo()`](https://ogrecave.github.io/ogre/api/1.11/class_ogre_1_1_resource_group_manager.html#a662f68163310401718d3c3981a7baec4).

To quickly check if resource exists, use [`game.checkResourceExists`](@ref Script2Game::GameScriptClass::checkResourceExists).

### Working with raw text files

Most likely you just want the text - [`game.loadTextResourceAsString`](@ref Script2Game::GameScriptClass::loadTextResourceAsString) is your friend.
It's counterpart to create/overwrite the file is [`game.createTextResourceFromString`](@ref Script2Game::GameScriptClass::createTextResourceFromString).

### Using INI-like format '.asdata'

This is a simple 'Name=Value' pair file, optionally divided to sections. To load it, create an Game2Script::LocalStorageClass object using it's constructor and specifying the file name.
Then use functions `get***()` and `set***()` as needed and finally save using `save()`, or discard changes using `reload()`.
The file will be saved in the 'Rigs of Rods/cache' directory in your user profile.

### Parsing game file formats

We have multiple formats with very similar syntax: rig def (truck), odef, tobj, race, addonpart, dashboard.
To be able to productively read and modify them from script, a [generic unified tokenizer](GenericFileFormat.md) was created.

Script management and diagnostics
---------------------------------

The game can run multiple scripts at once. Running scripts are called 'script units' and are identified by RoR::ScriptUnitID_t, abbreviated 'NID'.
Running scripts can be viewed/stopped/reloaded in the Console window (menu 'Script monitor').
Scripts can be run from multiple sources in multiple ways:
* In-game console snippets: command 'as' followed by code will execute any snippet.
* Snippets from server, sent using [`server.cmd()`](Script2Server::ServerScriptClass::cmd) within the script running on rorserver.
* Plain '*.as' files in user profile 'Rigs of Rods/scripts': these can be invoked on system command line as 'RoR -runscript foo.as', through RoR.cfg via cvar 'app_custom_scripts' (a comma-separated list, spaces in filenames are acceptable) or through in-game console using 'loadscript' command.
* Snippets generated by another script (string in memory), see below.
* Bundled '.as' files in mods: terrains, actors, races and gadgets can have script configured with them.

### Launch/stop/analyze scripts from script

Using MSG_APP_LOAD_SCRIPT_REQUESTED, you can run a script from file or a string in memory.
To get the details about running script, use [`game.getScriptDetails()`](Script2Game::GameScriptClass::getScriptDetails).
Each script has automatically created global variable `thisScript` which is it's own NID. To get NIDs of other scripts, use [`game.getRunningScripts()`](Script2Game::GameScriptClass::getRunningScripts).

### Debug scripts from script

All messages from AngelScript engine are forwarded to scripts it via [`eventCallbackEx()`](Game2Script::eventCallbackEx) with the following events:
```
    SE_ANGELSCRIPT_MSGCALLBACK,         //!< (see `asSMessageInfo`), args: #1 ScriptUnitID, #2 asEMsgType, #3 row, #4 col, #5 sectionName, #6 message
    SE_ANGELSCRIPT_LINECALLBACK,        //!< (see `SetLineCallback()`), args: #1 ScriptUnitID, #2 LineNumber, #3 CallstackSize, #4 unused, #5 FunctionName, #6 FunctionObjectTypeName #7 ObjectName
    SE_ANGELSCRIPT_EXCEPTIONCALLBACK,   //!< (see `SetExceptionCallback()`), args: #1 ScriptUnitID, #2 unused, #3 row (`GetExceptionLineNumber()`), #4 unused, #5 funcName, #6 message (`GetExceptionString()`)
```

Because AngelScript cannot propagate C++ exceptions, they must be caught within C++ functions. To let script programmer know this happened, there is a special event:
```
    SE_GENERIC_EXCEPTION_CAUGHT,        //!< Triggered when C++ exception (usually Ogre::Exception) is thrown; #1 ScriptUnitID, #5 originFuncName, #6 type, #7 message.
```




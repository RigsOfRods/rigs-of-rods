/*!
 * @page overview Codebase overview
 *
 * # The application layer
 * 
 * Rigs of Rods is a monolithic C++ program with it's own main input/rendering loop.
 * The entry point is a standard main() function which performs all initialization, message processing and cleanup.
 * 
 * ## Input handling
 * Inputs are received via OIS library's listener mechanism.
 * Class RoR::AppContext is the sole listener for all inputs and serves as a dispatcher between various subsystems.
 * Gameplay inputs are defined by RoR::events and handled by RoR::InputEngine, which reads configuration files '*.map', the default being 'input.map'.
 *
 * ## Video output
 * Graphical output is done via OGRE 3D rendering engine (not to be confused with game engine). Startup is done by RoR::AppContext::SetUpRendering().
 * Window events are handled by RoR::AppContext via OGRE's OgreBites::WindowEventListener. Note Ogre::FrameListener interface is not used, we roll our own rendering loop in main().
 * Rendering is configured by OGRE's own 'ogre.cfg' file.
 * 
 * ## Audio output
 * Sound is done using OpenAL Soft library, managed by RoR::SoundManager.
 * Individual sound effects are defined as 'sound scripts'. User creates <a href="https://docs.rigsofrods.org/vehicle-creation/fileformat-soundscript/">soundscript</a> files and RoR::SoundScriptManager parses them into individual objects RoR::SoundScriptTemplate (one per soundscript) and RoR::SoundScriptInstance (one per actor which uses it). Sound script 'trigger sources' are specified by RoR::SoundTriggers, 'pitch/gain sources' by RoR::ModulationSources.
 * During simulation, sound script instances are updated using RoR::SoundScriptManager macros like SOUND_START(), SOUND_STOP() and SOUND_MODULATE().
 *
 * ## Filesystem handling
 * All file manipulations are done using OGRE's resource system, which has many useful features:
 * * Transparently reads ZIP archives as if they were just directories.
 * * Organizes resources into 'resource groups' (alias RGs). Each can have multiple 'resource locations' (ZIP archives or directories or both).
 * * Loads resources automatically as needed.
 * * For details visit <a href="https://ogrecave.github.io/ogre/api/latest/_resource-_management.html">Resource Management</a> in OGRE manual.
 *
 * Initial configuration is done by RoR::AppContext::SetUpProgramPaths() and RoR::AppContext::SetUpResourcesDir().
 * Reading and writing files is done mostly using `Ogre::ResourceGroupManager().getSingleton().openResource()`, eventually using helper functions in PlatformUtils.h.
 *
 * # Gameplay concepts
 *
 * Rigs of Rods is a sandbox physics simulator with static terrain, scriptable events, free-moving player characters and deformable physics objects.
 *
 * The main interface to gameplay logic is RoR::GameContext. It maintains the simulation state, loads/updates/unloads individual elements and resolves their interactions.
 *
 * ## Message queue
 * A global message queue provides a simple and reliable method of announcing or requesting any change in the simulation. Messages are processed always on the main thread, one by one, so you cannot break the simulation state by doing something "at a wrong time". Message queue was added in 2020, unifying several pre-existing queues and event systems.
 * Message queue lives in RoR::GameContext. To add message, call RoR::GameContext::PushMessage() (write: `App::GetGameContext()->PushMessage()`). See RoR::MsgType for list of available messages. Currently, messages can be only submitted from code. In the future, it will be possible from scripting as well.
 * Messages are processed exclusively in main(). In the future it will be possible to register for messages from scripting.
 *
 * ## Console
 * The console receives text messages from all areas of the game; see RoR::Console::MessageType and RoR::Console::MessageArea. To submit a message, call RoR::Console::putMessage() (write: `App::GetConsole()->putMessage()`).
 * To view the messages in-game, use console window (GUI::ConsoleWindow) or on-screen chat display (GUI::GameChatBox). Both use GUI::ConsoleView to retrieve and filter the messages. All messages are also written to RoR.log.
 * 
 * ### Console variables (CVars)
 * This concept is borrowed from Quake engine. CVars configure every aspect of the program; see source of RoR::Console::CVarSetupBuiltins() for complete listing and partial doc on <a href="https://github.com/RigsOfRods/rigs-of-rods/wiki/CVars-(console-variables)"> github wiki </a>.
 * RoR::Console reads main configuration file 'RoR.cfg' and loads known values to cvars, see RoR::Console::LoadConfig(). Command-line arguments are also stored as cvars, see RoR::Console::ProcessCommandLine().
 *
 * ## ModCache
 * A database of all user-made content (mods). This includes terrains, trucks (.truck/load/etc.., see fileformat truck) and skins.
 * ModCache is maintained by object RoR::CacheSystem (use `App::GetCacheSystem()`). The process of (re)building the database is called 'cache regen' or the like. It's done by RoR::CacheSystem::LoadModCache().
 *
 * Mods are distributed in zip archives, traditionally with extensions .zip (terrain/truck/skin) or .skinzip (skin). Loading from directiories is also possible. These locations are reffered to as 'bundles'. Each bundle may contain several mods of mixed types. Bundles are shared between mods and loaded only once, each gets it's own OGRE resource group named "bundle $FULLPATH$". To load a bundle, use RoR::CacheSystem::LoadResource(), or helper RoR::CacheSystem::CheckResourceLoaded().
 * Mods are located under 'Documents\My Games\Rigs of Rods\mods', cached files are in 'Documents\My Games\Rigs of Rods\cache' (see cvar 'sys_cache_dir').
 * For each mod, a RoR::CacheEntry is created. To retrieve cache entries from modcache, use RoR::CacheSystem::Query() with RoR::CacheQuery, or RoR::CacheSystem::FindEntryByFilename(), or RoR::CacheSystem::FetchSkinByName().
 *
 * 
 * 
 */

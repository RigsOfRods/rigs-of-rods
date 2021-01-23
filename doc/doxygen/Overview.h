/*!
 * @page Codebase overview
 *
 * # The application
 * 
 * Rigs of Rods is a monolithic C++ program with it's own main input/rendering loop.
 * The entry point is a standard main() function which performs all initialization, message processing and cleanup.
 * 
 * # Input handling
 * Inputs are received via OIS library's listener mechanism.
 * Class RoR::AppContext is the sole listener for all inputs and serves as a dispatcher between various subsystems.
 * Gameplay inputs are defined by enum events and handled by InputEngine. RoR::InputEngine also reads input configuration files '*.map', the default being 'input.map'.
 *
 * # Rendering
 * Graphical output is done via OGRE 3D rendering engine (not to be confused with game engine).
 * Startup is done by RoR::AppContext::SetUpRendering().
 * Window events are handled by RoR::AppContext via OGRE's OgreBites::WindowEventListener. Note Ogre::FrameListener interface is not used, we roll our own rendering loop in main().
 * 
 * 
 *
 *
 */

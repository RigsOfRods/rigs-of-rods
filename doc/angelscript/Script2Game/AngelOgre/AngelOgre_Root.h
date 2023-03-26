
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */  

    /** a singleton - use `Ogre::Root::getSingleton()` to retrieve instance.
    */
    class  Root
    {
    public:
        /** @return a readonly dictionary view of all scene manager instances in the application.
        */
        SceneManagerInstanceDict@ getSceneManagers();
    };
    
/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
    
} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)



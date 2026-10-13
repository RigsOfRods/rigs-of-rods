
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */

/** \addtogroup Script2Game
 *  @{
 */

    /** a singleton - use `Ogre::MaterialManager::getSingleton()` to retrieve instance.
        Errors are forwarded as `SE_GENERIC_EXCEPTION_CAUGHT` script events.
    */
    class MaterialManager
    {
    public:
        /** Finds a material by name.
        @param name Material name.
        @param rg Resource group.
        @return The material, or a null pointer if not found.
        */
        MaterialPtr getByName(const string&in name, const string&in rg);

        /** Creates a new, empty material.
        */
        MaterialPtr create(const string&in name, const string&in rg);
    };

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)


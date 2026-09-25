
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */

/** \addtogroup Script2Game
 *  @{
 */

    /** a singleton - use `Ogre::MeshManager::getSingleton()` to retrieve instance.
        Errors are forwarded as `SE_GENERIC_EXCEPTION_CAUGHT` script events.
    */
    class MeshManager
    {
    public:
        /** Loads a mesh (or returns it if already loaded).
        @param file Mesh file name, e.g. "sphere.mesh".
        @param rg Resource group.
        @return The mesh, or a null pointer on error.
        */
        MeshPtr load(const string&in file, const string&in rg);

        /// Unloads the mesh and removes it from the manager.
        void remove(const string&in file, const string&in rg);
    };

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)


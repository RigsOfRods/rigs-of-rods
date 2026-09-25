
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */

/** \addtogroup Script2Game
 *  @{
 */

    /** A shared pointer to a mesh resource. Binding of `Ogre::MeshPtr`.
        @remarks
            Obtain one with `MeshManager::load()`, `Entity::getMesh()` or `ManualObject::convertToMesh()`.
            A mesh consists of one or more `SubMesh`es, each with its own material.
    */
    class MeshPtr
    {
    public:

        /// Creates an empty (null) pointer.
        MeshPtr();

        /// Creates a copy pointing to the same mesh.
        MeshPtr(const MeshPtr&in other);

        MeshPtr& opAssign(const MeshPtr&in other);

        /// Returns true if the pointer doesn't point to a mesh.
        bool isNull();

        /// Returns a read-only list of the mesh's submeshes.
        SubMeshArray@ getSubMeshes();

        /// Name of the mesh resource.
        string getName();

        /// Creates a new, empty submesh with the given name.
        SubMesh@ createSubMesh(const string&in name);

        /** Destroys the submesh with the given name.
        */
        void destroySubMesh(const string&in name);
    };

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)


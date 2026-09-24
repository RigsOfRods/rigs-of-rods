
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */

/** \addtogroup Script2Game
 *  @{
 */

    /** A shared pointer to a material. Binding of `Ogre::MaterialPtr`.
        @remarks
            Obtain one with `MaterialManager::getByName()` or `SubEntity::getMaterial()`.
            A material consists of `Technique`s, each consisting of `Pass`es.
    */
    class MaterialPtr
    {
    public:

        /// Creates an empty (null) pointer.
        MaterialPtr();

        /// Creates a copy pointing to the same material.
        MaterialPtr(const MaterialPtr&in other);

        MaterialPtr& opAssign(const MaterialPtr&in other);

        /// Returns true if the pointer doesn't point to a material.
        bool isNull();

        /// Returns a read-only list of the material's techniques.
        TechniqueArray@ getTechniques();

        /// Name of the material.
        string getName();

        /** Creates a new, empty technique.
        @return The technique, or null on error (forwarded as `SE_GENERIC_EXCEPTION_CAUGHT` script event).
        */
        Technique@ createTechnique();

        /// Removes the technique at the given index.
        void removeTechnique(uint16 index);
    };

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)


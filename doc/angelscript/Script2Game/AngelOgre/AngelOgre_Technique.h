
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */

/** \addtogroup Script2Game
 *  @{
 */

    /** One way of rendering a material; a material may have several (e.g. for different hardware or LOD). Binding of `Ogre::Technique`.
        @remarks Obtain one with `MaterialPtr::getTechniques()` or `MaterialPtr::createTechnique()`.
    */
    class Technique
    {
    public:
        /// Returns a read-only list of the technique's passes.
        PassArray@ getPasses();

        /// Creates a new pass and adds it to the end of the list.
        Pass@ createPass();

        /// Removes the pass at the given index.
        void removePass(uint16 index);

        /// Name of the technique (may be empty).
        const string& getName() const;
    };

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)



namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */

/** \addtogroup Script2Game
 *  @{
 */

    /** A texture layer of a `Pass`. Binding of `Ogre::TextureUnitState`.
        @remarks Obtain one with `Pass::getTextureUnitStates()`.
    */
    class TextureUnitState
    {
    public:
        /// Name of the texture unit (may be empty).
        const string& getName() const;

        /// Sets the texture used by this unit.
        void setTexture(const TexturePtr&in tex);

        /// Returns the texture used by this unit (loading it if needed).
        const TexturePtr& _getTexturePtr() const;
    };

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)


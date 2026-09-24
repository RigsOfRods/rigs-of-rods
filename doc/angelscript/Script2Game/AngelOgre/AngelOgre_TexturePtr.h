
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */  

    /** A shared pointer to a texture. Binding of `Ogre::TexturePtr`.
        @remarks Obtain one with `TextureManager::load()` or `TextureUnitState::_getTexturePtr()`.
    */
    class  TexturePtr
    {
    public:
        /// Creates an empty (null) pointer.
        TexturePtr();

        /// Creates a copy pointing to the same texture.
        TexturePtr(const TexturePtr&in other);

        TexturePtr& opAssign(const TexturePtr&in other);

        /// Returns true if the pointer doesn't point to a texture.
        bool isNull();

        /// Name of the texture resource.
        string getName() const;

        uint getWidth();
        uint getHeight();
        uint getNumMipmaps();

        /// Returns the pixel buffer of the given face and mipmap level, see `HardwarePixelBufferPtr`.
        HardwarePixelBufferPtr getBuffer(uint face, uint mipmap);
    };
    
/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
    
} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)



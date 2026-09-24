
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */

/** \addtogroup Script2Game
 *  @{
 */

    /** An image in system memory, which can hold multiple faces (cubemaps) and mipmap levels. Binding of `Ogre::Image`.
        @remarks
            Load one with `GameScriptClass::loadImageResource()`.
            To display it, copy it into a texture with `HardwarePixelBufferPtr::blitFromMemory()` using `getPixelBox()`.
    */
    class Image
    {
    public:

        /// Creates an empty image.
        Image();

        /// Creates a copy of the image, including pixel data.
        Image(const Image&in other);

        Image& opAssign(const Image&in other);

        /** Gets the colour of the pixel at the given position.
        @param z Slice; use 0 for 2D images.
        */
        color getColourAt(uint32 x, uint32 y, uint32 z);

        /** Sets the colour of the pixel at the given position, converting it to the image's pixel format.
        @param z Slice; use 0 for 2D images.
        */
        void setColourAt(const color& c, uint32 x, uint32 y, uint32 z);

        /// Flips (mirrors) the image around the X axis (upside down), in place.
        Image& flipAroundX();

        /// Flips (mirrors) the image around the Y axis (left to right), in place.
        Image& flipAroundY();

        /// Number of mipmaps, excluding the base level.
        uint getNumMipmaps();

        /// Number of faces - 6 for cubemaps, 1 otherwise.
        uint getNumFaces();

        /** Returns a `PixelBox` pointing into the image's memory.
        @param face Face index; 0 unless it's a cubemap.
        @param mipmap Mipmap level; 0 is the full-size image.
        @note The PixelBox is only valid while the image exists.
        */
        PixelBox getPixelBox(uint face, uint mipmap);

        /// Size of the image data, in bytes.
        uint getSize();

        /// Width of the image, in pixels.
        uint getWidth();

        /// Height of the image, in pixels.
        uint getHeight();

        /** Resizes the image in place, resampling the pixels.
        @param filter Resampling method, see `ImageFilter`.
        */
        void resize(uint16 width, uint16 height, ImageFilter filter);
    };

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)


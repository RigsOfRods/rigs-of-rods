
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */  

    /** A box of pixels in memory, with a known pixel format. Binding of `Ogre::PixelBox`.
        @remarks
            You get one from `HardwarePixelBufferPtr::lock()` (it then points directly into the locked buffer)
            or from `Image::getPixelBox()`. It can be passed to `HardwarePixelBufferPtr::blitFromMemory()`
            and `HardwarePixelBufferPtr::blitToMemory()`.
        @note A PixelBox does not own its memory - it's only valid while the buffer or image it came from exists
            (and, for a locked buffer, until `unlock()` is called).
    */
    class PixelBox
    {
    public:

        /// Creates an empty PixelBox which points to no memory.
        PixelBox();

        /// Creates a copy referencing the same memory.
        PixelBox(const PixelBox&in other);

        PixelBox& opAssign(const PixelBox&in other);

        /** Gets the colour of the pixel at the given position.
        @param x Column, relative to the box's left edge.
        @param y Row, relative to the box's top edge.
        @param z Slice, relative to the box's front edge (use 0 for 2D images).
        */
        color getColourAt(uint32 x, uint32 y, uint32 z);

        /** Sets the colour of the pixel at the given position, converting it to the box's pixel format.
        @param c The colour.
        @param x Column, relative to the box's left edge.
        @param y Row, relative to the box's top edge.
        @param z Slice, relative to the box's front edge (use 0 for 2D images).
        */
        void setColourAt(const color& c, uint32 x, uint32 y, uint32 z);

        /// Width of the box, in pixels.
        uint getWidth();

        /// Height of the box, in pixels.
        uint getHeight();

        /// Depth of the box, in pixels (1 for 2D images).
        uint getDepth();
    };
    
/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
    
} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)


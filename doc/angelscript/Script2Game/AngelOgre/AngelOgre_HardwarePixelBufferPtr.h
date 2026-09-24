
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */

/** \addtogroup Script2Game
 *  @{
 */

    /** A shared pointer to a pixel buffer, which may be a texture surface (a single face and mipmap level)
        or a render target. Binding of `Ogre::HardwarePixelBufferSharedPtr`.
        @remarks
            Obtain one with `TexturePtr::getBuffer()`. Use `lock()`/`unlock()` to access the pixels directly,
            or `blitFromMemory()`/`blitToMemory()` to copy them from/to a `PixelBox`.
        @par
            All methods catch OGRE exceptions and forward them as `SE_GENERIC_EXCEPTION_CAUGHT` script events,
            so check the event if a call seems to do nothing.
    */
    class HardwarePixelBufferPtr
    {
    public:

        /// Creates an empty (null) pointer.
        HardwarePixelBufferPtr();

        /// Creates a copy pointing to the same buffer.
        HardwarePixelBufferPtr(const HardwarePixelBufferPtr&in other);

        HardwarePixelBufferPtr& opAssign(const HardwarePixelBufferPtr&in other);

        /** Locks a region of the buffer for reading/writing.
        @param lockbox The region to lock, in pixels.
        @param opt Locking options, see `HardwareBufferLockOptions`.
        @return A `PixelBox` describing the locked memory. Remains valid until `unlock()` is called.
            If locking fails, an empty `PixelBox` is returned.
        */
        const PixelBox& lock(const box& lockbox, HardwareBufferLockOptions opt);

        /** Returns the `PixelBox` of the currently locked region.
        @note Only valid between `lock()` and `unlock()`; otherwise an empty `PixelBox` is returned.
        */
        const PixelBox& getCurrentLock();

        /// Releases the lock obtained by `lock()`, uploading any modifications to the GPU.
        void unlock();

        /// Width of the buffer, in pixels.
        uint getWidth();

        /// Height of the buffer, in pixels.
        uint getHeight();

        /** Copies a region of memory into a region of this buffer.
        @param src The source pixels.
        @param dst The destination region within this buffer; if its size differs from `src`,
            the pixels are scaled.
        @note The buffer must not be locked.
        */
        void blitFromMemory(const PixelBox& src, const box& dst);

        /** Copies a region of this buffer into memory.
        @param src The source region within this buffer.
        @param dst The destination pixels; if its size differs from `src`, the pixels are scaled.
        @note The buffer must not be locked.
        */
        void blitToMemory(const box& src, const PixelBox& dst);
    };

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)


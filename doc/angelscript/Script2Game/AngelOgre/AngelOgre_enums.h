
  // =================================================== //
  // THIS IS NOT A C++ HEADER! Only a dummy for Doxygen. //
  // =================================================== //

namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */  
 
 // PLEASE maintain the same order as in 'OgreAngelscript.cpp'
 
     enum IndexType
     {
        IT_16BIT,
        IT_32BIT
    };
    
    /** Enumeration denoting the spaces which a transform can be relative to.
    */
    enum TransformSpace
    {
        /// Transform is relative to the local space
        TS_LOCAL,
        /// Transform is relative to the space of the parent node
        TS_PARENT,
        /// Transform is relative to world space
        TS_WORLD
    };    
    

    /// The rendering operation type to perform; binding of `Ogre::RenderOperation::OperationType`
    enum RenderOperation
    {
        /// A list of points, 1 vertex per point
        OT_POINT_LIST = 1,
        /// A list of lines, 2 vertices per line
        OT_LINE_LIST = 2,
        /// A strip of connected lines, 1 vertex per line plus 1 start vertex
        OT_LINE_STRIP = 3,
        /// A list of triangles, 3 vertices per triangle
        OT_TRIANGLE_LIST = 4,
        /// A strip of triangles, 3 vertices for the first triangle, and 1 per triangle after that
        OT_TRIANGLE_STRIP = 5,
        /// A fan of triangles, 3 vertices for the first triangle, and 1 per triangle after that
        OT_TRIANGLE_FAN = 6
    };      

    /// Binding of `enum Ogre::Image::Filter`
    enum ImageFilter
    {
        FILTER_NEAREST,
        FILTER_LINEAR,
        FILTER_BILINEAR
    };   

    /// Locking options (binding of `enum Ogre::HardwareBuffer::LockOptions`)
    enum HardwareBufferLockOptions
    {
        /** Normal mode, ie allows read/write and contents are preserved.
         This kind of lock allows reading and writing from the buffer - it’s also the least
         optimal because basically you’re telling the card you could be doing anything at
         all. If you’re not using a shadow buffer, it requires the buffer to be transferred
         from the card and back again. If you’re using a shadow buffer the effect is
         minimal.
         */
        HBL_NORMAL,
        /** Discards the <em>entire</em> buffer while locking.
        This means you are happy for the card to discard the entire current contents of the
        buffer. Implicitly this means you are not going to read the data - it also means
        that the card can avoid any stalls if the buffer is currently being rendered from,
        because it will actually give you an entirely different one. Use this wherever
        possible when you are locking a buffer which was not created with a shadow buffer.
        If you are using a shadow buffer it matters less, although with a shadow buffer it’s
        preferable to lock the entire buffer at once, because that allows the shadow buffer
        to use HBL_DISCARD when it uploads the updated contents to the real buffer.
        @note Only useful on buffers created with the HBU_CPU_TO_GPU flag.
        */
        HBL_DISCARD,
        /** Lock the buffer for reading only. Not allowed in buffers which are created with
        HBU_GPU_ONLY.
        Mandatory on static buffers, i.e. those created without the HBU_DYNAMIC flag.
        */
        HBL_READ_ONLY,
        /** As HBL_WRITE_ONLY, except the application guarantees not to overwrite any
        region of the buffer which has already been used in this frame, can allow
        some optimisation on some APIs.
        @note Only useful on buffers with no shadow buffer.*/
        HBL_NO_OVERWRITE,
        /** Lock the buffer for writing only.*/
        HBL_WRITE_ONLY

    };    
    
    /// Enumerates the types of light sources available.
    enum LightTypes
    {
        /// Point light sources give off light equally in all directions, so require only position not direction
        LT_POINT,
        /// Directional lights simulate parallel light beams from a distant source, hence have direction but no position
        LT_DIRECTIONAL,
        /// Spotlights simulate a cone of light from a source so require position and direction, plus extra values for falloff
        LT_SPOTLIGHT
    };    
        
    // Register the GuiMetricsMode enum
    enum GuiMetricsMode
    {
        GMM_PIXELS,
        GMM_RELATIVE,
        GMM_RELATIVE_ASPECT_ADJUSTED
    };

    // Register the GuiHorizontalAlignment enum
    enum GuiHorizontalAlignment
    {
        GHA_LEFT,
        GHA_CENTER,
        GHA_RIGHT
    };
    
/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
    
} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)



  // =================================================== //
  // THIS IS NOT A C++ HEADER! Only a dummy for Doxygen. //
  // =================================================== //

namespace ScriptValueTypes {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */    

/**
 * @brief Binding of Ogre::Box - a 3D box of pixels, used to specify a region of an image or pixel buffer.
 * @remarks The right, bottom and back edges are exclusive, i.e. width = right - left.
 * The 4-parameter constructor creates a 2D box (front = 0, back = 1).
 */
struct box
{
public:
    // properties
    uint left;
    uint top;
    uint right;
    uint bottom;
    uint front;
    uint back;
    
    ///@name Constructors
    /// @{
    box();
    box(uint32 l, uint32 t, uint32 r, uint32 b);
    box(const box &other);
    /// @}

    box& opAssign(const box&in);
}

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace ScriptValueTypes

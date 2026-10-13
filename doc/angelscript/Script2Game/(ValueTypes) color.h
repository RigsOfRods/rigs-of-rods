
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
 * @brief Binding of Ogre::ColourValue
 */
struct color
{
public:
    // properties
    float r;
    float g;
    float b;
    float a;
    
    ///@name Constructors
    /// @{
    color();
    color(float r, float g, float b, float a);
    color(const color &in);
    /// @}
}

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace ScriptValueTypes


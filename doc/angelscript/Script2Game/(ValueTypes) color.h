
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
    color(float, float, float, float);
    color(const color &in);
    color();
    color(float);
    /// @}
}

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace ScriptValueTypes


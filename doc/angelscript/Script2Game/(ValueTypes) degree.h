
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
 * @brief Binding of Ogre::Degree
 */
struct degree
{
public:

    ///@name Constructors
    /// @{
    degree();
    degree(float);
    degree(const degree &in);
    /// @}
    
    /// @name Operators
    /// @{
    degree &opAssign(const degree &in)
    degree &opAssign(const float)
    degree &opAssign(const degree &in)
    degree opAdd() const
    degree opAdd(const degree &in) const
    degree opAdd(const degree &in) const
    degree &opAddAssign(const degree &in)
    degree &opAddAssign(const degree &in)
    degree opSub() const
    degree opSub(const degree &in) const
    degree opSub(const degree &in) const
    degree &opSubAssign(const degree &in)
    degree &opSubAssign(const degree &in)
    degree opMul(float) const
    degree opMul(const degree &in) const
    degree &opMulAssign(float)
    degree opDiv(float) const
    degree &opDivAssign(float)
    int opCmp(const degree &in) const
    bool opEquals(const degree &in) const
    /// @}
    
    /// @name Methods
    /// @{
    float valueDegrees() const;
    float valuedegrees() const;
    float valueAngleUnits() const;
    /// @}
}

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace ScriptValueTypes



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
 * @brief Binding of Ogre::Radian
 */
struct radian
{
public:

    ///@name Constructors
    /// @{
    radian();
    radian(float);
    radian(const radian &in);
    /// @}
    
    /// @name Operators
    /// @{
    radian &opAssign(const radian &in)
    radian &opAssign(const float)
    radian &opAssign(const degree &in)
    radian opAdd() const
    radian opAdd(const radian &in) const
    radian opAdd(const degree &in) const
    radian &opAddAssign(const radian &in)
    radian &opAddAssign(const degree &in)
    radian opSub() const
    radian opSub(const radian &in) const
    radian opSub(const degree &in) const
    radian &opSubAssign(const radian &in)
    radian &opSubAssign(const degree &in)
    radian opMul(float) const
    radian opMul(const radian &in) const
    radian &opMulAssign(float)
    radian opDiv(float) const
    radian &opDivAssign(float)
    int opCmp(const radian &in) const
    bool opEquals(const radian &in) const
    /// @}
    
    /// @name Methods
    /// @{
    float valueDegrees() const;
    float valueRadians() const;
    float valueAngleUnits() const;
    /// @}
}

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace ScriptValueTypes


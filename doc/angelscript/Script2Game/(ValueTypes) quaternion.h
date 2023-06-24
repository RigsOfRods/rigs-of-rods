
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
 * @brief Binding of Ogre::Quaternion
 */
struct quaternion
{
public:
    // properties
    float w;
    float x;
    float y;
    float z;
    
    ///@name Constructors
    /// @{
    quaternion();
    quaternion(const radian &in, const vector3 &in);
    quaternion(float, float, float, float);
    quaternion(const quaternion &in);
    quaternion(float);
    /// @}
    
    /// @name Operators
    /// @{
    float opIndex(int) const;
    bool opEquals(const quaternion &in) const;
    quaternion opAdd(const quaternion &in) const;
    quaternion opSub(const quaternion &in) const;
    quaternion opMul(const vector &in) const;
    quaternion opSub() const;
    quaternion opMul(const quaternion &in) const;
    quaternion &opAssign(const quaternion &in);
    /// @}
    
    /// @name Methods
    /// @{
    float Dot(const quaternion &in) const;
    float Norm() const;
    float normalise();
    quaternion Inverse() const;
    quaternion UnitInverse() const;
    quaternion Exp() const;
    quaternion Log() const;
    radian getRoll(bool reprojectAxis = true) const;
    radian getPitch(bool reprojectAxis = true) const;
    radian getYaw(bool reprojectAxis = true) const;
    bool equals(const quaternion &in, const radian &in) const;
    bool isNaN() const;
    /// @}
}

// Ogre::Quaternion static methods as global functions
quaternion Slerp(float, const quaternion &in, const quaternion &in, bool &in);
quaternion SlerpExtraSpins(float, const quaternion &in, const quaternion &in, int &in);
void Intermediate(const quaternion &in, const quaternion &in, const quaternion &in, const quaternion &in, const quaternion &in);
quaternion Squad(float, const quaternion &in, const quaternion &in, const quaternion &in, const quaternion &in, bool &in);
quaternion nlerp(float, const quaternion &in, const quaternion &in, bool &in);

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace ScriptValueTypes


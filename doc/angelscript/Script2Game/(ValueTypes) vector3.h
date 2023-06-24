
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
 * @brief Binding of Ogre::Vector3
 */
struct vector3
{
public:
    // properties
    float x;
    float y;
    float z;
    
    ///@name Constructors
    /// @{
    vector3();
    vector3(float, float, float);
    vector3(const vector3 &in);
    vector3(float);
    /// @}
    
    /// @name Operators
    /// @{
    float opIndex(int) const;
    bool opEquals(const vector3 &in) const;
    vector3 opAdd(const vector3 &in) const;
    vector3 opSub(const vector3 &in) const;
    vector3 opMul(float) const;
    vector3 opMul(const vector3 &in) const;
    vector3 opDiv(float) const;
    vector3 opDiv(const vector3 &in) const;
    vector3 opAdd() const;
    vector3 opSub() const;
    vector3 &opAddAssign(const vector3 &in);
    vector3 &opAddAssign(float);
    vector3 &opSubAssign(const vector3 &in);
    vector3 &opSubAssign(float);
    vector3 &opMulAssign(const vector3 &in);
    vector3 &opMulAssign(float);
    vector3 &opDivAssign(const vector3 &in);
    vector3 &opDivAssign(float);
    /// @}
    
    /// @name Methods
    /// @{
    float length() const;
    float squaredLength() const;
    float distance(const vector3 &in) const;
    float squaredDistance(const vector3 &in) const;
    float dotProduct(const vector3 &in) const;
    float absDotProduct(const vector3 &in) const;
    float normalise();
    float crossProduct(const vector3 &in) const;
    vector3 midPoint(const vector3 &in) const;
    void makeFloor(const vector3 &in);
    void makeCeil(const vector3 &in);
    vector3 perpendicular() const;
    vector3 randomDeviant(const radian &in, const vector3 &in) const;
    radian angleBetween(const vector3 &in);
    quaternion getRotationTo(const vector3 &in, const vector3 &in) const;
    bool isZeroLength() const;
    vector3 normalisedCopy() const;
    vector3 reflect(const vector3 &in) const;
    bool positionEquals(const vector3 &in, float) const;
    bool positionCloses(const vector3 &in, float) const;
    bool directionEquals(const vector3 &in, radian &in) const;
    bool isNaN() const;   
    /// @}
}

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace ScriptValueTypes


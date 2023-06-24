
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
 * @brief Binding of Ogre::Vector2
 */
struct vector2
{
public:
    // properties
    float x;
    float y;
    
    ///@name Constructors
    /// @{
    vector2();
    vector2(float, float, float);
    vector2(const vector2 &in);
    vector2(float);
    /// @}
    
    /// @name Operators
    /// @{
    float opIndex(int) const;
    bool opEquals(const vector2 &in) const;
    vector2 opAdd(const vector2 &in) const;
    vector2 opSub(const vector2 &in) const;
    vector2 opMul(float) const;
    vector2 opMul(const vector2 &in) const;
    vector2 opDiv(float) const;
    vector2 opDiv(const vector2 &in) const;
    vector2 opAdd() const;
    vector2 opSub() const;
    vector2 &opAddAssign(const vector2 &in);
    vector2 &opAddAssign(float);
    vector2 &opSubAssign(const vector2 &in);
    vector2 &opSubAssign(float);
    vector2 &opMulAssign(const vector2 &in);
    vector2 &opMulAssign(float);
    vector2 &opDivAssign(const vector2 &in);
    vector2 &opDivAssign(float);
    /// @}
    
    /// @name Methods
    /// @{
    float length() const;
    float squaredLength() const;
    float distance(const vector2 &in) const;
    float squaredDistance(const vector2 &in) const;
    float dotProduct(const vector2 &in) const;
    float absDotProduct(const vector2 &in) const;
    float normalise();
    float crossProduct(const vector2 &in) const;
    vector2 midPoint(const vector2 &in) const;
    void makeFloor(const vector2 &in);
    void makeCeil(const vector2 &in);
    vector2 perpendicular() const;
    vector2 randomDeviant(const radian &in, const vector2 &in) const;
    radian angleBetween(const vector2 &in);
    quaternion getRotationTo(const vector2 &in, const vector2 &in) const;
    bool isZeroLength() const;
    vector2 normalisedCopy() const;
    vector2 reflect(const vector2 &in) const;
    bool positionEquals(const vector2 &in, float) const;
    bool positionCloses(const vector2 &in, float) const;
    bool directionEquals(const vector2 &in, radian &in) const;
    bool isNaN() const;   
    /// @}
}

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace ScriptValueTypes


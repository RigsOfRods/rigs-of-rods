
namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */

/**
 * @brief Binding of RoR::ProceduralPoint;
 */
class ProceduralPointClass
{
public:
    vector3 position;
    quaternion rotation;
    float width;
    float border_width;
    float border_height;
    RoadType type;
    int pillar_type;
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game

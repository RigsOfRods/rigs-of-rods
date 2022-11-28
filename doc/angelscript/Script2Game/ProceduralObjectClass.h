
namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */

/**
 * @brief Binding of RoR::ProceduralObject; a spline for generating dynamic roads.
 */
class ProceduralObjectClass
{
public:
    /**
    * Name of the road/street this spline represents.
    */
    string name;
    
    /**
    * Adds point at the end.
    */
    void addPoint(procedural_point);
    
    /**
    * Adds point before the element at the specified position.
    */
    void insertPoint(int pos, procedural_point);
    void deletePoint(int pos);
    procedural_point getPoint(int pos);
    int getNumPoints();
    ProceduralRoadClass @getRoad();
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game

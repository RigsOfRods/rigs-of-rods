
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
    * Property (read/write), default 0: Use 0 to disable smoothing.
    */
    int smoothing_num_splits;
    
    /**
    * Property (read/write), default true: Disabling collisions makes loading map faster (useful for debugging).
    */
    bool collision_enabled;    
    
    /**
    * Property (read/write), default "": Set to empty string to use default material.
    */
    string custom_material_name;
    
    /**
    * Name of the road/street this spline represents.
    */
    string getName();
    /**
    * Name of the road/street this spline represents.
    */
    void setName(const string&in name);    
    
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

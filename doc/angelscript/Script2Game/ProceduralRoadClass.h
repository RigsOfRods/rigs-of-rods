
namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */
 
enum RoadType
{
    ROAD_AUTOMATIC,
    ROAD_FLAT,
    ROAD_LEFT,
    ROAD_RIGHT,
    ROAD_BOTH,
    ROAD_BRIDGE,
    ROAD_MONORAIL
};

enum TextureFit
{
    TEXFIT_NONE,
    TEXFIT_BRICKWALL,
    TEXFIT_ROADS1,
    TEXFIT_ROADS2,
    TEXFIT_ROAD,
    TEXFIT_ROADS3,
    TEXFIT_ROADS4,
    TEXFIT_CONCRETEWALL,
    TEXFIT_CONCRETEWALLI,
    TEXFIT_CONCRETETOP,
    TEXFIT_CONCRETEUNDER
};

/**
 * @brief Binding of RoR::ProceduralRoad; a dynamically generated road mesh.
 * @note For internal use by ProceduralManagerClass - do not use unless you know what you're doing!
 */ 
 */
class ProceduralRoadClass
{
public:
    /**
    * For internal use by ProceduralManagerClass - do not use unless you know what you're doing!
    */
    void addBlock(vector3 pos, quaternion rot, RoadType type, float width, float border_width, float border_height, int pillar_type = 1);
    /**
    * For internal use by ProceduralManagerClass - do not use unless you know what you're doing!
    */    
    void addQuad(vector3 p1, vector3 p2, vector3 p3, vector3 p4, TextureFit texfit, vector3 pos, vector3 lastpos, float width, bool flip = false);
    /**
    * For internal use by ProceduralManagerClass - do not use unless you know what you're doing!
    */    
    void addCollisionQuad(vector3 p1, vector3 p2, vector3 p3, vector3 p4, string const&in gm_name, bool flip = false);
    /**
    * For internal use by ProceduralManagerClass - do not use unless you know what you're doing!
    */    
    void createMesh();
    /**
    * For internal use by ProceduralManagerClass - do not use unless you know what you're doing!
    */    
    void finish();
    /**
    * For internal use by ProceduralManagerClass - do not use unless you know what you're doing!
    */    
    void setCollisionEnabled(bool v);
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game

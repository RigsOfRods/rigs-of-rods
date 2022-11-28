
namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */    

/**
 * @brief Binding of RoR::ProceduralManager; generates dynamic roads for terrain.
 * @note Obtain the object using `game.getTerrain().getProceduralManager()`.
 */
class ProceduralManagerClass
{
public:
    /**
    * Generates road mesh and adds to internal list
    */
    void addObject(ProceduralObjectClass @po);

    /**
    * Clears road mesh and removes from internal list
    */
    void removeObject(ProceduralObjectClass @po);

    int getNumObjects();

    ProceduralObjectClass@ getObject(int pos);
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game

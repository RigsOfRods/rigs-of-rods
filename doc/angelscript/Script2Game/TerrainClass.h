
namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */    

/**
 * @brief Binding of RoR::Terrain; represents a loaded terrain.
 * @note Obtain the object using `game.getTerrain()`.
 */
class TerrainClass
{
public:
    /**
     * @return Full name of the terrain
     */
    string getTerrainName();
    
    /**
     * @return File name of the terrain definition (TERRN2 format).
     */
    string getTerrainFileName(); 
    
    /**
     * @return OGRE resource group of the terrain bundle (ZIP/directory under 'mods/') where definition files live.
     */
    string getTerrainFileResourceGroup();    
    
    /**
     * @return GUID (global unique ID) of the terrain, or empty string if not specified.
     */
    string getGUID();

    /**
     * @return true if specified as flat (no heightmap).
     */
    bool isFlat();

    /**
     * @return version of the terrain, as specified by author.
     */
    int getVersion();
 
    /**
     * @return Player spawn position when entering game.
     */
    vector3 getSpawnPos();
    
    ProceduralManagerClass @getProceduralManager();
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game

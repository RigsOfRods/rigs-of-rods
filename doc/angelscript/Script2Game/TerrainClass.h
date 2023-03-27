
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
    /// @name General
    /// @{

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
     * @return version of the terrain, as specified by author.
     */
    int getVersion();    
    
    /// @}

    /// @name Landscape
    /// @{    

    /**
     * @return true if specified as flat (no heightmap).
     */
    bool isFlat();

    /// @}

    /// @name Gameplay
    /// @{
 
    /**
     * @return Player spawn position when entering game.
     */
    vector3 getSpawnPos();
    
    /**
    * Adds an icon with description to survey map (minimap).
    * @param type informational, optional
    * @param filename The file name. Don't forget to specify the resource group!
    * @param resource_group Leave empty to use TexturesRG ('icons.zip/famicons.zip'), otherwise use `BeamClass/TerrainClass getResourceGroup()` to read from actor/terrain ZIP.
    * @param caption optional
    * @param pos The world position of the point of interest, in meters.
    * @param angle The world yaw in radians.
    * @param id The race ID of the icon (>=0), or -1 if not a race icon. You can use larger negative numbers for custom IDs.
    */
    void addSurveyMapEntity(const std::string& type, const std::string& filename, const std::string& resource_group, const std::string& caption, const Ogre::Vector3& pos, float angle, int id);
    
    /**
    * Removes all survey map icons with the given ID.
    * @param id The race ID of the icon (>=0), or -1 if not a race icon. You can use larger negative numbers for custom IDs.
    */
    void delSurveyMapEntities(int id);
    
    /// @}

    /// @name Subsystems
    /// @{    
    
    ProceduralManagerClass @getProceduralManager();
    
    /// @}    
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game

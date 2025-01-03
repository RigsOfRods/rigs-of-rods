namespace Script2Game {

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */

/**
 * @brief Binding of RoR::TerrainEditorObject; use `game.getEditorObjects()` to obtain live instances.
 */
class TerrainEditorObjectClass
{
public:

    const string & getName();
    const string & getInstanceName();
    const string & getType();
    const vector3 & getPosition();
    const vector3 & getRotation();
    void setPosition(const vector3&in pos);
    void setRotation(const vector3&in rot);
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} //namespace Script2Game

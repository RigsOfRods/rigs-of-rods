
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */  

    /** Encapsulates everything renderable by OGRE - use `game.getSceneManager()` to get it.
    */
    class  SceneManager
    {
    public:
        Entity@ createEntity(const string&in ent_name, const string &in mesh_name, const string &in mesh_rg);
        
        const string& getName() const;

        SceneNode@ getRootSceneNode();

        void destroyEntity(Entity@);

        void destroyEntity(const string &in);
        
        void destroySceneNode(SceneNode@);
        
        void destroySceneNode(const string &in);
        
    
    };
    
/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
    
} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)
    
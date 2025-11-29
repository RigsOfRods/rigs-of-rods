
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
        // PLEASE maintain the same order as in source/main/scripting/bindings/OgreAngelscript.cpp
    public:
    
        const string& getName() const;
        
        array<MovableObject@>@ __getMovableObjectsByType(const string&in typeName);
    
        /// @name Entities
        /// @{
        Entity@ createEntity(const string&in ent_name, const string &in mesh_name, const string &in mesh_rg);
        void destroyEntity(Entity@);
        void destroyEntity(const string &in);        
        /// @}
        
        /// @name Scene nodes
        /// @{
        SceneNode@ getRootSceneNode();  
        void destroySceneNode(SceneNode@);        
        void destroySceneNode(const string &in);
        /// @}
        
        /// @name Ambient light
        /// @{
        const color& getAmbientLight() const;
        void setAmbientLight(const color &in);
        /// @}
        
        /// @name Manual object
        /// @{
        ManualObject@ createManualObject(const string &in);           
        ManualObject@ getManualObject(const string &in);
        ManualObject@ destroyManualObject(const string &in);       
        void destroyManualObject(ManualObject@);
        /// @}
    
    };
    
/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
    
} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)
    

namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */  

    class  Node
    {
    public:
        const vector3& getPosition() const;
        void setPosition(const vector3 &in);
        
        const vector3& getScale() const;
        void setScale(const vector3 &in);
        
        const string& getName() const;
        Node@ getParent();
        
        /// A Rigs of Rods extension - generates unique name as "{name} ({mem_address})"
        string __getUniqueName() const;
        
        /// Not const because we don't want all elements to be const (this isn't the case with raw pointers in C++).
        /// @return a readonly array view object.
        ChildNodeArray@ getChildren();
    };
    
/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
    
} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)



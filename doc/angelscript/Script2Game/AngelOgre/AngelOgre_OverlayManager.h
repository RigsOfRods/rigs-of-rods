
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */  

    /** a singleton - use `Ogre::OverlayManager::getSingleton()` to retrieve instance.
    */
    class  OverlayManager
    {
    public:
        // Overlay (container) objects
        Overlay@ create(const string&in name);
        Overlay@ getByName(const string&in name);
        void destroy(const string&in name);
        void destroy(Overlay@ overlay);
        void destroyAll();
        array<Overlay@>@ getOverlays();
        // Utils
        float getViewportHeight() const;
        float getViewportWidth() const;
        // OverlayElement objects
        OverlayElement@ createOverlayElement(const string&in type, const string&in name, bool isTemplate=false);
        OverlayElement@ getOverlayElement(const string&in name) const;
        bool hasOverlayElement(const string&in) const;
        void destroyOverlayElement(const string&in, bool isTemplate=false) const;
        void destroyOverlayElement(OverlayElement@, bool isTemplate=false) const;
        void destroyAllOverlayElements(bool isTemplate=false) const;
        // Templates
        OverlayElement@ createOverlayElementFromTemplate(const string&in, const string&in, const string&in, bool=false);
        OverlayElement@ cloneOverlayElementFromTemplate(const string&in, const string&in);
        array<OverlayElement@>@ getTemplates();
        bool isTemplate(const string&in);
    };
    
/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
    
} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)



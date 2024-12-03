
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */  

    /** 
    */
    class  Overlay
    {
    public:
        // (order roughly matches OgreOverlay.h)
        
        const string& getName() const;
        // > z-order
        uint16 getZOrder();
        // > visibility
        bool isVisible() const;
        void show();
        void hide();
        // > 2D elements
        void add2D(OverlayElement@);
        void remove2D(OverlayElement@);
        // > scrolling
        void setScroll(float, float);
        float getScrollX() const;
        float getScrollY() const;
        void scroll(float, float);
        // > rotating
        void setRotate(const radian&in);
        const radian& getRotate() const;
        void rotate(const radian&in);
        // > scaling
        void setScale(float, float);
        float getScaleX() const;
        float getScaleY() const;
        // > 2D elements
        array<OverlayElement@>@ get2DElements();

    };
    
/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
    
} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)



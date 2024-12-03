
namespace AngelOgre { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */  
 
    // Register the GuiMetricsMode enum
    enum GuiMetricsMode
    {
        GMM_PIXELS,
        GMM_RELATIVE,
        GMM_RELATIVE_ASPECT_ADJUSTED
    }

    // Register the GuiHorizontalAlignment enum
    enum GuiHorizontalAlignment
    {
        GHA_LEFT,
        GHA_CENTER,
        GHA_RIGHT
    }


    class  OverlayElement
    {
    public:
        // (order roughly matches OgreOverlayElement.h)
        const string& getName() const;
        // > visibility
        void show();
        void hide();
        bool isVisible() const;
        // > positioning
        void setPosition(float, float);
        void setDimensions(float, float);
        float getLeft() const;
        float getTop() const;
        float getWidth() const;
        float getHeight() const;
        void setLeft(float);
        void setTop(float);
        void setWidth(float);
        void setHeight(float);
        // > material
        const string& getMaterialName() const;
        void setMaterialName(const string&in, const string&in);
        // > caption
        void setCaption(const string&in);
        const string& getCaption() const;
        // > color
        void setColour(const color&in);
        const color& getColour() const;
        // > GuiMetricsMode
        GuiMetricsMode getMetricsMode() const;
        void setMetricsMode(GuiMetricsMode);
        // > GuiHorizontalAlignment
        GuiHorizontalAlignment getHorizontalAlignment() const;
        void setHorizontalAlignment(GuiHorizontalAlignment);


    };
    
/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs
    
} // namespace AngelOgre (dummy, just to distinguish AngelScript from C++)



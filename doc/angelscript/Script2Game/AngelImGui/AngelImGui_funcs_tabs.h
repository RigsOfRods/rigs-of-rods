
  // =================================================== //
  // THIS IS NOT A C++ HEADER! Only a dummy for Doxygen. //
  // =================================================== //

namespace AngelImGui { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */   

    // Tab bars, tabs
    
    bool BeginTabBar(const string&in, int = 0);
    /// BeginTabItem() without X close button.
    bool BeginTabItem(const string&in, int = 0);
    /// BeginTabItem() with X close button.
    bool BeginTabItem(const string&in, bool&inout, int = 0);
    void SetTabItemClosed(const string&in);   

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelImGui (dummy, just to distinguish AngelScript from C++)
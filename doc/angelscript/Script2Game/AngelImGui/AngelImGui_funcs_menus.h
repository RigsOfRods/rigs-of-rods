
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

    // Menus
    bool BeginMainMenuBar();
    void EndMainMenuBar();
    bool BeginMenuBar();
    void EndMenuBar();
    bool BeginMenu(const string&in, bool = true);
    void EndMenu();
    bool MenuItem(const string&in, const string&in = string(), bool = false, bool = true);
    bool MenuItem(const string&in, const string&in, bool &inout, bool = true);

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelImGui (dummy, just to distinguish AngelScript from C++)
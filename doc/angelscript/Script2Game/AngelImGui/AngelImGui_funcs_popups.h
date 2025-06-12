
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

    void OpenPopup(const string&in);
   // bool BeginPopup(const string&in, int = 0);  // FIXME: update imgui!
    bool BeginPopup(const string&in, int = 0);                                         
    bool BeginPopupContextItem(const string&in = string(), int = 1);                                  
    bool BeginPopupContextWindow(const string&in = string(), int = 1, bool = true); // FIXME: update imgui! -- swapped args
    bool BeginPopupContextVoid(const string&in = string(), int = 1);
    bool BeginPopupModal(const string&in, bool &inout = null, int = 0);           
    void EndPopup(); 
//   bool OpenPopupOnItemClick(const string&in = string(), int = 1); // FIXME: update imgui!
 //   bool IsPopupOpen(const string&in);  // FIXME: update imgui!
    void CloseCurrentPopup();


/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelImGui (dummy, just to distinguish AngelScript from C++)
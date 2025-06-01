
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

    // Columns (considered legacy in latest versions - superseded by Tables!)
    
    void Columns(int = 1, const string&in = string(), bool = true);
    void NextColumn();                      
    int GetColumnIndex();                   
    float GetColumnWidth(int = -1);         
    float GetColumnOffset(int = -1);        
    void SetColumnOffset(int, float);       
    int GetColumnsCount();                  

 
/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelImGui (dummy, just to distinguish AngelScript from C++)
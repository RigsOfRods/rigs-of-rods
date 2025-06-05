
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

    // > Windows
    bool Begin(const string&in, bool&inout, int=0);     
    void End();                                         

    // > Child windows
    bool BeginChild(const string&in, const vector2&in=vector2(0,0), bool=false, int=0);      
    bool BeginChild(uint, const vector2&in=vector2(0,0), bool=false, int=0);                 
    void EndChild();                                       
    ImDrawList@ GetWindowDrawList();                       
    void PushStyleVar(int index, float val);               
    void PushStyleVar(int index, const vector2&in val);    
    void PopStyleVar(int count = 1)", asFUNCTION(ImGui::PopStyleVar), asCALL_CDECL);
    void PushStyleColor(int index, const color&in color);                
    void PopStyleColor(int count = 1);                                   
    void SetNextItemWidth(float);                                        
    void SetNextItemOpen(bool, ImGuiCond);                               

    vector2 GetContentRegionMax();
    vector2 GetContentRegionAvail();
    float GetContentRegionAvailWidth();
    vector2 GetWindowContentRegionMin();
    vector2 GetWindowContentRegionMax();
    float GetWindowRegionWidth();

    vector2 GetWindowPos();
    vector2 GetWindowSize(); 
    float GetWindowWedth();                                     
    float GetWindowHeight();                                    
    bool IsWindowCollapsed();                                   
    void SetWindowFontScale(float);                             

    void SetNextWindowPos(vector2, int=0, vector2=vector2(0,0));
    void SetNextWindowSize(vector2);
    void SetNextWindowContentSize(vector2);
    void SetNextWindowCollapsed(bool);
    void SetNextWindowFocus();
    void SetWindowPos(vector2);
    void SetWindowSize(vector2);
    void SetWindowCollapsed(bool);
    void SetWindowFocus();

    void SetWindowPos(const string&in, vector2);
    void SetWindowSize(const string&in, vector2);
    void SetWindowCollapsed(const string&in, bool);
    void SetWindowFocus(const string&in);

    float GetScrollX();                                 
    float GetScrollY();                                 
    float GetScrollMaxX();                              
    float GetScrollMaxY();                              
    void SetScrollX(float);                             
    void SetScrollY(float);                             
    void SetScrollHere(float = 0.5f);                   
    void SetScrollFromPosY(float, float = 0.5f);        

    void Separator();                                   
    void SameLine(float = 0.0f, float = -1.0f);         
    void NewLine();                                     
    void Spacing();                                     
    void Dummy(vector2);                                
    void Indent(float = 0.0f);                          
    void Unindent(float = 0.0f);                        
    void BeginGroup();                                  
    void EndGroup();                                    
    vector2 GetCursorPos();                             
    float GetCursorPosX();                              
    float GetCursorPosY();                              
    void SetCursorPos(vector2);                         
    void SetCursorPosX(float);                          
    void SetCursorPosY(float);                          
    vector2 GetCursorStartPos();                        
    vector2 GetCursorScreenPos();                       
    void SetCursorScreenPos(vector2);                   
    void AlignTextToFramePadding();                     
    float GetTextLineHeight();                          
    float GetTextLineHeightWithSpacing();               
        
        
 
/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelImGui (dummy, just to distinguish AngelScript from C++)
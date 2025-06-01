
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

    // Widgets: Text
    void Text(const string&in);             
    void TextDisabled(const string&in);       
    void TextColored(color col, const string&in);         
    void TextWrapped(const string&in);                                
    void LabelText(const string&in, const string&in);                               
    void Bullet();                                 
    void BulletText(const string&in);    

    // Widgets: Main
    bool Button(const string&in, vector2 = vector2(0,0));   
    bool SmallButton(const string&in);                                    
    bool InvisibleButton(const string&in, vector2);                        
    void Image(const Ogre::TexturePtr&in, vector2);                   
    bool Checkbox(const string&in, bool&inout);                             
    bool CheckboxFlags(const string&in, uint&inout, uint);                                 
    bool RadioButton(const string&in, bool);                                 
    bool RadioButton(const string&in, int&inout, int);                     
    void ProgressBar(float, vector2=vector2(-1,0), const string&in = \"\");     


    // Widgets: Drags 
    bool DragFloat(const string&in, float&inout, float = 1.0f, float = 0.0f, float = 0.0f);             
    bool DragFloat2(const string&in, vector2&inout);                                      
    bool DragFloat3(const string&in, vector3&inout);                                     

    bool DragFloatRange2(const string&in, float&inout, float&inout, float = 0.0f, float = 1.0f);                             
    bool DragIntRange2(const string&in, int&inout, int&inout, int, int);                                 

    // Widgets: Input with Keyboard
    static char imgui_text_buffer[4096]; // shared with multiple widgets
    bool InputText(const string&in, string&inout); 
    bool InputTextMultiline(const string&in, string&inout, const vector2&in = vector2(0,0));  
    bool InputFloat(const string&, float&inout);                                   
    bool InputFloat2(const string&, vector2&inout);                                    
    bool InputFloat3(const string&, vector3&inout);                                    

    bool InputInt(const string&, int&inout);                 


    // Widgets: Sliders (tip: ctrl+click on a slider to input with keyboard. manually input values aren't clamped, can go off-bounds)
    bool SliderFloat(const string&in, float&inout, float = 0.0f, float = 0.0f);                           
    bool SliderFloat2(const string&in, vector2&inout, float, float);                                       
    bool SliderFloat3(const string&in, vector3&inout, float, float);                
    bool SliderInt(const string&in, int&inout, int = 0, int = 0);    


    // Widgets: Color Editor/Picker
    bool ColorEdit3(const string&in, color&inout);    
    bool ColorEdit4(const string&in, color&inout);   

    bool ColorButton(const string&in, color); 

    // Widgets: Trees
    bool TreeNode(const string&in);                     
    void TreePush(const string&in);                     
    void TreePop();                                     
    void TreeAdvanceToLabelPos();                       
    float GetTreeNodeToLabelSpacing();                  
    void SetNextTreeNodeOpen(bool);                     
    bool CollapsingHeader(const string&in);             
    bool CollapsingHeader(const string&in, bool&inout); 

    // Widgets: Selectable / Lists
    bool Selectable(const string&in, bool = false);   
    bool ListBoxHeader(const string&in);              
        
    // Values
    void Value(const string&in, bool); 
    void Value(const string&in, int);  
    void Value(const string&in, uint); 
    void Value(const string&in, float);


/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelImGui (dummy, just to distinguish AngelScript from C++)
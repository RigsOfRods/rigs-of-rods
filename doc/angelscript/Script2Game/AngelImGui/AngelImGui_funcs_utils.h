
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
 
    // Clip-rects
    void PushClipRect(const vector2&, const vector2&, bool);    
    void PopClipRect();    

    // Focus
 //   void SetItemDefaultFocus();  // TODO update imgui
    void SetKeyboardFocusHere(int = 0);     


 

    bool IsItemHovered(int = 0);
    bool IsItemActive();
    bool IsItemClicked(int = 0);
    bool IsItemVisible();
    bool IsAnyItemHovered();
    bool IsAnyItemActive();
    vector2 GetItemRectMin();
    vector2 GetItemRectMax();
    vector2 GetItemRectSize();
    void SetItemAllowOverlap();
    bool IsWindowFocused(int = 0);
    bool IsWindowHovered(int = 0);
    bool IsRectVisible(const vector2&);
    bool IsRectVisible(const vector2&, const vector2&);
    float GetTime();
    int GetFrameCount();

    vector2 CalcTextSize(const string&in, bool hide_text_after_double_hash = false, float wrap_width = -1.0f);
    void CalcListClipping(int, float, int&inout, int&inout);
    bool BeginChildFrame(uint, const vector2&, int = 0);
    void EndChildFrame();

    int GetKeyIndex(int);
    bool IsKeyDown(int);
    bool IsKeyPressed(int, bool = true);
    bool IsKeyReleased(int);

    bool IsMouseDown(int);
    bool IsMouseClicked(int, bool = false);
    bool IsMouseDoubleClicked(int);
    bool IsMouseReleased(int);
    bool IsMouseDragging(int = 0, float = -1.0f);
    bool IsMouseHoveringRect(const vector2&in, const vector2&in, bool = true);
    vector2 GetMousePos();
    vector2 GetMousePosOnOpeningCurrentPopup();
    vector2 GetMouseDragDelta(int = 0, float = -1.0f);
    void ResetMouseDragDelta(int = 0);
    int GetMouseCursor();
    void SetMouseCursor(int);
    void CaptureKeyboardFromApp(bool = true);
    void CaptureMouseFromApp(bool = true);


    string GetClipboardText();
    void SetClipboardText(const string&in);

    /// Data plotting - we wrap the 'getter func' variant to resemble the 'float*' variant.
    /// PlotLines(const char* label, const float* values, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0, 0), int stride = sizeof(float));
    void PlotLines(const string&in label, array<float>&in values, int values_count, int values_offset = 0, const string&in overlay_text = string(), float scale_min = FLT_MAX, float scale_max = FLT_MAX, vector2 graph_size = vector2(0,0));


/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace AngelImGui (dummy, just to distinguish AngelScript from C++)
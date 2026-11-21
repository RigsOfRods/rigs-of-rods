
  // =================================================== //
  // THIS IS NOT A C++ HEADER! Only a dummy for Doxygen. //
  // =================================================== //

// NOTE: ImGui structs are in global namespace, not `ImGui::`
namespace Script2Game { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */   

/// Obtain using AngelImGui::GetWindowDrawList(); see AngelImGui
class ImDrawList
{
    void AddLine(const vector2&in p1, const vector2&in p2, const color&in col, float thickness = 1.f);
    void AddTriangle(const vector2&in p1, const vector2&in p2, const vector2&in p3, const color&in col, float thickness = 1.f);                       
    void AddTriangleFilled(const vector2&in p1, const vector2&in p2, const vector2&in p3, const color&in col);
    void AddRect(const vector2&in p_min, const vector2&in p_max, const color&in col, float rounding = 0.0f, int rounding_corners = 15, float thickness = 1.f);
    void AddRectFilled(const vector2&in p_min, const vector2&in p_max, const color&in col, float rounding = 0.0f, int rounding_corners = 15);
    void AddCircle(const vector2&in center, float radius, const color&in col, int num_segments = 12, float thickness = 1.f);
    void AddCircleFilled(const vector2&in center, float radius, const color&in col, int num_segments = 12);
    void AddText(const vector2&in pos, const color&in col, const string&in text);
    void AddText(const vector2&in pos, const color&in col, const string&in text, float font_size);
    void PathClear();
    void PathLineTo(const vector2&in pos);
    void PathArcTo(const vector2&in center, float radius, float a_min, float a_max, int num_segments = 0);
    void PathFillConvex(const color&in col);
    void PathStroke(const color&in col, bool closed, float thickness = 1.f);
    void AddImage(const Ogre::TexturePtr&in tex, const vector2&in p_min, const vector2&in p_max, const vector2&in uv_min, const vector2&in uv_max, const color&in col);
}

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace Script2Game (dummy, just to distinguish AngelScript from C++)
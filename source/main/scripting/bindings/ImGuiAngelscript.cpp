/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2022 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/// @file
/// @author https://gist.github.com/JSandusky/54b85068aa30390c91a0b377703f042e
/// @author https://discourse.urho3d.io/t/dear-imgui-w-o-steamrolling/3960
/// @author Petr Ohlidal (enums, ImDrawList, tabs...)

#include "OgreImGui.h"
#include "scriptarray/scriptarray.h"
#include "ScriptEngine.h"

#include <angelscript.h>
#include <Ogre.h>
#include <string>

using namespace AngelScript;
using namespace Ogre;
using namespace std;

float ImGuiPlotLinesScriptValueGetterFunc(void* data, int index)
{
    CScriptArray* array_obj = static_cast<CScriptArray*>(data);
    void* value_raw = array_obj->At(index);
    if (value_raw == nullptr)
    {
        return 0.f; // out of bounds
    }
    else
    {
        return *static_cast<float*>(value_raw);
    }
}

void RoR::RegisterImGuiBindings(AngelScript::asIScriptEngine* engine)
{
    // ENUMS (global namespace)


    engine->RegisterEnum("ImGuiStyleVar"); 
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_Alpha", ImGuiStyleVar_Alpha);                         // float     Alpha
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_WindowPadding", ImGuiStyleVar_WindowPadding);        // ImVec2    WindowPadding
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_WindowRounding", ImGuiStyleVar_WindowRounding);       // float     WindowRounding
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_WindowBorderSize", ImGuiStyleVar_WindowBorderSize);    // float     WindowBorderSize
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_WindowMinSize", ImGuiStyleVar_WindowMinSize);         // ImVec2    WindowMinSize
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_WindowTitleAlign", ImGuiStyleVar_WindowTitleAlign);    // ImVec2    WindowTitleAlign
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_ChildRounding", ImGuiStyleVar_ChildRounding);         // float     ChildRounding
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_ChildBorderSize", ImGuiStyleVar_ChildBorderSize);     // float     ChildBorderSize
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_PopupRounding", ImGuiStyleVar_PopupRounding);         // float     PopupRounding
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_PopupBorderSize", ImGuiStyleVar_PopupBorderSize);     // float     PopupBorderSize
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_FramePadding", ImGuiStyleVar_FramePadding);          // ImVec2    FramePadding
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_FrameRounding", ImGuiStyleVar_FrameRounding);        // float     FrameRounding
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_FrameBorderSize", ImGuiStyleVar_FrameBorderSize);     // float     FrameBorderSize
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_ItemSpacing", ImGuiStyleVar_ItemSpacing);             // ImVec2    ItemSpacing
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_ItemInnerSpacing", ImGuiStyleVar_ItemInnerSpacing);    // ImVec2    ItemInnerSpacing
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_IndentSpacing", ImGuiStyleVar_IndentSpacing);          // float     IndentSpacing
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_ScrollbarSize", ImGuiStyleVar_ScrollbarSize);          // float     ScrollbarSize
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_ScrollbarRounding", ImGuiStyleVar_ScrollbarRounding);   // float     ScrollbarRounding
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_GrabMinSize", ImGuiStyleVar_GrabMinSize);                // float     GrabMinSize
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_GrabRounding", ImGuiStyleVar_GrabRounding);              // float     GrabRounding
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_TabRounding", ImGuiStyleVar_TabRounding);                 // float     TabRounding
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_ButtonTextAlign", ImGuiStyleVar_ButtonTextAlign);         // ImVec2    ButtonTextAlign
    engine->RegisterEnumValue("ImGuiStyleVar", "ImGuiStyleVar_SelectableTextAlign", ImGuiStyleVar_SelectableTextAlign); // ImVec2    SelectableTextAlign

    engine->RegisterEnum("ImGuiWindowFlags");
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_None", ImGuiWindowFlags_None);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_NoTitleBar", ImGuiWindowFlags_NoTitleBar);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_NoResize", ImGuiWindowFlags_NoResize);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_NoMove", ImGuiWindowFlags_NoMove);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_NoScrollbar", ImGuiWindowFlags_NoScrollbar);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_NoScrollWithMouse", ImGuiWindowFlags_NoScrollWithMouse);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_NoCollapse", ImGuiWindowFlags_NoCollapse);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_AlwaysAutoResize", ImGuiWindowFlags_AlwaysAutoResize);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_NoBackground", ImGuiWindowFlags_NoBackground);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_NoSavedSettings", ImGuiWindowFlags_NoSavedSettings);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_NoMouseInputs", ImGuiWindowFlags_NoMouseInputs);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_MenuBar", ImGuiWindowFlags_MenuBar);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_HorizontalScrollbar", ImGuiWindowFlags_HorizontalScrollbar);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_NoFocusOnAppearing", ImGuiWindowFlags_NoFocusOnAppearing);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_NoBringToFrontOnFocus", ImGuiWindowFlags_NoBringToFrontOnFocus);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_AlwaysVerticalScrollbar", ImGuiWindowFlags_AlwaysVerticalScrollbar);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_AlwaysHorizontalScrollbar", ImGuiWindowFlags_AlwaysHorizontalScrollbar);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_AlwaysUseWindowPadding", ImGuiWindowFlags_AlwaysUseWindowPadding);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_NoNavInputs", ImGuiWindowFlags_NoNavInputs);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_NoNavFocus", ImGuiWindowFlags_NoNavFocus);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_UnsavedDocument", ImGuiWindowFlags_UnsavedDocument);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_NoNav", ImGuiWindowFlags_NoNav);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_NoDecoration", ImGuiWindowFlags_NoDecoration);
    engine->RegisterEnumValue("ImGuiWindowFlags", "ImGuiWindowFlags_NoInputs", ImGuiWindowFlags_NoInputs);

    engine->RegisterEnum("ImGuiCond");
    engine->RegisterEnumValue("ImGuiCond", "ImGuiCond_Always", ImGuiCond_Always);
    engine->RegisterEnumValue("ImGuiCond", "ImGuiCond_Once", ImGuiCond_Once); // Set the variable once per runtime session (only the first call with succeed)
    engine->RegisterEnumValue("ImGuiCond", "ImGuiCond_FirstUseEver", ImGuiCond_FirstUseEver); // Set the variable if the object/window has no persistently saved data (no entry in .ini file)
    engine->RegisterEnumValue("ImGuiCond", "ImGuiCond_Appearing", ImGuiCond_Appearing); // Set the variable if the object/window is appearing after being hidden/inactive (or the first time)

    engine->RegisterEnum("ImGuiTabBarFlags");
    engine->RegisterEnumValue("ImGuiTabBarFlags", "ImGuiTabBarFlags_None", ImGuiTabBarFlags_None);
    engine->RegisterEnumValue("ImGuiTabBarFlags", "ImGuiTabBarFlags_Reorderable", ImGuiTabBarFlags_Reorderable); // // Allow manually dragging tabs to re-order them + New tabs are appended at the end of list
    engine->RegisterEnumValue("ImGuiTabBarFlags", "ImGuiTabBarFlags_AutoSelectNewTabs", ImGuiTabBarFlags_AutoSelectNewTabs); // // Automatically select new tabs when they appear
    engine->RegisterEnumValue("ImGuiTabBarFlags", "ImGuiTabBarFlags_TabListPopupButton", ImGuiTabBarFlags_TabListPopupButton); // // Disable buttons to open the tab list popup
    engine->RegisterEnumValue("ImGuiTabBarFlags", "ImGuiTabBarFlags_NoCloseWithMiddleMouseButton", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton); // // Disable behavior of closing tabs (that are submitted with p_open != NULL) with middle mouse button. You can still repro this behavior on user's side with if (IsItemHovered() && IsMouseClicked(2)) *p_open = false.
    engine->RegisterEnumValue("ImGuiTabBarFlags", "ImGuiTabBarFlags_NoTabListScrollingButtons", ImGuiTabBarFlags_NoTabListScrollingButtons); // // Disable scrolling buttons (apply when fitting policy is ImGuiTabBarFlags_FittingPolicyScroll)
    engine->RegisterEnumValue("ImGuiTabBarFlags", "ImGuiTabBarFlags_NoTooltip", ImGuiTabBarFlags_NoTooltip); // // Disable tooltips when hovering a tab
    engine->RegisterEnumValue("ImGuiTabBarFlags", "ImGuiTabBarFlags_FittingPolicyResizeDown", ImGuiTabBarFlags_FittingPolicyResizeDown); // // Resize tabs when they don't fit
    engine->RegisterEnumValue("ImGuiTabBarFlags", "ImGuiTabBarFlags_FittingPolicyScroll", ImGuiTabBarFlags_FittingPolicyScroll); // // Add scroll buttons when tabs don't fit
    engine->RegisterEnumValue("ImGuiTabBarFlags", "ImGuiTabBarFlags_FittingPolicyMask_", ImGuiTabBarFlags_FittingPolicyMask_); // ImGuiTabBarFlags_FittingPolicyResizeDown | ImGuiTabBarFlags_FittingPolicyScroll,
    engine->RegisterEnumValue("ImGuiTabBarFlags", "ImGuiTabBarFlags_FittingPolicyDefault_", ImGuiTabBarFlags_FittingPolicyDefault_); // ImGuiTabBarFlags_FittingPolicyResizeDown

    engine->RegisterEnum("ImGuiTabItemFlags");
    engine->RegisterEnumValue("ImGuiTabItemFlags", "ImGuiTabItemFlags_None", ImGuiTabItemFlags_None);
    engine->RegisterEnumValue("ImGuiTabItemFlags", "ImGuiTabItemFlags_UnsavedDocument", ImGuiTabItemFlags_UnsavedDocument); //  // Append '*' to title without affecting the ID, as a convenience to avoid using the ### operator. Also: tab is selected on closure and closure is deferred by one frame to allow code to undo it without flicker.
    engine->RegisterEnumValue("ImGuiTabItemFlags", "ImGuiTabItemFlags_SetSelected", ImGuiTabItemFlags_SetSelected); // // Trigger flag to programmatically make the tab selected when calling BeginTabItem()
    engine->RegisterEnumValue("ImGuiTabItemFlags", "ImGuiTabItemFlags_NoCloseWithMiddleMouseButton", ImGuiTabItemFlags_NoCloseWithMiddleMouseButton); //  // Disable behavior of closing tabs (that are submitted with p_open != NULL) with middle mouse button. You can still repro this behavior on user's side with if (IsItemHovered() && IsMouseClicked(2)) *p_open = false.
    engine->RegisterEnumValue("ImGuiTabItemFlags", "ImGuiTabItemFlags_NoPushId", ImGuiTabItemFlags_NoPushId); //  // Don't call PushID(tab->ID)/PopID() on BeginTabItem()/EndTabItem()
    

    engine->RegisterEnum("ImGuiCol");
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_Text", ImGuiCol_Text);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_TextDisabled", ImGuiCol_TextDisabled);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_WindowBg", ImGuiCol_WindowBg);              // Background of normal windows
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ChildBg", ImGuiCol_ChildBg);               // Background of child windows
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_PopupBg", ImGuiCol_PopupBg);               // Background of popups, menus, tooltips windows
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_Border", ImGuiCol_Border);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_BorderShadow", ImGuiCol_BorderShadow);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_FrameBg", ImGuiCol_FrameBg);               // Background of checkbox, radio button, plot, slider, text input
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_FrameBgHovered", ImGuiCol_FrameBgHovered);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_FrameBgActive", ImGuiCol_FrameBgActive);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_TitleBg", ImGuiCol_TitleBg);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_TitleBgActive", ImGuiCol_TitleBgActive);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_TitleBgCollapsed", ImGuiCol_TitleBgCollapsed);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_MenuBarBg", ImGuiCol_MenuBarBg);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ScrollbarBg", ImGuiCol_ScrollbarBg);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ScrollbarGrab", ImGuiCol_ScrollbarGrab);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_CheckMark", ImGuiCol_CheckMark);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_SliderGrab", ImGuiCol_SliderGrab);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_SliderGrabActive", ImGuiCol_SliderGrabActive);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_Button", ImGuiCol_Button);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ButtonHovered", ImGuiCol_ButtonHovered);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ButtonActive", ImGuiCol_ButtonActive);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_Header", ImGuiCol_Header);                // Header* colors are used for CollapsingHeader, TreeNode, Selectable, MenuItem
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_HeaderHovered", ImGuiCol_HeaderHovered);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_HeaderActive", ImGuiCol_HeaderActive);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_Separator", ImGuiCol_Separator);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_SeparatorHovered", ImGuiCol_SeparatorHovered);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_SeparatorActive", ImGuiCol_SeparatorActive);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ResizeGrip", ImGuiCol_ResizeGrip);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ResizeGripHovered", ImGuiCol_ResizeGripHovered);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ResizeGripActive", ImGuiCol_ResizeGripActive);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_Tab", ImGuiCol_Tab);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_TabHovered", ImGuiCol_TabHovered);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_TabActive", ImGuiCol_TabActive);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_TabUnfocused", ImGuiCol_TabUnfocused);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_TabUnfocusedActive", ImGuiCol_TabUnfocusedActive);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_PlotLines", ImGuiCol_PlotLines);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_PlotLinesHovered", ImGuiCol_PlotLinesHovered);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_PlotHistogram", ImGuiCol_PlotHistogram);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_PlotHistogramHovered", ImGuiCol_PlotHistogramHovered);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_TextSelectedBg", ImGuiCol_TextSelectedBg);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_DragDropTarget", ImGuiCol_DragDropTarget);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_NavHighlight", ImGuiCol_NavHighlight);          // Gamepad/keyboard: current highlighted item
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_NavWindowingHighlight", ImGuiCol_NavWindowingHighlight); // Highlight window when using CTRL+TAB
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_NavWindowingDimBg", ImGuiCol_NavWindowingDimBg);     // Darken/colorize entire screen behind the CTRL+TAB window list, when active
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ModalWindowDimBg", ImGuiCol_ModalWindowDimBg);      // Darken/colorize entire screen behind a modal window, when one is active

    // ImDrawList object (global namespace)
    engine->RegisterObjectType("ImDrawList", sizeof(ImDrawList), asOBJ_REF | asOBJ_NOCOUNT);
    engine->RegisterObjectMethod("ImDrawList", "void AddLine(const vector2&in p1, const vector2&in p2, const color&in col, float thickness = 1.f)", asFUNCTIONPR([](ImDrawList* drawlist, Ogre::Vector2 const& p1, Ogre::Vector2 const& p2, Ogre::ColourValue const& col, float thickness) { drawlist->AddLine(ImVec2(p1.x, p1.y), ImVec2(p2.x, p2.y), ImColor(col.r, col.g, col.b, col.a), thickness); }, (ImDrawList * , Ogre::Vector2 const& , Ogre::Vector2 const& , Ogre::ColourValue const& , float ), void), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("ImDrawList", "void AddTriangle(const vector2&in p1, const vector2&in p2, const vector2&in p3, const color&in col, float thickness = 1.f)", asFUNCTIONPR([](ImDrawList* drawlist, Ogre::Vector2 const& p1, Ogre::Vector2 const& p2, Ogre::Vector2 const& p3, Ogre::ColourValue const& col, float thickness) { drawlist->AddTriangle(ImVec2(p1.x, p1.y), ImVec2(p2.x, p2.y), ImVec2(p3.x, p3.y), ImColor(col.r, col.g, col.b, col.a), thickness); }, (ImDrawList*, Ogre::Vector2 const&, Ogre::Vector2 const&, Ogre::Vector2 const&, Ogre::ColourValue const&, float), void), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("ImDrawList", "void AddTriangleFilled(const vector2&in p1, const vector2&in p2, const vector2&in p3, const color&in col)", asFUNCTIONPR([](ImDrawList* drawlist, Ogre::Vector2 const& p1, Ogre::Vector2 const& p2, Ogre::Vector2 const& p3, Ogre::ColourValue const& col) { drawlist->AddTriangleFilled(ImVec2(p1.x, p1.y), ImVec2(p2.x, p2.y), ImVec2(p3.x, p3.y), ImColor(col.r, col.g, col.b, col.a)); }, (ImDrawList*, Ogre::Vector2 const&, Ogre::Vector2 const&, Ogre::Vector2 const&, Ogre::ColourValue const&), void), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("ImDrawList", "void AddRect(const vector2&in p_min, const vector2&in p_max, const color&in col, float rounding = 0.0f, int rounding_corners = 15, float thickness = 1.f)", asFUNCTIONPR([](ImDrawList* drawlist, Ogre::Vector2 const& p1, Ogre::Vector2 const& p2, Ogre::ColourValue const& col, float rounding, int rounding_corners, float thickness) { drawlist->AddRect(ImVec2(p1.x, p1.y), ImVec2(p2.x, p2.y), ImColor(col.r, col.g, col.b, col.a), rounding, rounding_corners, thickness); }, (ImDrawList * drawlist, Ogre::Vector2 const& , Ogre::Vector2 const& , Ogre::ColourValue const& , float , int , float ), void), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("ImDrawList", "void AddRectFilled(const vector2&in p_min, const vector2&in p_max, const color&in col, float rounding = 0.0f, int rounding_corners = 15)", asFUNCTIONPR([](ImDrawList* drawlist, Ogre::Vector2 const& p1, Ogre::Vector2 const& p2, Ogre::ColourValue const& col, float rounding, int rounding_corners) { drawlist->AddRectFilled(ImVec2(p1.x, p1.y), ImVec2(p2.x, p2.y), ImColor(col.r, col.g, col.b, col.a), rounding, rounding_corners); }, (ImDrawList * drawlist, Ogre::Vector2 const&, Ogre::Vector2 const&, Ogre::ColourValue const&, float, int), void), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("ImDrawList", "void AddCircle(const vector2&in center, float radius, const color&in col, int num_segments = 12, float thickness = 1.f)", asFUNCTIONPR([](ImDrawList* drawlist, Ogre::Vector2 const& center, float radius, Ogre::ColourValue const& col, int num_segments, float thickness) { drawlist->AddCircle(ImVec2(center.x, center.y), radius, ImColor(col.r, col.g, col.b, col.a), num_segments, thickness); }, (ImDrawList*, Ogre::Vector2 const&, float, Ogre::ColourValue const&, int, float), void), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("ImDrawList", "void AddCircleFilled(const vector2&in center, float radius, const color&in col, int num_segments = 12)", asFUNCTIONPR([](ImDrawList* drawlist, Ogre::Vector2 const& center, float radius, Ogre::ColourValue const& col, int num_segments) { drawlist->AddCircleFilled(ImVec2(center.x, center.y), radius, ImColor(col.r, col.g, col.b, col.a), num_segments); }, (ImDrawList*, Ogre::Vector2 const&, float, Ogre::ColourValue const&, int), void), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("ImDrawList", "void AddText(const vector2&in pos, const color&in col, const string&in text)", asFUNCTIONPR([](ImDrawList* drawlist, Ogre::Vector2 const& pos, Ogre::ColourValue const& col, std::string const& text) { drawlist->AddText(ImVec2(pos.x, pos.y), ImColor(col.r, col.g, col.b, col.a), text.c_str()); }, (ImDrawList * drawlist, Ogre::Vector2 const&, Ogre::ColourValue const&, std::string const&), void), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("ImDrawList", "void AddImage(const Ogre::TexturePtr&in tex, const vector2&in p_min, const vector2&in p_max, const vector2&in uv_min, const vector2&in uv_max, const color&in col)", asFUNCTIONPR([](ImDrawList* drawlist, Ogre::TexturePtr const& tex, Ogre::Vector2 const& p_min, Ogre::Vector2 const& p_max, Ogre::Vector2 const& uv_min, Ogre::Vector2 const& uv_max, Ogre::ColourValue const& col) { drawlist->AddImage((ImTextureID)tex->getHandle(), ImVec2(p_min.x, p_min.y), ImVec2(p_max.x, p_max.y), ImVec2(uv_min.x, uv_min.y), ImVec2(uv_max.x, uv_max.y), ImColor(col.r, col.g, col.b, col.a)); }, (ImDrawList*, Ogre::TexturePtr const&, Ogre::Vector2 const&, Ogre::Vector2 const&, Ogre::Vector2 const&, Ogre::Vector2 const&, Ogre::ColourValue const&), void), asCALL_CDECL_OBJFIRST);
    // FUNCTIONS (namespace ImGui)
    engine->SetDefaultNamespace("ImGui");
    
    // > Windows
    engine->RegisterGlobalFunction("bool Begin(const string&in, bool, int=0)", asFUNCTIONPR([](const string& name, bool opened, int flags) { return ImGui::Begin(name.c_str(), &opened, flags); }, (const string&, bool, int), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("void End()", asFUNCTIONPR(ImGui::End, (), void), asCALL_CDECL);

    // > Child windows
    engine->RegisterGlobalFunction("bool BeginChild(const string&in, const vector2&in=vector2(0,0), bool=false, int=0)", asFUNCTIONPR([](const string& name, const Ogre::Vector2& size, bool border, int flags) { return ImGui::BeginChild(name.c_str(), ImVec2(size.x, size.y), border, flags); }, (const string&, const Ogre::Vector2&, bool, int), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool BeginChild(uint, const vector2&in=vector2(0,0), bool=false, int=0)", asFUNCTIONPR([](ImGuiID id, const Ogre::Vector2& size, bool border, int flags) { return ImGui::BeginChild(id, ImVec2(size.x, size.y), border, flags); }, (ImGuiID, const Ogre::Vector2&, bool, int), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("void EndChild()", asFUNCTIONPR(ImGui::EndChild, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("ImDrawList@ GetWindowDrawList()", asFUNCTIONPR(ImGui::GetWindowDrawList, (), ImDrawList*), asCALL_CDECL);
    engine->RegisterGlobalFunction("void PushStyleVar(int index, float val)", asFUNCTIONPR([](int index, float val) { ImGui::PushStyleVar(index,val); }, (int, float), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void PushStyleVar(int index, const vector2&in val)", asFUNCTIONPR([](int index, const Ogre::Vector2& val) { ImGui::PushStyleVar(index, ImVec2(val.x, val.y)); }, (int, const Ogre::Vector2 &), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void PopStyleVar(int count = 1)", asFUNCTION(ImGui::PopStyleVar), asCALL_CDECL);
    engine->RegisterGlobalFunction("void PushStyleColor(int index, const color&in color)", asFUNCTIONPR([](int index, Ogre::ColourValue const& col) { ImGui::PushStyleColor(index, (ImU32)ImColor(col.r, col.g, col.b, col.a)); }, (int, Ogre::ColourValue const&), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void PopStyleColor(int count = 1)", asFUNCTIONPR(ImGui::PopStyleColor, (int), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetNextItemWidth(float)", asFUNCTIONPR(ImGui::SetNextItemWidth, (float), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetNextItemOpen(bool, ImGuiCond)", asFUNCTIONPR(ImGui::SetNextItemOpen, (bool, int), void), asCALL_CDECL);

    engine->RegisterGlobalFunction("vector2 GetContentRegionMax()", asFUNCTIONPR([]() { 
        auto v = ImGui::GetContentRegionMax(); return Vector2(v.x, v.y); }, (), Vector2), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetContentRegionAvail()", asFUNCTIONPR([]() { 
        auto v = ImGui::GetContentRegionAvail(); return Vector2(v.x, v.y); }, (), Vector2), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetContentRegionAvailWidth()", asFUNCTIONPR(ImGui::GetContentRegionAvailWidth, (), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetWindowContentRegionMin()", asFUNCTIONPR([]() { 
        auto v = ImGui::GetWindowContentRegionMin(); return Vector2(v.x, v.y); }, (), Vector2), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetWindowContentRegionMax()", asFUNCTIONPR([]() { 
        auto v = ImGui::GetWindowContentRegionMax(); return Vector2(v.x, v.y); }, (), Vector2), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetWindowRegionWidth()", asFUNCTIONPR(ImGui::GetWindowContentRegionWidth, (), float), asCALL_CDECL);

    engine->RegisterGlobalFunction("vector2 GetWindowPos()", asFUNCTIONPR([]() { 
        auto v = ImGui::GetWindowPos(); return Vector2(v.x, v.y); }, (), Vector2), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetWindowSize()", asFUNCTIONPR([]() { 
        auto v = ImGui::GetWindowSize(); return Vector2(v.x, v.y); }, (), Vector2), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetWindowWedth()", asFUNCTIONPR(ImGui::GetWindowWidth, (), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetWindowHeight()", asFUNCTIONPR(ImGui::GetWindowHeight, (), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsWindowCollapsed()", asFUNCTIONPR(ImGui::IsWindowCollapsed, (), bool), asCALL_CDECL);
  //  engine->RegisterGlobalFunction("bool IsWindowAppearing()", asFUNCTIONPR(ImGui::IsWindowAppearing, (), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetWindowFontScale(float)", asFUNCTIONPR(ImGui::SetWindowFontScale, (float), void), asCALL_CDECL);

    engine->RegisterGlobalFunction("void SetNextWindowPos(vector2)", asFUNCTIONPR([](Vector2 v) { 
        ImGui::SetNextWindowPos(ImVec2(v.x, v.y)); }, (Vector2), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetNextWindowSize(vector2)", asFUNCTIONPR([](Vector2 v) { 
        ImGui::SetNextWindowSize(ImVec2(v.x, v.y)); }, (Vector2), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetNextWindowContentSize(vector2)", asFUNCTIONPR([](Vector2 v) { 
        ImGui::SetNextWindowContentSize(ImVec2(v.x, v.y)); }, (Vector2), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetNextWindowCollapsed(bool)", asFUNCTIONPR([](bool v) { 
        ImGui::SetNextWindowCollapsed(v); }, (bool), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetNextWindowFocus()", asFUNCTIONPR([]() { 
        ImGui::SetNextWindowFocus(); }, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetWindowPos(vector2)", asFUNCTIONPR([](Vector2 v) { 
        ImGui::SetWindowPos(ImVec2(v.x, v.y)); }, (Vector2), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetWindowSize(vector2)", asFUNCTIONPR([](Vector2 v) { 
        ImGui::SetWindowSize(ImVec2(v.x, v.y)); }, (Vector2), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetWindowCollapsed(bool)", asFUNCTIONPR([](bool v) { 
        ImGui::SetWindowCollapsed(v); }, (bool), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetWindowFocus()", asFUNCTIONPR([]() { ImGui::SetWindowFocus(); }, (), void), asCALL_CDECL);

    engine->RegisterGlobalFunction("void SetWindowPos(const string&in, vector2)", asFUNCTIONPR([](const string& name, Vector2 v) {
        ImGui::SetWindowPos(name.c_str(), ImVec2(v.x, v.y)); }, (const string&, Vector2), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetWindowSize(const string&in, vector2)", asFUNCTIONPR([](const string& name, Vector2 v) {
        ImGui::SetWindowSize(name.c_str(), ImVec2(v.x, v.y)); }, (const string&, Vector2), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetWindowCollapsed(const string&in, bool)", asFUNCTIONPR([](const string& name, bool v) { 
        ImGui::SetWindowCollapsed(name.c_str(), v); }, (const string&, bool), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetWindowFocus(const string&in)", asFUNCTIONPR([](const string& v) { 
        ImGui::SetWindowFocus(v.c_str()); }, (const string&), void), asCALL_CDECL);

    engine->RegisterGlobalFunction("float GetScrollX()", asFUNCTIONPR(ImGui::GetScrollX, (), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetScrollY()", asFUNCTIONPR(ImGui::GetScrollY, (), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetScrollMaxX()", asFUNCTIONPR(ImGui::GetScrollMaxX, (), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetScrollMaxY()", asFUNCTIONPR(ImGui::GetScrollMaxY, (), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetScrollX(float)", asFUNCTIONPR(ImGui::SetScrollX, (float), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetScrollY(float)", asFUNCTIONPR(ImGui::SetScrollY, (float), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetScrollHere(float = 0.5f)", asFUNCTIONPR(ImGui::SetScrollHere, (float), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetScrollFromPosY(float, float = 0.5f)", asFUNCTIONPR(ImGui::SetScrollFromPosY, (float,float), void), asCALL_CDECL);

    engine->RegisterGlobalFunction("void Separator()", asFUNCTIONPR(ImGui::Separator, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SameLine(float = 0.0f, float = -1.0f)", asFUNCTIONPR(ImGui::SameLine, (float,float), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void NewLine()", asFUNCTIONPR(ImGui::NewLine, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void Spacing()", asFUNCTIONPR(ImGui::Spacing, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void Dummy(vector2)", asFUNCTIONPR([](Vector2 v) { ImGui::Dummy(ImVec2(v.x, v.y)); }, (Vector2), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void Indent(float = 0.0f)", asFUNCTIONPR(ImGui::Indent, (float), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void Unindent(float = 0.0f)", asFUNCTIONPR(ImGui::Unindent, (float), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void BeginGroup()", asFUNCTIONPR(ImGui::BeginGroup, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void EndGroup()", asFUNCTIONPR(ImGui::EndGroup, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetCursorPos()", asFUNCTIONPR([]() { auto v = ImGui::GetCursorPos(); return Vector2(v.x, v.y); }, (), Vector2), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetCursorPosX()", asFUNCTIONPR(ImGui::GetCursorPosX, (), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetCursorPosY()", asFUNCTIONPR(ImGui::GetCursorPosY, (), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetCursorPos(vector2)", asFUNCTIONPR([](Vector2 v) { ImGui::SetCursorPos(ImVec2(v.x, v.y)); }, (Vector2), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetCursorPosX(float)", asFUNCTIONPR(ImGui::SetCursorPosX, (float), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetCursorPosY(float)", asFUNCTIONPR(ImGui::SetCursorPosY, (float), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetCursorStartPos()", asFUNCTIONPR([]() { auto v = ImGui::GetCursorStartPos(); return Vector2(v.x, v.y); }, (), Vector2), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetCursorScreenPos()", asFUNCTIONPR([]() { auto v = ImGui::GetCursorScreenPos(); return Vector2(v.x, v.y); }, (), Vector2), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetCursorScreenPos(vector2)", asFUNCTIONPR([](Vector2 v) { ImGui::SetCursorScreenPos(ImVec2(v.x, v.y)); }, (Vector2), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void AlignTextToFramePadding()", asFUNCTIONPR(ImGui::AlignTextToFramePadding, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetTextLineHeight()", asFUNCTIONPR(ImGui::GetTextLineHeight, (), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetTextLineHeightWithSpacing()", asFUNCTIONPR(ImGui::GetTextLineHeightWithSpacing, (), float), asCALL_CDECL);
  //  engine->RegisterGlobalFunction("float GetFrameHeight()", asFUNCTIONPR(ImGui::GetFrameHeight, (), float), asCALL_CDECL);
  //  engine->RegisterGlobalFunction("float GetFrameHeightWithSpacing()", asFUNCTIONPR(ImGui::GetFrameHeightWithSpacing, (), float), asCALL_CDECL);

    // Columns (considered legacy in latest versions - superseded by Tables!)
    engine->RegisterGlobalFunction("void Columns(int = 1, const string&in = string(), bool = true)", asFUNCTIONPR([](int a, const string& b, bool c) {  
        ImGui::Columns(a, b.c_str(), c);  }, (int, const string&, bool), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void NextColumn()", asFUNCTIONPR([]() {  ImGui::NextColumn();  }, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("int GetColumnIndex()", asFUNCTIONPR([]() {  return ImGui::GetColumnIndex();  }, (), int), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetColumnWidth(int = -1)", asFUNCTIONPR([](int a) {  return ImGui::GetColumnWidth(a);  }, (int), float), asCALL_CDECL);
  //  engine->RegisterGlobalFunction("void SetColumnWidth(int, float)", asFUNCTIONPR([](int a, float b) {  ImGui::SetColumnWidth(a, b);  }, (int,float), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetColumnOffset(int = -1)", asFUNCTIONPR([](int a) {  return ImGui::GetColumnOffset(a);  }, (int), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetColumnOffset(int, float)", asFUNCTIONPR([](int a, float b) {  ImGui::SetColumnOffset(a, b);  }, (int,float), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("int GetColumnsCount()", asFUNCTIONPR([]() {  return ImGui::GetColumnsCount(); }, (), int), asCALL_CDECL);


    // Tab bars, tabs
    engine->RegisterGlobalFunction("bool BeginTabBar(const string&in, int = 0)", asFUNCTIONPR([](const string& str_id, ImGuiTabBarFlags flags) { return ImGui::BeginTabBar(str_id.c_str(), flags); }, (const string&, ImGuiTabBarFlags), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("void EndTabBar()", asFUNCTION(ImGui::EndTabBar), asCALL_CDECL);
    // BeginTabItem() without X close button.
    engine->RegisterGlobalFunction("bool BeginTabItem(const string&in, int = 0)", asFUNCTIONPR([](const string& label, ImGuiTabItemFlags flags) { return ImGui::BeginTabItem(label.c_str(), nullptr, flags); }, (const string&,  ImGuiTabItemFlags), bool), asCALL_CDECL);
    // BeginTabItem() with X close button.
    engine->RegisterGlobalFunction("bool BeginTabItem(const string&in, bool&inout, int = 0)", asFUNCTIONPR([](const string& label, bool& p_open, ImGuiTabItemFlags flags) { return ImGui::BeginTabItem(label.c_str(), &p_open, flags); }, (const string&, bool&, ImGuiTabItemFlags), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("void EndTabItem()", asFUNCTION(ImGui::EndTabItem), asCALL_CDECL);
    //engine->RegisterGlobalFunction("bool TabItemButton(const string&in, ImGuiTabItemFlags = 0)", asFUNCTIONPR([](const string& label, ImGuiTabItemFlags flags) { return ImGui::TabItemButton(label.c_str(), flags); }, (const string&, ImGuiTabItemFlags), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetTabItemClosed(const string&in)", asFUNCTIONPR([](const string& tab_or_docked_window_label) { ImGui::SetTabItemClosed(tab_or_docked_window_label.c_str()); }, (const string&), void), asCALL_CDECL);


    // ID scopes
    engine->RegisterGlobalFunction("void PushID(const string&in)", asFUNCTIONPR([](const string& n) { ImGui::PushID(n.c_str()); }, (const string&), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void PushID(int int_id)", asFUNCTIONPR([](int id) { ImGui::PushID(id); }, (int), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void PopID()", asFUNCTIONPR(ImGui::PopID, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("uint GetID(const string&in)", asFUNCTIONPR([](const string& n) { return ImGui::GetID(n.c_str()); }, (const string&), unsigned), asCALL_CDECL);

    // Widgets: Text
    engine->RegisterGlobalFunction("void Text(const string&in)", asFUNCTIONPR([](const string& n) {
        ImGui::Text(n.c_str());
    }, (const string&), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void TextDisabled(const string&in)", asFUNCTIONPR([](const string& n) {
        ImGui::TextDisabled(n.c_str());
    }, (const string&), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void TextColored(color col, const string&in)", asFUNCTIONPR([](ColourValue c, const string& n) {
        ImGui::TextColored(ImVec4(c.r, c.g, c.b, c.a), n.c_str());
    }, (ColourValue, const string&), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void TextWrapped(const string&in)", asFUNCTIONPR([](const string& n) {
        ImGui::TextWrapped(n.c_str()); }, (const string&), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void LabelText(const string&in, const string&in)", asFUNCTIONPR([](const string& l, const string& n) { 
        ImGui::LabelText(l.c_str(), n.c_str()); }, (const string&, const string&), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void Bullet()", asFUNCTIONPR(ImGui::Bullet, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void BulletText(const string&in)", asFUNCTIONPR([](const string& n) { 
        ImGui::BulletText(n.c_str()); }, (const string&), void), asCALL_CDECL);

    // Widgets: Main
    engine->RegisterGlobalFunction("bool Button(const string&in, vector2 = vector2(0,0))", asFUNCTIONPR([](const string& n, Vector2 v) {
        return ImGui::Button(n.c_str(), ImVec2(v.x, v.y));
    }, (const string&, Vector2), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool SmallButton(const string&in)", asFUNCTIONPR([](const string& n) { 
        return ImGui::SmallButton(n.c_str()); }, (const string&), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool InvisibleButton(const string&in, vector2)", asFUNCTIONPR([](const string& id, Vector2 v) { 
        return ImGui::InvisibleButton(id.c_str(), ImVec2(v.x, v.y)); }, (const string&, Vector2), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("void Image(const Ogre::TexturePtr&in, vector2)", asFUNCTIONPR([](Ogre::TexturePtr const& tex, Vector2 v) {
        ImGui::Image((ImTextureID)tex->getHandle(), ImVec2(v.x, v.y));
    }, (Ogre::TexturePtr const&, Vector2), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool Checkbox(const string&in, bool&inout)", asFUNCTIONPR([](const string& n, bool& v) { 
        return ImGui::Checkbox(n.c_str(), &v); }, (const string&, bool&), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool CheckboxFlags(const string&in, uint&inout, uint)", asFUNCTIONPR([](const string& n, unsigned& f, unsigned v) { 
        return ImGui::CheckboxFlags(n.c_str(), &f, v); }, (const string&, unsigned&, unsigned), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool RadioButton(const string&in, bool)", asFUNCTIONPR([](const string& n, bool v) { 
        return ImGui::RadioButton(n.c_str(), v); }, (const string&, bool), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool RadioButton(const string&in, int&inout, int)", asFUNCTIONPR([](const string& n, int& v, int vv) { 
        return ImGui::RadioButton(n.c_str(), &v, vv); }, (const string&, int&, int), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("void ProgressBar(float, vector2=vector2(-1,0), const string&in = \"\")", asFUNCTIONPR([](float v, Ogre::Vector2 size, const string& overlay) { 
        ImGui::ProgressBar(v, ImVec2(size.x, size.y), (overlay != "")?overlay.c_str():nullptr); }, (float,  Ogre::Vector2, const string&), void), asCALL_CDECL);


    // Widgets: Combo Box
 //   engine->RegisterGlobalFunction("bool BeginCombo(const string&in, const string&in, int = 0)", asFUNCTIONPR([](const string& id, const string& prevItem, int flags) { return ImGui::BeginCombo(id.c_str(), prevItem.c_str(), flags); }, (const string&, const string&, int), bool), asCALL_CDECL);
 //   engine->RegisterGlobalFunction("void EndCombo()", asFUNCTIONPR(ImGui::EndCombo, (), void), asCALL_CDECL);
 /*
    static char imgui_comboItem[4096];
    engine->RegisterGlobalFunction("bool Combo(const string&in, int&inout, const Array<string>@+)", asFUNCTIONPR([](const string& lbl, int& index, const CScriptArray* items) {
        memset(imgui_comboItem, 0, sizeof(char) * 4096);
        unsigned offset = 0;
        for (unsigned i = 0; i < items->GetSize(); ++i)
        {
            string* str = ((string*)items->At(i));
            strcpy(imgui_comboItem + offset, str->c_str());
            offset += str->length() + 1;
        }
        return ImGui::Combo(lbl.c_str(), &index, imgui_comboItem, -1);
    }, (const string&, int&, const CScriptArray*), bool), asCALL_CDECL);
    */
    // Widgets: Drags 
    engine->RegisterGlobalFunction("bool DragFloat(const string&in, float&inout, float = 1.0f, float = 0.0f, float = 0.0f)", asFUNCTIONPR([](const string& n, float& v, float speed, float mn, float mx) { 
        return ImGui::DragFloat(n.c_str(), &v, speed, mn, mx); }, (const string&, float&, float, float, float), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool DragFloat2(const string&in, vector2&inout)", asFUNCTIONPR([](const string& n, Vector2& v) { 
        return ImGui::DragFloat2(n.c_str(), &v.x); }, (const string&, Vector2&), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool DragFloat3(const string&in, vector3&inout)", asFUNCTIONPR([](const string& n, Vector3& v) { 
        return ImGui::DragFloat3(n.c_str(), &v.x); }, (const string&, Vector3&), bool), asCALL_CDECL);
    /* --- TODO: Register Vector4
    engine->RegisterGlobalFunction("bool DragFloat4(const string&in, Vector4&inout)", asFUNCTIONPR([](const string& n, Vector4& v) { 
        return ImGui::DragFloat4(n.c_str(), &v.x); }, (const string&, Vector4&), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool DragInt(const string&in, int&inout, int = 0, int = 0)", asFUNCTIONPR([](const string& n, int& v, int mn, int mx) { 
        return ImGui::DragInt(n.c_str(), &v, 1.0f, mn, mx); }, (const string&, int&, int, int), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool DragInt2(const string&in, IntVector2&inout, int = 0, int = 0)", asFUNCTIONPR([](const string& n, IntVector2& v, int mn, int mx) { 
        return ImGui::DragInt2(n.c_str(), &v.x, 1.0f, mn, mx); }, (const string&, IntVector2&, int,int), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool DragInt3(const string&in, IntVector3&inout, int = 0, int = 0)", asFUNCTIONPR([](const string& n, IntVector3& v, int mn, int mx) { 
        return ImGui::DragInt3(n.c_str(), &v.x, 1.0f, mn, mx); }, (const string&, IntVector3&, int,int), bool), asCALL_CDECL);
        */
    engine->RegisterGlobalFunction("bool DragFloatRange2(const string&in, float&inout, float&inout, float = 0.0f, float = 1.0f)", asFUNCTIONPR([](const string& n, float& v0, float&v1, float mn, float mx) { 
        return ImGui::DragFloatRange2(n.c_str(), &v0, &v1, 1.0f, mn, mx); }, (const string&, float&, float&, float, float), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool DragIntRange2(const string&in, int&inout, int&inout, int, int)", asFUNCTIONPR([](const string& n, int& v0, int&v1, int mn, int mx) { 
        return ImGui::DragIntRange2(n.c_str(), &v0, &v1, 1.0f, mn, mx); }, (const string&, int&, int&, int, int), bool), asCALL_CDECL);

    // Widgets: Input with Keyboard
    static char imgui_text_buffer[4096]; // shared with multiple widgets
    engine->RegisterGlobalFunction("bool InputText(const string&in, string&inout)", asFUNCTIONPR([](const string& id, string& val) {
        memset(imgui_text_buffer, 0, sizeof(char) * 4096);
        strcpy(imgui_text_buffer, val.c_str());
        if (ImGui::InputText(id.c_str(), imgui_text_buffer, 4096))
        {
            val = imgui_text_buffer;
            return true;
        }
        return false;
    }, (const string&, string&), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool InputTextMultiline(const string&in, string&inout, const vector2&in = vector2(0,0))", asFUNCTIONPR([](const string& id, string& val, const Ogre::Vector2& size) {
        return ImGui::InputTextMultiline(id.c_str(), (char*)val.data(), val.size(), ImVec2(size.x, size.y));
    }, (const string&, string&, const Ogre::Vector2&), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool InputFloat(const string&, float&inout)", asFUNCTIONPR([](const string& id, float& val) {
        return ImGui::InputFloat(id.c_str(), &val); }, (const string&, float&), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool InputFloat2(const string&, vector2&inout)", asFUNCTIONPR([](const string& id, Vector2& val) {
        return ImGui::InputFloat2(id.c_str(), &val.x); }, (const string&, Vector2&), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool InputFloat3(const string&, vector3&inout)", asFUNCTIONPR([](const string& id, Vector3& val) {
        return ImGui::InputFloat3(id.c_str(), &val.x); }, (const string&, Vector3&),bool), asCALL_CDECL);
    /* --- TODO: Register Vector4
    engine->RegisterGlobalFunction("bool InputFloat4(const string&, Vector4&inout)", asFUNCTIONPR([](const string& id, Vector4& val) {
        return ImGui::InputFloat4(id.c_str(), &val.x_); }, (const string&, Vector4&),bool), asCALL_CDECL);
        */
    engine->RegisterGlobalFunction("bool InputInt(const string&, int&inout)", asFUNCTIONPR([](const string& id, int& val) {
        return ImGui::InputInt(id.c_str(), &val); }, (const string&, int&), bool), asCALL_CDECL);
    /*
    engine->RegisterGlobalFunction("bool InputInt2(const string&, IntVector2&inout)", asFUNCTIONPR([](const string& id, IntVector2& val) {
        return ImGui::InputInt2(id.c_str(), &val.x_); }, (const string&, IntVector2&),bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool InputInt3(const string&, IntVector3&inout)", asFUNCTIONPR([](const string& id, IntVector3& val) {
        return ImGui::InputInt3(id.c_str(), &val.x_); }, (const string&, IntVector3&), bool), asCALL_CDECL);
        */

    // Widgets: Sliders (tip: ctrl+click on a slider to input with keyboard. manually input values aren't clamped, can go off-bounds)
    engine->RegisterGlobalFunction("bool SliderFloat(const string&in, float&inout, float = 0.0f, float = 0.0f)", asFUNCTIONPR([](const string& n, float& v, float mn, float mx) { 
        return ImGui::SliderFloat(n.c_str(), &v, mn, mx); }, (const string&, float&, float,float), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool SliderFloat2(const string&in, vector2&inout, float, float)", asFUNCTIONPR([](const string& n, Vector2& v, float mn, float mx) { 
        return ImGui::SliderFloat2(n.c_str(), &v.x, mn, mx); }, (const string&, Vector2&, float,float),bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool SliderFloat3(const string&in, vector3&inout, float, float)", asFUNCTIONPR([](const string& n, Vector3& v, float mn, float mx) { 
        return ImGui::SliderFloat3(n.c_str(), &v.x, mn, mx); }, (const string&, Vector3&, float,float),bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool SliderInt(const string&in, int&inout, int = 0, int = 0)", asFUNCTIONPR([](const string& n, int& v, int mn, int mx) {
        return ImGui::SliderInt(n.c_str(), &v, mn, mx); }, (const string&, int&, int, int), bool), asCALL_CDECL);
/* --- TODO: Register Vector4
    engine->RegisterGlobalFunction("bool SliderFloat4(const string&in, Vector4&inout, float, float)", asFUNCTIONPR([](const string& n, Vector4& v, float mn, float mx) { 
        return ImGui::SliderFloat4(n.c_str(), &v.x, mn, mx); }, (const string&, Vector4&,float,float),bool), asCALL_CDECL);
    
    engine->RegisterGlobalFunction("bool SliderInt2(const string&in, IntVector2&inout, int = 0, int = 0)", asFUNCTIONPR([](const string& n, IntVector2& v, int mn, int mx) { 
        return ImGui::SliderInt2(n.c_str(), &v.x, mn, mx); }, (const string&, IntVector2&, int,int),bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool SliderInt3(const string&in, IntVector3&inout, int = 0, int = 0)", asFUNCTIONPR([](const string& n, IntVector3& v, int mn, int mx) { 
        return ImGui::SliderInt3(n.c_str(), &v.x, mn, mx); }, (const string&, IntVector3&, int,int),bool), asCALL_CDECL);
        */

    // Widgets: Color Editor/Picker
    engine->RegisterGlobalFunction("bool ColorEdit3(const string&in, color&inout)", asFUNCTIONPR([](const string& id, ColourValue& val) {
        Vector3 v(val.r, val.g, val.b);
        if (ImGui::ColorEdit3(id.c_str(), &v.x))
        {
            val = ColourValue(v.x, v.y, v.z);
            return true;
        }
        return false;
    }, (const string&, ColourValue&),bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool ColorEdit4(const string&in, color&inout)", asFUNCTIONPR([](const string& id, ColourValue& val) {
        Vector4 v(val.r, val.g, val.b, val.a);
        if (ImGui::ColorEdit4(id.c_str(), &v.x))
        {
            val = ColourValue(v.x, v.y, v.z, v.w);
            return true;
        }
        return false;
    }, (const string&, ColourValue&),bool), asCALL_CDECL);
 /*   engine->RegisterGlobalFunction("bool ColorPicker3(const string&in, color&inout)", asFUNCTIONPR([](const string& id, ColourValue& val) {
        Vector3 v(val.r, val.g, val.b);
        if (ImGui::ColorPicker3(id.c_str(), &v.x))
        {
            val = ColourValue(v.x, v.y, v.z);
            return true;
        }
        return false;
    }, (const string&, ColourValue&),bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool ColorPicker4(const string&in, color&inout)", asFUNCTIONPR([](const string& id, ColourValue& val) {
        Vector4 v(val.r, val.g, val.b, val.a);
        if (ImGui::ColorPicker4(id.c_str(), &v.x))
        {
            val = ColourValue(v.x, v.y, v.z, v.w);
            return true;
        }
        return false;
    }, (const string&, ColourValue&),bool), asCALL_CDECL);*/
    engine->RegisterGlobalFunction("bool ColorButton(const string&in, color)", asFUNCTIONPR([](const string& id, ColourValue val) {
        Vector4 v(val.r, val.g, val.b, val.a);
        ImVec4 vv(v.x, v.y, v.z, v.w);
        return ImGui::ColorButton(id.c_str(), vv);
    }, (const string&, ColourValue), bool), asCALL_CDECL);

    // Widgets: Trees
    engine->RegisterGlobalFunction("bool TreeNode(const string&in)", asFUNCTIONPR([](const string& id) { return ImGui::TreeNode(id.c_str()); }, (const string&), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("void TreePush(const string&in)", asFUNCTIONPR([](const string& id) { ImGui::TreePush(id.c_str()); }, (const string&), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void TreePop()", asFUNCTIONPR(ImGui::TreePop, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void TreeAdvanceToLabelPos()", asFUNCTIONPR(ImGui::TreeAdvanceToLabelPos, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetTreeNodeToLabelSpacing()", asFUNCTIONPR(ImGui::GetTreeNodeToLabelSpacing, (), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetNextTreeNodeOpen(bool)", asFUNCTIONPR([](bool val) { ImGui::SetNextTreeNodeOpen(val); }, (bool), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool CollapsingHeader(const string&in)", asFUNCTIONPR([](const string& n) { return ImGui::CollapsingHeader(n.c_str()); }, (const string&), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool CollapsingHeader(const string&in, bool&inout)", asFUNCTIONPR([](const string& n, bool& v) { return ImGui::CollapsingHeader(n.c_str(), &v); }, (const string&, bool&), bool), asCALL_CDECL);

    // Widgets: Selectable / Lists
    engine->RegisterGlobalFunction("bool Selectable(const string&in, bool = false)", asFUNCTIONPR([](const string& n, bool v) { return ImGui::Selectable(n.c_str(), v); }, (const string&, bool), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool ListBoxHeader(const string&in)", asFUNCTIONPR([](const string& n) { return ImGui::ListBoxHeader(n.c_str()); }, (const string&), bool), asCALL_CDECL);
        
    // Values
    engine->RegisterGlobalFunction("void Value(const string&in, bool)", asFUNCTIONPR([](const string& n, bool v) { ImGui::Value(n.c_str(), v);    }, (const string&, bool), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void Value(const string&in, int)", asFUNCTIONPR([](const string& n, int v) { ImGui::Value(n.c_str(), v);      }, (const string&, int), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void Value(const string&in, uint)", asFUNCTIONPR([](const string& n, unsigned v) { ImGui::Value(n.c_str(), v);}, (const string&, unsigned), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void Value(const string&in, float)", asFUNCTIONPR([](const string& n, float v) { ImGui::Value(n.c_str(), v);  }, (const string&, float), void), asCALL_CDECL);

    // Tooltips
    engine->RegisterGlobalFunction("void BeginTooltip()", asFUNCTIONPR(ImGui::BeginTooltip  , (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void EndTooltip()", asFUNCTIONPR(ImGui::EndTooltip      , (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetTooltip(const string&in)", asFUNCTIONPR([](const string& t) { ImGui::SetTooltip(t.c_str()); }, (const string&), void), asCALL_CDECL);

    // Menus
    engine->RegisterGlobalFunction("bool BeginMainMenuBar()", asFUNCTIONPR([]() {  return ImGui::BeginMainMenuBar();  }, (), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("void EndMainMenuBar()", asFUNCTIONPR([]() {  ImGui::EndMainMenuBar();  }, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool BeginMenuBar()", asFUNCTIONPR([]() {  return ImGui::BeginMenuBar();  }, (), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("void EndMenuBar()", asFUNCTIONPR([]() {  ImGui::EndMenuBar();  }, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool BeginMenu(const string&in, bool = true)", asFUNCTIONPR([](const string& a, bool b) {  
        return ImGui::BeginMenu(a.c_str(), b);  }, (const string&, bool), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("void EndMenu()", asFUNCTIONPR([]() {  ImGui::EndMenu();  }, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool MenuItem(const string&in, const string&in = string(), bool = false, bool = true)", asFUNCTIONPR([](const string& a, const string& b, bool c, bool d) {  
        return ImGui::MenuItem(a.c_str(), b.c_str(), c, d);  }, (const string&, const string&, bool, bool), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool MenuItem(const string&in, const string&in, bool &inout, bool = true)", asFUNCTIONPR([](const string& a, const string& b, bool& c, bool d) {  
        return ImGui::MenuItem(a.c_str(), b.c_str(), &c, d);  }, (const string&, const string&, bool&, bool), bool), asCALL_CDECL);

    // Popups
    engine->RegisterGlobalFunction("void OpenPopup(const string&in)", asFUNCTIONPR([](const string& a) {  ImGui::OpenPopup(a.c_str());  }, (const string&), void), asCALL_CDECL);
   /* engine->RegisterGlobalFunction("bool BeginPopup(const string&in, int = 0)", asFUNCTIONPR([](const string& a, int b) {  
        return ImGui::BeginPopup(a.c_str(), (ImGuiWindowFlags)b);  }, (const string&, int), bool), asCALL_CDECL);*/ // FIXME: update imgui!
    engine->RegisterGlobalFunction("bool BeginPopup(const string&in, int = 0)", asFUNCTIONPR([](const string& a, int b) {  
        return ImGui::BeginPopup(a.c_str());  }, (const string&, int), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool BeginPopupContextItem(const string&in = string(), int = 1)", asFUNCTIONPR([](const string& a, int b) {  
        return ImGui::BeginPopupContextItem(a.c_str(), b);  }, (const string&, int), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool BeginPopupContextWindow(const string&in = string(), int = 1, bool = true)", asFUNCTIONPR([](const string& a, int b, bool c) {  
        return ImGui::BeginPopupContextWindow(a.c_str(), b, c);  }, (const string&, int, bool), bool), asCALL_CDECL); // FIXME: update imgui! -- swapped args
    engine->RegisterGlobalFunction("bool BeginPopupContextVoid(const string&in = string(), int = 1)", asFUNCTIONPR([](const string& a, int b) {  
        return ImGui::BeginPopupContextVoid(a.c_str(), b);  }, (const string&, int), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool BeginPopupModal(const string&in, bool &inout = null, int = 0)", asFUNCTIONPR([](const string& a, bool& b, int c) {  
        return ImGui::BeginPopupModal(a.c_str(), &b, (ImGuiWindowFlags)c);  }, (const string&, bool&, int), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("void EndPopup()", asFUNCTIONPR([]() {  ImGui::EndPopup();  }, (), void), asCALL_CDECL);
/*    engine->RegisterGlobalFunction("bool OpenPopupOnItemClick(const string&in = string(), int = 1)", asFUNCTIONPR([](const string& a, int b) {  
        return ImGui::OpenPopupOnItemClick(a.c_str(), b);  }, (const string&, int), bool), asCALL_CDECL);*/ // FIXME: update imgui!
 /*   engine->RegisterGlobalFunction("bool IsPopupOpen(const string&in)", asFUNCTIONPR([](const string& a) {  
        return ImGui::IsPopupOpen(a.c_str());  }, (const string&), bool), asCALL_CDECL); */ // FIXME: update imgui!
    engine->RegisterGlobalFunction("void CloseCurrentPopup()", asFUNCTIONPR([]() {  ImGui::CloseCurrentPopup();  }, (), void), asCALL_CDECL);

    // Clip-rects
    engine->RegisterGlobalFunction("void PushClipRect(const vector2&, const vector2&, bool)", asFUNCTIONPR([](const Vector2& a, const Vector2& b, bool c) {
        ImGui::PushClipRect(ImVec2(a.x, a.y), ImVec2(b.x, b.y), c);  }, (const Vector2&, const Vector2&, bool), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void PopClipRect()", asFUNCTIONPR([]() {  ImGui::PopClipRect();  }, (), void), asCALL_CDECL);

    // Focus
 /*   engine->RegisterGlobalFunction("void SetItemDefaultFocus()", asFUNCTIONPR([]() {  ImGui::SetItemDefaultFocus();  }, (), void), asCALL_CDECL); */ // TODO update imgui
    engine->RegisterGlobalFunction("void SetKeyboardFocusHere(int = 0)", asFUNCTIONPR([](int a) {  ImGui::SetKeyboardFocusHere(a); }, (int), void), asCALL_CDECL);

    // Utilities
    engine->RegisterGlobalFunction("bool IsItemHovered(int = 0)", asFUNCTIONPR([](int a) {  return ImGui::IsItemHovered();  }, (int), bool), asCALL_CDECL); // TODO: update imgui -- flags omitted
    engine->RegisterGlobalFunction("bool IsItemActive()", asFUNCTIONPR([]() {  return ImGui::IsItemActive();  }, (), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsItemClicked(int = 0)", asFUNCTIONPR([](int a) {  return ImGui::IsItemClicked(a);  }, (int), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsItemVisible()", asFUNCTIONPR([]() {  return ImGui::IsItemVisible();  }, (), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsAnyItemHovered()", asFUNCTIONPR([]() {  return ImGui::IsAnyItemHovered();  }, (), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsAnyItemActive()", asFUNCTIONPR([]() {  return ImGui::IsAnyItemActive();  }, (), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetItemRectMin()", asFUNCTIONPR([]() {  auto v = ImGui::GetItemRectMin(); return Vector2(v.x, v.y); }, (), Vector2), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetItemRectMax()", asFUNCTIONPR([]() {  auto v = ImGui::GetItemRectMax(); return Vector2(v.x, v.y); }, (), Vector2), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetItemRectSize()", asFUNCTIONPR([]() {  auto v = ImGui::GetItemRectSize(); return Vector2(v.x, v.y); }, (), Vector2), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetItemAllowOverlap()", asFUNCTIONPR([]() {  ImGui::SetItemAllowOverlap();  }, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsWindowFocused(int = 0)", asFUNCTIONPR([](int a) {  return ImGui::IsWindowFocused(); }, (int), bool), asCALL_CDECL); // TODO: update imgui -- flags omitted
    engine->RegisterGlobalFunction("bool IsWindowHovered(int = 0)", asFUNCTIONPR([](int a) {  return ImGui::IsWindowHovered(); }, (int), bool), asCALL_CDECL); // TODO: update imgui -- flags omitted
    engine->RegisterGlobalFunction("bool IsRectVisible(const vector2&)", asFUNCTIONPR([](const Vector2& a) {  return ImGui::IsRectVisible(ImVec2(a.x, a.y)); }, (const Vector2&), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsRectVisible(const vector2&, const vector2&)", asFUNCTIONPR([](const Vector2& a, const Vector2& b) {  return ImGui::IsRectVisible(ImVec2(a.x, a.y), ImVec2(b.x, b.y));  }, (const Vector2&, const Vector2&), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetTime()", asFUNCTIONPR([]() {  return (float)ImGui::GetTime();  }, (), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("int GetFrameCount()", asFUNCTIONPR([]() {  return ImGui::GetFrameCount();  }, (), int), asCALL_CDECL);

    engine->RegisterGlobalFunction("vector2 CalcTextSize(const string&in, bool hide_text_after_double_hash = false, float wrap_width = -1.0f)", asFUNCTIONPR([](const string& a, bool c, float d) {  
        auto v = ImGui::CalcTextSize(a.c_str(), nullptr, c, d); return Vector2(v.x, v.y); }, (const string&,  bool, float), Vector2), asCALL_CDECL);
    engine->RegisterGlobalFunction("void CalcListClipping(int, float, int&inout, int&inout)", asFUNCTIONPR([](int a, float b, int& c, int& d) {  
        ImGui::CalcListClipping(a, b, &c, &d);  }, (int,float,int&,int&), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool BeginChildFrame(uint, const vector2&, int = 0)", asFUNCTIONPR([](unsigned a, const Vector2& b, int c) {  
        return ImGui::BeginChildFrame(a, ImVec2(b.x,b.y), (ImGuiWindowFlags)c);  }, (unsigned, const Vector2&, int), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("void EndChildFrame()", asFUNCTIONPR([]() {  ImGui::EndChildFrame();  }, (), void), asCALL_CDECL);

    engine->RegisterGlobalFunction("int GetKeyIndex(int)", asFUNCTIONPR([](int a) {  return ImGui::GetKeyIndex((ImGuiKey)a);  }, (int), int), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsKeyDown(int)", asFUNCTIONPR([](int a) {  return ImGui::IsKeyDown(a);  }, (int), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsKeyPressed(int, bool = true)", asFUNCTIONPR([](int a, bool b) {  return ImGui::IsKeyPressed(a, b);  }, (int,bool), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsKeyReleased(int)", asFUNCTIONPR([](int a) {  return ImGui::IsKeyReleased(a);  }, (int), bool), asCALL_CDECL);
    /*engine->RegisterGlobalFunction("int GetKeyPressedAmount(int, float, float)", asFUNCTIONPR([](int a, float b, float c) {  return ImGui::GetKeyPressedAmount(a, b, c);  }, (int,float,float), int), asCALL_CDECL);*/  // FIXME update imgui
    engine->RegisterGlobalFunction("bool IsMouseDown(int)", asFUNCTIONPR([](int a) {  return ImGui::IsMouseDown(a);  }, (int), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsMouseClicked(int, bool = false)", asFUNCTIONPR([](int a, bool b) {  return ImGui::IsMouseClicked(a, b); }, (int,bool), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsMouseDoubleClicked(int)", asFUNCTIONPR([](int a) {  return ImGui::IsMouseDoubleClicked(a); }, (int), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsMouseReleased(int)", asFUNCTIONPR([](int a) {  return ImGui::IsMouseReleased(a);  }, (int), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsMouseDragging(int = 0, float = -1.0f)", asFUNCTIONPR([](int a, float b) {  return ImGui::IsMouseDragging(a, b);  }, (int, float), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsMouseHoveringRect(const vector2&in, const vector2&in, bool = true)", asFUNCTIONPR([](const Vector2& a, const Vector2& b, bool c) { return ImGui::IsMouseHoveringRect(ImVec2(a.x, a.y), ImVec2(b.x, b.y), c);  }, (const Vector2&, const Vector2&, bool), bool), asCALL_CDECL);
  /*  engine->RegisterGlobalFunction("bool IsMousePosValid(const Vector2&in)", asFUNCTIONPR([](const Vector2& a) {  auto v = ImVec2(a.x, a.y);  return ImGui::IsMousePosValid(&v);  }, (const Vector2&), bool), asCALL_CDECL); */ // FIXME update imgui
    engine->RegisterGlobalFunction("vector2 GetMousePos()", asFUNCTIONPR([]() {  auto v = ImGui::GetMousePos(); return Vector2(v.x, v.y); }, (), Vector2), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetMousePosOnOpeningCurrentPopup()", asFUNCTIONPR([]() {  auto v = ImGui::GetMousePosOnOpeningCurrentPopup(); return Vector2(v.x, v.y); }, (), Vector2), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetMouseDragDelta(int = 0, float = -1.0f)", asFUNCTIONPR([](int a, float b) {  auto v = ImGui::GetMouseDragDelta(a, b); return Vector2(v.x, v.y); }, (int,float), Vector2), asCALL_CDECL);
    engine->RegisterGlobalFunction("void ResetMouseDragDelta(int = 0)", asFUNCTIONPR([](int a) {  ImGui::ResetMouseDragDelta(a); }, (int), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("int GetMouseCursor()", asFUNCTIONPR([]() {  return ImGui::GetMouseCursor();  }, (), int), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetMouseCursor(int)", asFUNCTIONPR([](ImGuiMouseCursor a) {  ImGui::SetMouseCursor(a);  }, (int), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void CaptureKeyboardFromApp(bool = true)", asFUNCTIONPR([](bool a) {  ImGui::CaptureKeyboardFromApp(a);  }, (bool), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void CaptureMouseFromApp(bool = true)", asFUNCTIONPR([](bool a) {  ImGui::CaptureMouseFromApp(a);  }, (bool), void), asCALL_CDECL);


    engine->RegisterGlobalFunction("string GetClipboardText()", asFUNCTIONPR([]() { return string(ImGui::GetClipboardText());  }, (), string), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetClipboardText(const string&in)", asFUNCTIONPR([](const string& a) {  ImGui::SetClipboardText(a.c_str());  }, (const string&), void), asCALL_CDECL);

    // Data plotting - we wrap the 'getter func' variant to resemble the 'float*' variant.
    //                                   PlotLines(const char* label, const float* values, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0, 0), int stride = sizeof(float));
    engine->RegisterGlobalFunction("void PlotLines(const string&in label, array<float>&in values, int values_count, int values_offset = 0, const string&in overlay_text = string(), float scale_min = FLT_MAX, float scale_max = FLT_MAX, vector2 graph_size = vector2(0,0))",
                                    asFUNCTIONPR([](const string& label, CScriptArray* values, int values_count, int values_offset, const string& overlay_text, float scale_min, float scale_max, Vector2 graph_size)
                                        { ImGui::PlotLines(label.c_str(), &ImGuiPlotLinesScriptValueGetterFunc, values, values_count, values_offset, overlay_text.c_str(), scale_min, scale_max, ImVec2(graph_size.x, graph_size.y)); },
                                            (const string&, CScriptArray*, int, int, const string&, float, float, Vector2), void), asCALL_CDECL);

    engine->SetDefaultNamespace("");
}


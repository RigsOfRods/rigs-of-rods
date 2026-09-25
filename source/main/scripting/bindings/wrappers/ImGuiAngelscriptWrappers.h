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
/// Helper/wrapper functions shared by native (ImGuiAngelscript.cpp)
/// and generic (ImGuiAngelscriptGeneric.cpp) DearIMGUI bindings.
/// Native bindings register these directly (asCALL_CDECL/asCALL_CDECL_OBJFIRST),
/// generic bindings wrap them using WRAP_FN()/WRAP_OBJ_FIRST().

#pragma once

#include "OgreImGui.h"
#include "scriptarray/scriptarray.h"

#include <angelscript.h>
#include <Ogre.h>
#include <string>

namespace ImGuiAngelscriptWrappers {

using namespace AngelScript;
using namespace Ogre;
using std::string;

static float ImGuiPlotLinesScriptValueGetterFunc(void* data, int index)
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

// ---------------------------------------------------------------------------
// ImDrawList
// ---------------------------------------------------------------------------

static void ImDrawList_AddLine(ImDrawList* drawlist, Ogre::Vector2 const& p1, Ogre::Vector2 const& p2, Ogre::ColourValue const& col, float thickness)
{
    drawlist->AddLine(ImVec2(p1.x, p1.y), ImVec2(p2.x, p2.y), ImColor(col.r, col.g, col.b, col.a), thickness);
}

static void ImDrawList_AddTriangle(ImDrawList* drawlist, Ogre::Vector2 const& p1, Ogre::Vector2 const& p2, Ogre::Vector2 const& p3, Ogre::ColourValue const& col, float thickness)
{
    drawlist->AddTriangle(ImVec2(p1.x, p1.y), ImVec2(p2.x, p2.y), ImVec2(p3.x, p3.y), ImColor(col.r, col.g, col.b, col.a), thickness);
}

static void ImDrawList_AddTriangleFilled(ImDrawList* drawlist, Ogre::Vector2 const& p1, Ogre::Vector2 const& p2, Ogre::Vector2 const& p3, Ogre::ColourValue const& col)
{
    drawlist->AddTriangleFilled(ImVec2(p1.x, p1.y), ImVec2(p2.x, p2.y), ImVec2(p3.x, p3.y), ImColor(col.r, col.g, col.b, col.a));
}

static void ImDrawList_AddRect(ImDrawList* drawlist, Ogre::Vector2 const& p1, Ogre::Vector2 const& p2, Ogre::ColourValue const& col, float rounding, int rounding_corners, float thickness)
{
    drawlist->AddRect(ImVec2(p1.x, p1.y), ImVec2(p2.x, p2.y), ImColor(col.r, col.g, col.b, col.a), rounding, rounding_corners, thickness);
}

static void ImDrawList_AddRectFilled(ImDrawList* drawlist, Ogre::Vector2 const& p1, Ogre::Vector2 const& p2, Ogre::ColourValue const& col, float rounding, int rounding_corners)
{
    drawlist->AddRectFilled(ImVec2(p1.x, p1.y), ImVec2(p2.x, p2.y), ImColor(col.r, col.g, col.b, col.a), rounding, rounding_corners);
}

static void ImDrawList_AddCircle(ImDrawList* drawlist, Ogre::Vector2 const& center, float radius, Ogre::ColourValue const& col, int num_segments, float thickness)
{
    drawlist->AddCircle(ImVec2(center.x, center.y), radius, ImColor(col.r, col.g, col.b, col.a), num_segments, thickness);
}

static void ImDrawList_AddCircleFilled(ImDrawList* drawlist, Ogre::Vector2 const& center, float radius, Ogre::ColourValue const& col, int num_segments)
{
    drawlist->AddCircleFilled(ImVec2(center.x, center.y), radius, ImColor(col.r, col.g, col.b, col.a), num_segments);
}

static void ImDrawList_AddText(ImDrawList* drawlist, Ogre::Vector2 const& pos, Ogre::ColourValue const& col, std::string const& text)
{
    drawlist->AddText(ImVec2(pos.x, pos.y), ImColor(col.r, col.g, col.b, col.a), text.c_str());
}

static void ImDrawList_AddImage(ImDrawList* drawlist, Ogre::TexturePtr const& tex, Ogre::Vector2 const& p_min, Ogre::Vector2 const& p_max, Ogre::Vector2 const& uv_min, Ogre::Vector2 const& uv_max, Ogre::ColourValue const& col)
{
    drawlist->AddImage((ImTextureID)tex->getHandle(), ImVec2(p_min.x, p_min.y), ImVec2(p_max.x, p_max.y), ImVec2(uv_min.x, uv_min.y), ImVec2(uv_max.x, uv_max.y), ImColor(col.r, col.g, col.b, col.a));
}

// ---------------------------------------------------------------------------
// Windows
// ---------------------------------------------------------------------------

static bool ImGui_Begin(const string& name, bool& opened, int flags)
{
    return ImGui::Begin(name.c_str(), &opened, flags);
}

static bool ImGui_BeginChild(const string& name, const Ogre::Vector2& size, bool border, int flags)
{
    return ImGui::BeginChild(name.c_str(), ImVec2(size.x, size.y), border, flags);
}

static bool ImGui_BeginChildById(ImGuiID id, const Ogre::Vector2& size, bool border, int flags)
{
    return ImGui::BeginChild(id, ImVec2(size.x, size.y), border, flags);
}

static void ImGui_PushStyleVarFloat(int index, float val)
{
    ImGui::PushStyleVar(index, val);
}

static void ImGui_PushStyleVarVector2(int index, const Ogre::Vector2& val)
{
    ImGui::PushStyleVar(index, ImVec2(val.x, val.y));
}

static void ImGui_PushStyleColor(int index, Ogre::ColourValue const& col)
{
    ImGui::PushStyleColor(index, (ImU32)ImColor(col.r, col.g, col.b, col.a));
}

static Vector2 ImGui_GetContentRegionMax()
{
    auto v = ImGui::GetContentRegionMax(); return Vector2(v.x, v.y);
}

static Vector2 ImGui_GetContentRegionAvail()
{
    auto v = ImGui::GetContentRegionAvail(); return Vector2(v.x, v.y);
}

static Vector2 ImGui_GetWindowContentRegionMin()
{
    auto v = ImGui::GetWindowContentRegionMin(); return Vector2(v.x, v.y);
}

static Vector2 ImGui_GetWindowContentRegionMax()
{
    auto v = ImGui::GetWindowContentRegionMax(); return Vector2(v.x, v.y);
}

static Vector2 ImGui_GetWindowPos()
{
    auto v = ImGui::GetWindowPos(); return Vector2(v.x, v.y);
}

static Vector2 ImGui_GetWindowSize()
{
    auto v = ImGui::GetWindowSize(); return Vector2(v.x, v.y);
}

static void ImGui_SetNextWindowPos(Vector2 v, int flags, Vector2 pivot)
{
    ImGui::SetNextWindowPos(ImVec2(v.x, v.y), flags, ImVec2(pivot.x, pivot.y));
}

static void ImGui_SetNextWindowSize(Vector2 v)
{
    ImGui::SetNextWindowSize(ImVec2(v.x, v.y));
}

static void ImGui_SetNextWindowContentSize(Vector2 v)
{
    ImGui::SetNextWindowContentSize(ImVec2(v.x, v.y));
}

static void ImGui_SetNextWindowCollapsed(bool v)
{
    ImGui::SetNextWindowCollapsed(v);
}

static void ImGui_SetNextWindowFocus()
{
    ImGui::SetNextWindowFocus();
}

static void ImGui_SetWindowPos(Vector2 v)
{
    ImGui::SetWindowPos(ImVec2(v.x, v.y));
}

static void ImGui_SetWindowSize(Vector2 v)
{
    ImGui::SetWindowSize(ImVec2(v.x, v.y));
}

static void ImGui_SetWindowCollapsed(bool v)
{
    ImGui::SetWindowCollapsed(v);
}

static void ImGui_SetWindowFocus()
{
    ImGui::SetWindowFocus();
}

static void ImGui_SetWindowPosByName(const string& name, Vector2 v)
{
    ImGui::SetWindowPos(name.c_str(), ImVec2(v.x, v.y));
}

static void ImGui_SetWindowSizeByName(const string& name, Vector2 v)
{
    ImGui::SetWindowSize(name.c_str(), ImVec2(v.x, v.y));
}

static void ImGui_SetWindowCollapsedByName(const string& name, bool v)
{
    ImGui::SetWindowCollapsed(name.c_str(), v);
}

static void ImGui_SetWindowFocusByName(const string& v)
{
    ImGui::SetWindowFocus(v.c_str());
}

// ---------------------------------------------------------------------------
// Cursor / Layout
// ---------------------------------------------------------------------------

static void ImGui_Dummy(Vector2 v)
{
    ImGui::Dummy(ImVec2(v.x, v.y));
}

static Vector2 ImGui_GetCursorPos()
{
    auto v = ImGui::GetCursorPos(); return Vector2(v.x, v.y);
}

static void ImGui_SetCursorPos(Vector2 v)
{
    ImGui::SetCursorPos(ImVec2(v.x, v.y));
}

static Vector2 ImGui_GetCursorStartPos()
{
    auto v = ImGui::GetCursorStartPos(); return Vector2(v.x, v.y);
}

static Vector2 ImGui_GetCursorScreenPos()
{
    auto v = ImGui::GetCursorScreenPos(); return Vector2(v.x, v.y);
}

static void ImGui_SetCursorScreenPos(Vector2 v)
{
    ImGui::SetCursorScreenPos(ImVec2(v.x, v.y));
}

// ---------------------------------------------------------------------------
// Columns (considered legacy in latest versions - superseded by Tables!)
// ---------------------------------------------------------------------------

static void ImGui_Columns(int a, const string& b, bool c)
{
    ImGui::Columns(a, b.c_str(), c);
}

static void ImGui_NextColumn()
{
    ImGui::NextColumn();
}

static int ImGui_GetColumnIndex()
{
    return ImGui::GetColumnIndex();
}

static float ImGui_GetColumnWidth(int a)
{
    return ImGui::GetColumnWidth(a);
}

static float ImGui_GetColumnOffset(int a)
{
    return ImGui::GetColumnOffset(a);
}

static void ImGui_SetColumnOffset(int a, float b)
{
    ImGui::SetColumnOffset(a, b);
}

static int ImGui_GetColumnsCount()
{
    return ImGui::GetColumnsCount();
}

// ---------------------------------------------------------------------------
// Tab bars, tabs
// ---------------------------------------------------------------------------

static bool ImGui_BeginTabBar(const string& str_id, ImGuiTabBarFlags flags)
{
    return ImGui::BeginTabBar(str_id.c_str(), flags);
}

// BeginTabItem() without X close button.
static bool ImGui_BeginTabItem(const string& label, ImGuiTabItemFlags flags)
{
    return ImGui::BeginTabItem(label.c_str(), nullptr, flags);
}

// BeginTabItem() with X close button.
static bool ImGui_BeginTabItemClosable(const string& label, bool& p_open, ImGuiTabItemFlags flags)
{
    return ImGui::BeginTabItem(label.c_str(), &p_open, flags);
}

static void ImGui_SetTabItemClosed(const string& tab_or_docked_window_label)
{
    ImGui::SetTabItemClosed(tab_or_docked_window_label.c_str());
}

// ---------------------------------------------------------------------------
// ID scopes
// ---------------------------------------------------------------------------

static void ImGui_PushIDStr(const string& n)
{
    ImGui::PushID(n.c_str());
}

static void ImGui_PushIDInt(int id)
{
    ImGui::PushID(id);
}

static unsigned ImGui_GetID(const string& n)
{
    return ImGui::GetID(n.c_str());
}

// ---------------------------------------------------------------------------
// Widgets: Text
// ---------------------------------------------------------------------------

static void ImGui_Text(const string& n)
{
    ImGui::Text(n.c_str());
}

static void ImGui_TextDisabled(const string& n)
{
    ImGui::TextDisabled(n.c_str());
}

static void ImGui_TextColored(ColourValue c, const string& n)
{
    ImGui::TextColored(ImVec4(c.r, c.g, c.b, c.a), n.c_str());
}

static void ImGui_TextWrapped(const string& n)
{
    ImGui::TextWrapped(n.c_str());
}

static void ImGui_LabelText(const string& l, const string& n)
{
    ImGui::LabelText(l.c_str(), n.c_str());
}

static void ImGui_BulletText(const string& n)
{
    ImGui::BulletText(n.c_str());
}

// ---------------------------------------------------------------------------
// Widgets: Main
// ---------------------------------------------------------------------------

static bool ImGui_Button(const string& n, Vector2 v)
{
    return ImGui::Button(n.c_str(), ImVec2(v.x, v.y));
}

static bool ImGui_SmallButton(const string& n)
{
    return ImGui::SmallButton(n.c_str());
}

static bool ImGui_InvisibleButton(const string& id, Vector2 v)
{
    return ImGui::InvisibleButton(id.c_str(), ImVec2(v.x, v.y));
}

static void ImGui_Image(Ogre::TexturePtr const& tex, Vector2 v)
{
    ImGui::Image((ImTextureID)tex->getHandle(), ImVec2(v.x, v.y));
}

static bool ImGui_Checkbox(const string& n, bool& v)
{
    return ImGui::Checkbox(n.c_str(), &v);
}

static bool ImGui_CheckboxFlags(const string& n, unsigned& f, unsigned v)
{
    return ImGui::CheckboxFlags(n.c_str(), &f, v);
}

static bool ImGui_RadioButton(const string& n, bool v)
{
    return ImGui::RadioButton(n.c_str(), v);
}

static bool ImGui_RadioButtonInt(const string& n, int& v, int vv)
{
    return ImGui::RadioButton(n.c_str(), &v, vv);
}

static void ImGui_ProgressBar(float v, Ogre::Vector2 size, const string& overlay)
{
    ImGui::ProgressBar(v, ImVec2(size.x, size.y), (overlay != "")?overlay.c_str():nullptr);
}

// ---------------------------------------------------------------------------
// Widgets: Drags
// ---------------------------------------------------------------------------

static bool ImGui_DragFloat(const string& n, float& v, float speed, float mn, float mx)
{
    return ImGui::DragFloat(n.c_str(), &v, speed, mn, mx);
}

static bool ImGui_DragFloat2(const string& n, Vector2& v)
{
    return ImGui::DragFloat2(n.c_str(), &v.x);
}

static bool ImGui_DragFloat3(const string& n, Vector3& v)
{
    return ImGui::DragFloat3(n.c_str(), &v.x);
}

static bool ImGui_DragFloatRange2(const string& n, float& v0, float& v1, float mn, float mx)
{
    return ImGui::DragFloatRange2(n.c_str(), &v0, &v1, 1.0f, mn, mx);
}

static bool ImGui_DragIntRange2(const string& n, int& v0, int& v1, int mn, int mx)
{
    return ImGui::DragIntRange2(n.c_str(), &v0, &v1, 1.0f, mn, mx);
}

// ---------------------------------------------------------------------------
// Widgets: Input with Keyboard
// ---------------------------------------------------------------------------

static char imgui_text_buffer[4096]; // shared with multiple widgets

static bool ImGui_InputText(const string& id, string& val)
{
    memset(imgui_text_buffer, 0, sizeof(char) * 4096);
    strcpy(imgui_text_buffer, val.c_str());
    if (ImGui::InputText(id.c_str(), imgui_text_buffer, 4096))
    {
        val = imgui_text_buffer;
        return true;
    }
    return false;
}

static bool ImGui_InputTextMultiline(const string& id, string& val, const Ogre::Vector2& size)
{
    return ImGui::InputTextMultiline(id.c_str(), (char*)val.data(), val.size(), ImVec2(size.x, size.y));
}

static bool ImGui_InputFloat(const string& id, float& val)
{
    return ImGui::InputFloat(id.c_str(), &val);
}

static bool ImGui_InputFloat2(const string& id, Vector2& val)
{
    return ImGui::InputFloat2(id.c_str(), &val.x);
}

static bool ImGui_InputFloat3(const string& id, Vector3& val)
{
    return ImGui::InputFloat3(id.c_str(), &val.x);
}

static bool ImGui_InputInt(const string& id, int& val)
{
    return ImGui::InputInt(id.c_str(), &val);
}

// ---------------------------------------------------------------------------
// Widgets: Sliders
// ---------------------------------------------------------------------------

static bool ImGui_SliderFloat(const string& n, float& v, float mn, float mx)
{
    return ImGui::SliderFloat(n.c_str(), &v, mn, mx);
}

static bool ImGui_SliderFloat2(const string& n, Vector2& v, float mn, float mx)
{
    return ImGui::SliderFloat2(n.c_str(), &v.x, mn, mx);
}

static bool ImGui_SliderFloat3(const string& n, Vector3& v, float mn, float mx)
{
    return ImGui::SliderFloat3(n.c_str(), &v.x, mn, mx);
}

static bool ImGui_SliderInt(const string& n, int& v, int mn, int mx)
{
    return ImGui::SliderInt(n.c_str(), &v, mn, mx);
}

// ---------------------------------------------------------------------------
// Widgets: Color Editor/Picker
// ---------------------------------------------------------------------------

static bool ImGui_ColorEdit3(const string& id, ColourValue& val)
{
    Vector3 v(val.r, val.g, val.b);
    if (ImGui::ColorEdit3(id.c_str(), &v.x))
    {
        val = ColourValue(v.x, v.y, v.z);
        return true;
    }
    return false;
}

static bool ImGui_ColorEdit4(const string& id, ColourValue& val)
{
    Vector4 v(val.r, val.g, val.b, val.a);
    if (ImGui::ColorEdit4(id.c_str(), &v.x))
    {
        val = ColourValue(v.x, v.y, v.z, v.w);
        return true;
    }
    return false;
}

static bool ImGui_ColorButton(const string& id, ColourValue val)
{
    Vector4 v(val.r, val.g, val.b, val.a);
    ImVec4 vv(v.x, v.y, v.z, v.w);
    return ImGui::ColorButton(id.c_str(), vv);
}

// ---------------------------------------------------------------------------
// Widgets: Trees
// ---------------------------------------------------------------------------

static bool ImGui_TreeNode(const string& id)
{
    return ImGui::TreeNode(id.c_str());
}

static void ImGui_TreePush(const string& id)
{
    ImGui::TreePush(id.c_str());
}

static void ImGui_SetNextTreeNodeOpen(bool val)
{
    ImGui::SetNextTreeNodeOpen(val);
}

static bool ImGui_CollapsingHeader(const string& n)
{
    return ImGui::CollapsingHeader(n.c_str());
}

static bool ImGui_CollapsingHeaderClosable(const string& n, bool& v)
{
    return ImGui::CollapsingHeader(n.c_str(), &v);
}

// ---------------------------------------------------------------------------
// Widgets: Selectable / Lists
// ---------------------------------------------------------------------------

static bool ImGui_Selectable(const string& n, bool v)
{
    return ImGui::Selectable(n.c_str(), v);
}

static bool ImGui_ListBoxHeader(const string& n)
{
    return ImGui::ListBoxHeader(n.c_str());
}

// ---------------------------------------------------------------------------
// Values
// ---------------------------------------------------------------------------

static void ImGui_ValueBool(const string& n, bool v)
{
    ImGui::Value(n.c_str(), v);
}

static void ImGui_ValueInt(const string& n, int v)
{
    ImGui::Value(n.c_str(), v);
}

static void ImGui_ValueUint(const string& n, unsigned v)
{
    ImGui::Value(n.c_str(), v);
}

static void ImGui_ValueFloat(const string& n, float v)
{
    ImGui::Value(n.c_str(), v);
}

// ---------------------------------------------------------------------------
// Tooltips
// ---------------------------------------------------------------------------

static void ImGui_SetTooltip(const string& t)
{
    ImGui::SetTooltip(t.c_str());
}

// ---------------------------------------------------------------------------
// Menus
// ---------------------------------------------------------------------------

static bool ImGui_BeginMainMenuBar()
{
    return ImGui::BeginMainMenuBar();
}

static void ImGui_EndMainMenuBar()
{
    ImGui::EndMainMenuBar();
}

static bool ImGui_BeginMenuBar()
{
    return ImGui::BeginMenuBar();
}

static void ImGui_EndMenuBar()
{
    ImGui::EndMenuBar();
}

static bool ImGui_BeginMenu(const string& a, bool b)
{
    return ImGui::BeginMenu(a.c_str(), b);
}

static void ImGui_EndMenu()
{
    ImGui::EndMenu();
}

static bool ImGui_MenuItem(const string& a, const string& b, bool c, bool d)
{
    return ImGui::MenuItem(a.c_str(), b.c_str(), c, d);
}

static bool ImGui_MenuItemToggle(const string& a, const string& b, bool& c, bool d)
{
    return ImGui::MenuItem(a.c_str(), b.c_str(), &c, d);
}

// ---------------------------------------------------------------------------
// Popups
// ---------------------------------------------------------------------------

static void ImGui_OpenPopup(const string& a)
{
    ImGui::OpenPopup(a.c_str());
}

static bool ImGui_BeginPopup(const string& a, int b)
{
    return ImGui::BeginPopup(a.c_str()); // FIXME: update imgui! -- flags omitted
}

static bool ImGui_BeginPopupContextItem(const string& a, int b)
{
    return ImGui::BeginPopupContextItem(a.c_str(), b);
}

static bool ImGui_BeginPopupContextWindow(const string& a, int b, bool c)
{
    return ImGui::BeginPopupContextWindow(a.c_str(), b, c); // FIXME: update imgui! -- swapped args
}

static bool ImGui_BeginPopupContextVoid(const string& a, int b)
{
    return ImGui::BeginPopupContextVoid(a.c_str(), b);
}

static bool ImGui_BeginPopupModal(const string& a, bool& b, int c)
{
    return ImGui::BeginPopupModal(a.c_str(), &b, (ImGuiWindowFlags)c);
}

static void ImGui_EndPopup()
{
    ImGui::EndPopup();
}

static void ImGui_CloseCurrentPopup()
{
    ImGui::CloseCurrentPopup();
}

// ---------------------------------------------------------------------------
// Clip-rects
// ---------------------------------------------------------------------------

static void ImGui_PushClipRect(const Vector2& a, const Vector2& b, bool c)
{
    ImGui::PushClipRect(ImVec2(a.x, a.y), ImVec2(b.x, b.y), c);
}

static void ImGui_PopClipRect()
{
    ImGui::PopClipRect();
}

// ---------------------------------------------------------------------------
// Focus
// ---------------------------------------------------------------------------

static void ImGui_SetKeyboardFocusHere(int a)
{
    ImGui::SetKeyboardFocusHere(a);
}

// ---------------------------------------------------------------------------
// Utilities
// ---------------------------------------------------------------------------

static bool ImGui_IsItemHovered(int a)
{
    return ImGui::IsItemHovered(); // TODO: update imgui -- flags omitted
}

static bool ImGui_IsItemActive()
{
    return ImGui::IsItemActive();
}

static bool ImGui_IsItemClicked(int a)
{
    return ImGui::IsItemClicked(a);
}

static bool ImGui_IsItemVisible()
{
    return ImGui::IsItemVisible();
}

static bool ImGui_IsAnyItemHovered()
{
    return ImGui::IsAnyItemHovered();
}

static bool ImGui_IsAnyItemActive()
{
    return ImGui::IsAnyItemActive();
}

static Vector2 ImGui_GetItemRectMin()
{
    auto v = ImGui::GetItemRectMin(); return Vector2(v.x, v.y);
}

static Vector2 ImGui_GetItemRectMax()
{
    auto v = ImGui::GetItemRectMax(); return Vector2(v.x, v.y);
}

static Vector2 ImGui_GetItemRectSize()
{
    auto v = ImGui::GetItemRectSize(); return Vector2(v.x, v.y);
}

static void ImGui_SetItemAllowOverlap()
{
    ImGui::SetItemAllowOverlap();
}

static bool ImGui_IsWindowFocused(int a)
{
    return ImGui::IsWindowFocused(); // TODO: update imgui -- flags omitted
}

static bool ImGui_IsWindowHovered(int a)
{
    return ImGui::IsWindowHovered(); // TODO: update imgui -- flags omitted
}

static bool ImGui_IsRectVisibleSize(const Vector2& a)
{
    return ImGui::IsRectVisible(ImVec2(a.x, a.y));
}

static bool ImGui_IsRectVisible(const Vector2& a, const Vector2& b)
{
    return ImGui::IsRectVisible(ImVec2(a.x, a.y), ImVec2(b.x, b.y));
}

static float ImGui_GetTime()
{
    return (float)ImGui::GetTime();
}

static int ImGui_GetFrameCount()
{
    return ImGui::GetFrameCount();
}

static Vector2 ImGui_CalcTextSize(const string& a, bool c, float d)
{
    auto v = ImGui::CalcTextSize(a.c_str(), nullptr, c, d); return Vector2(v.x, v.y);
}

static void ImGui_CalcListClipping(int a, float b, int& c, int& d)
{
    ImGui::CalcListClipping(a, b, &c, &d);
}

static bool ImGui_BeginChildFrame(unsigned a, const Vector2& b, int c)
{
    return ImGui::BeginChildFrame(a, ImVec2(b.x,b.y), (ImGuiWindowFlags)c);
}

static void ImGui_EndChildFrame()
{
    ImGui::EndChildFrame();
}

// ---------------------------------------------------------------------------
// Inputs
// ---------------------------------------------------------------------------

static int ImGui_GetKeyIndex(int a)
{
    return ImGui::GetKeyIndex((ImGuiKey)a);
}

static bool ImGui_IsKeyDown(int a)
{
    return ImGui::IsKeyDown(a);
}

static bool ImGui_IsKeyPressed(int a, bool b)
{
    return ImGui::IsKeyPressed(a, b);
}

static bool ImGui_IsKeyReleased(int a)
{
    return ImGui::IsKeyReleased(a);
}

static bool ImGui_IsMouseDown(int a)
{
    return ImGui::IsMouseDown(a);
}

static bool ImGui_IsMouseClicked(int a, bool b)
{
    return ImGui::IsMouseClicked(a, b);
}

static bool ImGui_IsMouseDoubleClicked(int a)
{
    return ImGui::IsMouseDoubleClicked(a);
}

static bool ImGui_IsMouseReleased(int a)
{
    return ImGui::IsMouseReleased(a);
}

static bool ImGui_IsMouseDragging(int a, float b)
{
    return ImGui::IsMouseDragging(a, b);
}

static bool ImGui_IsMouseHoveringRect(const Vector2& a, const Vector2& b, bool c)
{
    return ImGui::IsMouseHoveringRect(ImVec2(a.x, a.y), ImVec2(b.x, b.y), c);
}

static Vector2 ImGui_GetMousePos()
{
    auto v = ImGui::GetMousePos(); return Vector2(v.x, v.y);
}

static Vector2 ImGui_GetMousePosOnOpeningCurrentPopup()
{
    auto v = ImGui::GetMousePosOnOpeningCurrentPopup(); return Vector2(v.x, v.y);
}

static Vector2 ImGui_GetMouseDragDelta(int a, float b)
{
    auto v = ImGui::GetMouseDragDelta(a, b); return Vector2(v.x, v.y);
}

static void ImGui_ResetMouseDragDelta(int a)
{
    ImGui::ResetMouseDragDelta(a);
}

static int ImGui_GetMouseCursor()
{
    return ImGui::GetMouseCursor();
}

static void ImGui_SetMouseCursor(ImGuiMouseCursor a)
{
    ImGui::SetMouseCursor(a);
}

static void ImGui_CaptureKeyboardFromApp(bool a)
{
    ImGui::CaptureKeyboardFromApp(a);
}

static void ImGui_CaptureMouseFromApp(bool a)
{
    ImGui::CaptureMouseFromApp(a);
}

// ---------------------------------------------------------------------------
// Clipboard
// ---------------------------------------------------------------------------

static string ImGui_GetClipboardText()
{
    return string(ImGui::GetClipboardText());
}

static void ImGui_SetClipboardText(const string& a)
{
    ImGui::SetClipboardText(a.c_str());
}

// ---------------------------------------------------------------------------
// Data plotting - we wrap the 'getter func' variant to resemble the 'float*' variant.
// PlotLines(const char* label, const float* values, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0, 0), int stride = sizeof(float));
// ---------------------------------------------------------------------------

static void ImGui_PlotLines(const string& label, CScriptArray* values, int values_count, int values_offset, const string& overlay_text, float scale_min, float scale_max, Vector2 graph_size)
{
    ImGui::PlotLines(label.c_str(), &ImGuiPlotLinesScriptValueGetterFunc, values, values_count, values_offset, overlay_text.c_str(), scale_min, scale_max, ImVec2(graph_size.x, graph_size.y));
}

} // namespace ImGuiAngelscriptWrappers

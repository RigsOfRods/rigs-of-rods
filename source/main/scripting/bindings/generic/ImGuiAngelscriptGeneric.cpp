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

#include "../../autowrapper/aswrappedcall.h"
#include "AngelScriptBindings.h"
#include "OgreImGui.h"
#include "scriptarray/scriptarray.h"
#include "wrappers/ImGuiAngelscriptWrappers.h"

#include <angelscript.h>
#include <Ogre.h>
#include <string>

using namespace AngelScript;
using namespace Ogre;
using namespace std;
using namespace ImGuiAngelscriptWrappers;

void RoR::RegisterImGuiGenericBindings(AngelScript::asIScriptEngine* engine)
{
    // NOTE: enums and the ImDrawList object type are registered in RegisterImGuiCommonBindings()

    // ImDrawList object (global namespace)
    engine->RegisterObjectMethod("ImDrawList", "void AddLine(const vector2&in p1, const vector2&in p2, const color&in col, float thickness = 1.f)", WRAP_OBJ_FIRST(ImDrawList_AddLine), asCALL_GENERIC);
    engine->RegisterObjectMethod("ImDrawList", "void AddTriangle(const vector2&in p1, const vector2&in p2, const vector2&in p3, const color&in col, float thickness = 1.f)", WRAP_OBJ_FIRST(ImDrawList_AddTriangle), asCALL_GENERIC);
    engine->RegisterObjectMethod("ImDrawList", "void AddTriangleFilled(const vector2&in p1, const vector2&in p2, const vector2&in p3, const color&in col)", WRAP_OBJ_FIRST(ImDrawList_AddTriangleFilled), asCALL_GENERIC);
    engine->RegisterObjectMethod("ImDrawList", "void AddRect(const vector2&in p_min, const vector2&in p_max, const color&in col, float rounding = 0.0f, int rounding_corners = 15, float thickness = 1.f)", WRAP_OBJ_FIRST(ImDrawList_AddRect), asCALL_GENERIC);
    engine->RegisterObjectMethod("ImDrawList", "void AddRectFilled(const vector2&in p_min, const vector2&in p_max, const color&in col, float rounding = 0.0f, int rounding_corners = 15)", WRAP_OBJ_FIRST(ImDrawList_AddRectFilled), asCALL_GENERIC);
    engine->RegisterObjectMethod("ImDrawList", "void AddCircle(const vector2&in center, float radius, const color&in col, int num_segments = 12, float thickness = 1.f)", WRAP_OBJ_FIRST(ImDrawList_AddCircle), asCALL_GENERIC);
    engine->RegisterObjectMethod("ImDrawList", "void AddCircleFilled(const vector2&in center, float radius, const color&in col, int num_segments = 12)", WRAP_OBJ_FIRST(ImDrawList_AddCircleFilled), asCALL_GENERIC);
    engine->RegisterObjectMethod("ImDrawList", "void AddText(const vector2&in pos, const color&in col, const string&in text)", WRAP_OBJ_FIRST(ImDrawList_AddText), asCALL_GENERIC);
    engine->RegisterObjectMethod("ImDrawList", "void AddImage(const Ogre::TexturePtr&in tex, const vector2&in p_min, const vector2&in p_max, const vector2&in uv_min, const vector2&in uv_max, const color&in col)", WRAP_OBJ_FIRST(ImDrawList_AddImage), asCALL_GENERIC);
    // FUNCTIONS (namespace ImGui)
    engine->SetDefaultNamespace("ImGui");

    // > Windows
    engine->RegisterGlobalFunction("bool Begin(const string&in, bool&inout, int=0)", WRAP_FN(ImGui_Begin), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void End()", WRAP_FN_PR(ImGui::End, (), void), asCALL_GENERIC);

    // > Child windows
    engine->RegisterGlobalFunction("bool BeginChild(const string&in, const vector2&in=vector2(0,0), bool=false, int=0)", WRAP_FN(ImGui_BeginChild), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool BeginChild(uint, const vector2&in=vector2(0,0), bool=false, int=0)", WRAP_FN(ImGui_BeginChildById), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void EndChild()", WRAP_FN_PR(ImGui::EndChild, (), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("ImDrawList@ GetWindowDrawList()", WRAP_FN_PR(ImGui::GetWindowDrawList, (), ImDrawList*), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void PushStyleVar(int index, float val)", WRAP_FN(ImGui_PushStyleVarFloat), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void PushStyleVar(int index, const vector2&in val)", WRAP_FN(ImGui_PushStyleVarVector2), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void PopStyleVar(int count = 1)", WRAP_FN_PR(ImGui::PopStyleVar, (int), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void PushStyleColor(int index, const color&in color)", WRAP_FN(ImGui_PushStyleColor), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void PopStyleColor(int count = 1)", WRAP_FN_PR(ImGui::PopStyleColor, (int), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetNextItemWidth(float)", WRAP_FN_PR(ImGui::SetNextItemWidth, (float), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetNextItemOpen(bool, ImGuiCond)", WRAP_FN_PR(ImGui::SetNextItemOpen, (bool, int), void), asCALL_GENERIC);

    engine->RegisterGlobalFunction("vector2 GetContentRegionMax()", WRAP_FN(ImGui_GetContentRegionMax), asCALL_GENERIC);
    engine->RegisterGlobalFunction("vector2 GetContentRegionAvail()", WRAP_FN(ImGui_GetContentRegionAvail), asCALL_GENERIC);
    engine->RegisterGlobalFunction("float GetContentRegionAvailWidth()", WRAP_FN_PR(ImGui::GetContentRegionAvailWidth, (), float), asCALL_GENERIC);
    engine->RegisterGlobalFunction("vector2 GetWindowContentRegionMin()", WRAP_FN(ImGui_GetWindowContentRegionMin), asCALL_GENERIC);
    engine->RegisterGlobalFunction("vector2 GetWindowContentRegionMax()", WRAP_FN(ImGui_GetWindowContentRegionMax), asCALL_GENERIC);
    engine->RegisterGlobalFunction("float GetWindowRegionWidth()", WRAP_FN_PR(ImGui::GetWindowContentRegionWidth, (), float), asCALL_GENERIC);

    engine->RegisterGlobalFunction("vector2 GetWindowPos()", WRAP_FN(ImGui_GetWindowPos), asCALL_GENERIC);
    engine->RegisterGlobalFunction("vector2 GetWindowSize()", WRAP_FN(ImGui_GetWindowSize), asCALL_GENERIC);
    engine->RegisterGlobalFunction("float GetWindowWidth()", WRAP_FN_PR(ImGui::GetWindowWidth, (), float), asCALL_GENERIC);
    engine->RegisterGlobalFunction("float GetWindowHeight()", WRAP_FN_PR(ImGui::GetWindowHeight, (), float), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool IsWindowCollapsed()", WRAP_FN_PR(ImGui::IsWindowCollapsed, (), bool), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetWindowFontScale(float)", WRAP_FN_PR(ImGui::SetWindowFontScale, (float), void), asCALL_GENERIC);

    engine->RegisterGlobalFunction("void SetNextWindowPos(vector2, int=0, vector2=vector2(0,0))", WRAP_FN(ImGui_SetNextWindowPos), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetNextWindowSize(vector2)", WRAP_FN(ImGui_SetNextWindowSize), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetNextWindowContentSize(vector2)", WRAP_FN(ImGui_SetNextWindowContentSize), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetNextWindowCollapsed(bool)", WRAP_FN(ImGui_SetNextWindowCollapsed), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetNextWindowFocus()", WRAP_FN(ImGui_SetNextWindowFocus), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetWindowPos(vector2)", WRAP_FN(ImGui_SetWindowPos), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetWindowSize(vector2)", WRAP_FN(ImGui_SetWindowSize), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetWindowCollapsed(bool)", WRAP_FN(ImGui_SetWindowCollapsed), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetWindowFocus()", WRAP_FN(ImGui_SetWindowFocus), asCALL_GENERIC);

    engine->RegisterGlobalFunction("void SetWindowPos(const string&in, vector2)", WRAP_FN(ImGui_SetWindowPosByName), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetWindowSize(const string&in, vector2)", WRAP_FN(ImGui_SetWindowSizeByName), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetWindowCollapsed(const string&in, bool)", WRAP_FN(ImGui_SetWindowCollapsedByName), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetWindowFocus(const string&in)", WRAP_FN(ImGui_SetWindowFocusByName), asCALL_GENERIC);

    engine->RegisterGlobalFunction("float GetScrollX()", WRAP_FN_PR(ImGui::GetScrollX, (), float), asCALL_GENERIC);
    engine->RegisterGlobalFunction("float GetScrollY()", WRAP_FN_PR(ImGui::GetScrollY, (), float), asCALL_GENERIC);
    engine->RegisterGlobalFunction("float GetScrollMaxX()", WRAP_FN_PR(ImGui::GetScrollMaxX, (), float), asCALL_GENERIC);
    engine->RegisterGlobalFunction("float GetScrollMaxY()", WRAP_FN_PR(ImGui::GetScrollMaxY, (), float), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetScrollX(float)", WRAP_FN_PR(ImGui::SetScrollX, (float), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetScrollY(float)", WRAP_FN_PR(ImGui::SetScrollY, (float), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetScrollHere(float = 0.5f)", WRAP_FN_PR(ImGui::SetScrollHere, (float), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetScrollFromPosY(float, float = 0.5f)", WRAP_FN_PR(ImGui::SetScrollFromPosY, (float,float), void), asCALL_GENERIC);

    engine->RegisterGlobalFunction("void Separator()", WRAP_FN_PR(ImGui::Separator, (), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SameLine(float = 0.0f, float = -1.0f)", WRAP_FN_PR(ImGui::SameLine, (float,float), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void NewLine()", WRAP_FN_PR(ImGui::NewLine, (), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void Spacing()", WRAP_FN_PR(ImGui::Spacing, (), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void Dummy(vector2)", WRAP_FN(ImGui_Dummy), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void Indent(float = 0.0f)", WRAP_FN_PR(ImGui::Indent, (float), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void Unindent(float = 0.0f)", WRAP_FN_PR(ImGui::Unindent, (float), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void BeginGroup()", WRAP_FN_PR(ImGui::BeginGroup, (), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void EndGroup()", WRAP_FN_PR(ImGui::EndGroup, (), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("vector2 GetCursorPos()", WRAP_FN(ImGui_GetCursorPos), asCALL_GENERIC);
    engine->RegisterGlobalFunction("float GetCursorPosX()", WRAP_FN_PR(ImGui::GetCursorPosX, (), float), asCALL_GENERIC);
    engine->RegisterGlobalFunction("float GetCursorPosY()", WRAP_FN_PR(ImGui::GetCursorPosY, (), float), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetCursorPos(vector2)", WRAP_FN(ImGui_SetCursorPos), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetCursorPosX(float)", WRAP_FN_PR(ImGui::SetCursorPosX, (float), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetCursorPosY(float)", WRAP_FN_PR(ImGui::SetCursorPosY, (float), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("vector2 GetCursorStartPos()", WRAP_FN(ImGui_GetCursorStartPos), asCALL_GENERIC);
    engine->RegisterGlobalFunction("vector2 GetCursorScreenPos()", WRAP_FN(ImGui_GetCursorScreenPos), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetCursorScreenPos(vector2)", WRAP_FN(ImGui_SetCursorScreenPos), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void AlignTextToFramePadding()", WRAP_FN_PR(ImGui::AlignTextToFramePadding, (), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("float GetTextLineHeight()", WRAP_FN_PR(ImGui::GetTextLineHeight, (), float), asCALL_GENERIC);
    engine->RegisterGlobalFunction("float GetTextLineHeightWithSpacing()", WRAP_FN_PR(ImGui::GetTextLineHeightWithSpacing, (), float), asCALL_GENERIC);

    // Columns (considered legacy in latest versions - superseded by Tables!)
    engine->RegisterGlobalFunction("void Columns(int = 1, const string&in = string(), bool = true)", WRAP_FN(ImGui_Columns), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void NextColumn()", WRAP_FN(ImGui_NextColumn), asCALL_GENERIC);
    engine->RegisterGlobalFunction("int GetColumnIndex()", WRAP_FN(ImGui_GetColumnIndex), asCALL_GENERIC);
    engine->RegisterGlobalFunction("float GetColumnWidth(int = -1)", WRAP_FN(ImGui_GetColumnWidth), asCALL_GENERIC);
    engine->RegisterGlobalFunction("float GetColumnOffset(int = -1)", WRAP_FN(ImGui_GetColumnOffset), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetColumnOffset(int, float)", WRAP_FN(ImGui_SetColumnOffset), asCALL_GENERIC);
    engine->RegisterGlobalFunction("int GetColumnsCount()", WRAP_FN(ImGui_GetColumnsCount), asCALL_GENERIC);

    // Tab bars, tabs
    engine->RegisterGlobalFunction("bool BeginTabBar(const string&in, int = 0)", WRAP_FN(ImGui_BeginTabBar), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void EndTabBar()", WRAP_FN(ImGui::EndTabBar), asCALL_GENERIC);
    // BeginTabItem() without X close button.
    engine->RegisterGlobalFunction("bool BeginTabItem(const string&in, int = 0)", WRAP_FN(ImGui_BeginTabItem), asCALL_GENERIC);
    // BeginTabItem() with X close button.
    engine->RegisterGlobalFunction("bool BeginTabItem(const string&in, bool&inout, int = 0)", WRAP_FN(ImGui_BeginTabItemClosable), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void EndTabItem()", WRAP_FN(ImGui::EndTabItem), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetTabItemClosed(const string&in)", WRAP_FN(ImGui_SetTabItemClosed), asCALL_GENERIC);

    // ID scopes
    engine->RegisterGlobalFunction("void PushID(const string&in)", WRAP_FN(ImGui_PushIDStr), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void PushID(int int_id)", WRAP_FN(ImGui_PushIDInt), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void PopID()", WRAP_FN_PR(ImGui::PopID, (), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("uint GetID(const string&in)", WRAP_FN(ImGui_GetID), asCALL_GENERIC);

    // Widgets: Text
    engine->RegisterGlobalFunction("void Text(const string&in)", WRAP_FN(ImGui_Text), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void TextDisabled(const string&in)", WRAP_FN(ImGui_TextDisabled), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void TextColored(color col, const string&in)", WRAP_FN(ImGui_TextColored), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void TextWrapped(const string&in)", WRAP_FN(ImGui_TextWrapped), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void LabelText(const string&in, const string&in)", WRAP_FN(ImGui_LabelText), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void Bullet()", WRAP_FN_PR(ImGui::Bullet, (), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void BulletText(const string&in)", WRAP_FN(ImGui_BulletText), asCALL_GENERIC);

    // Widgets: Main
    engine->RegisterGlobalFunction("bool Button(const string&in, vector2 = vector2(0,0))", WRAP_FN(ImGui_Button), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool SmallButton(const string&in)", WRAP_FN(ImGui_SmallButton), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool InvisibleButton(const string&in, vector2)", WRAP_FN(ImGui_InvisibleButton), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void Image(const Ogre::TexturePtr&in, vector2)", WRAP_FN(ImGui_Image), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool Checkbox(const string&in, bool&inout)", WRAP_FN(ImGui_Checkbox), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool CheckboxFlags(const string&in, uint&inout, uint)", WRAP_FN(ImGui_CheckboxFlags), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool RadioButton(const string&in, bool)", WRAP_FN(ImGui_RadioButton), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool RadioButton(const string&in, int&inout, int)", WRAP_FN(ImGui_RadioButtonInt), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void ProgressBar(float, vector2=vector2(-1,0), const string&in = \"\")", WRAP_FN(ImGui_ProgressBar), asCALL_GENERIC);

    // Widgets: Combo Box
    // Widgets: Drags
    engine->RegisterGlobalFunction("bool DragFloat(const string&in, float&inout, float = 1.0f, float = 0.0f, float = 0.0f)", WRAP_FN(ImGui_DragFloat), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool DragFloat2(const string&in, vector2&inout)", WRAP_FN(ImGui_DragFloat2), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool DragFloat3(const string&in, vector3&inout)", WRAP_FN(ImGui_DragFloat3), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool DragFloatRange2(const string&in, float&inout, float&inout, float = 0.0f, float = 1.0f)", WRAP_FN(ImGui_DragFloatRange2), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool DragIntRange2(const string&in, int&inout, int&inout, int, int)", WRAP_FN(ImGui_DragIntRange2), asCALL_GENERIC);

    // Widgets: Input with Keyboard
    engine->RegisterGlobalFunction("bool InputText(const string&in, string&inout)", WRAP_FN(ImGui_InputText), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool InputTextMultiline(const string&in, string&inout, const vector2&in = vector2(0,0))", WRAP_FN(ImGui_InputTextMultiline), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool InputFloat(const string&, float&inout)", WRAP_FN(ImGui_InputFloat), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool InputFloat2(const string&, vector2&inout)", WRAP_FN(ImGui_InputFloat2), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool InputFloat3(const string&, vector3&inout)", WRAP_FN(ImGui_InputFloat3), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool InputInt(const string&, int&inout)", WRAP_FN(ImGui_InputInt), asCALL_GENERIC);

    // Widgets: Sliders (tip: ctrl+click on a slider to input with keyboard. manually input values aren't clamped, can go off-bounds)
    engine->RegisterGlobalFunction("bool SliderFloat(const string&in, float&inout, float = 0.0f, float = 0.0f)", WRAP_FN(ImGui_SliderFloat), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool SliderFloat2(const string&in, vector2&inout, float, float)", WRAP_FN(ImGui_SliderFloat2), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool SliderFloat3(const string&in, vector3&inout, float, float)", WRAP_FN(ImGui_SliderFloat3), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool SliderInt(const string&in, int&inout, int = 0, int = 0)", WRAP_FN(ImGui_SliderInt), asCALL_GENERIC);

    // Widgets: Color Editor/Picker
    engine->RegisterGlobalFunction("bool ColorEdit3(const string&in, color&inout)", WRAP_FN(ImGui_ColorEdit3), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool ColorEdit4(const string&in, color&inout)", WRAP_FN(ImGui_ColorEdit4), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool ColorButton(const string&in, color)", WRAP_FN(ImGui_ColorButton), asCALL_GENERIC);

    // Widgets: Trees
    engine->RegisterGlobalFunction("bool TreeNode(const string&in)", WRAP_FN(ImGui_TreeNode), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void TreePush(const string&in)", WRAP_FN(ImGui_TreePush), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void TreePop()", WRAP_FN_PR(ImGui::TreePop, (), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void TreeAdvanceToLabelPos()", WRAP_FN_PR(ImGui::TreeAdvanceToLabelPos, (), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("float GetTreeNodeToLabelSpacing()", WRAP_FN_PR(ImGui::GetTreeNodeToLabelSpacing, (), float), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetNextTreeNodeOpen(bool)", WRAP_FN(ImGui_SetNextTreeNodeOpen), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool CollapsingHeader(const string&in)", WRAP_FN(ImGui_CollapsingHeader), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool CollapsingHeader(const string&in, bool&inout)", WRAP_FN(ImGui_CollapsingHeaderClosable), asCALL_GENERIC);

    // Widgets: Selectable / Lists
    engine->RegisterGlobalFunction("bool Selectable(const string&in, bool = false)", WRAP_FN(ImGui_Selectable), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool ListBoxHeader(const string&in)", WRAP_FN(ImGui_ListBoxHeader), asCALL_GENERIC);

    // Values
    engine->RegisterGlobalFunction("void Value(const string&in, bool)", WRAP_FN(ImGui_ValueBool), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void Value(const string&in, int)", WRAP_FN(ImGui_ValueInt), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void Value(const string&in, uint)", WRAP_FN(ImGui_ValueUint), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void Value(const string&in, float)", WRAP_FN(ImGui_ValueFloat), asCALL_GENERIC);

    // Tooltips
    engine->RegisterGlobalFunction("void BeginTooltip()", WRAP_FN_PR(ImGui::BeginTooltip, (), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void EndTooltip()", WRAP_FN_PR(ImGui::EndTooltip, (), void), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetTooltip(const string&in)", WRAP_FN(ImGui_SetTooltip), asCALL_GENERIC);

    // Menus
    engine->RegisterGlobalFunction("bool BeginMainMenuBar()", WRAP_FN(ImGui_BeginMainMenuBar), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void EndMainMenuBar()", WRAP_FN(ImGui_EndMainMenuBar), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool BeginMenuBar()", WRAP_FN(ImGui_BeginMenuBar), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void EndMenuBar()", WRAP_FN(ImGui_EndMenuBar), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool BeginMenu(const string&in, bool = true)", WRAP_FN(ImGui_BeginMenu), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void EndMenu()", WRAP_FN(ImGui_EndMenu), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool MenuItem(const string&in, const string&in = string(), bool = false, bool = true)", WRAP_FN(ImGui_MenuItem), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool MenuItem(const string&in, const string&in, bool &inout, bool = true)", WRAP_FN(ImGui_MenuItemToggle), asCALL_GENERIC);

    // Popups
    engine->RegisterGlobalFunction("void OpenPopup(const string&in)", WRAP_FN(ImGui_OpenPopup), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool BeginPopup(const string&in, int = 0)", WRAP_FN(ImGui_BeginPopup), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool BeginPopupContextItem(const string&in = string(), int = 1)", WRAP_FN(ImGui_BeginPopupContextItem), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool BeginPopupContextWindow(const string&in = string(), int = 1, bool = true)", WRAP_FN(ImGui_BeginPopupContextWindow), asCALL_GENERIC); // FIXME: update imgui! -- swapped args
    engine->RegisterGlobalFunction("bool BeginPopupContextVoid(const string&in = string(), int = 1)", WRAP_FN(ImGui_BeginPopupContextVoid), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool BeginPopupModal(const string&in, bool &inout = null, int = 0)", WRAP_FN(ImGui_BeginPopupModal), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void EndPopup()", WRAP_FN(ImGui_EndPopup), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void CloseCurrentPopup()", WRAP_FN(ImGui_CloseCurrentPopup), asCALL_GENERIC);

    // Clip-rects
    engine->RegisterGlobalFunction("void PushClipRect(const vector2&, const vector2&, bool)", WRAP_FN(ImGui_PushClipRect), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void PopClipRect()", WRAP_FN(ImGui_PopClipRect), asCALL_GENERIC);

    // Focus
    engine->RegisterGlobalFunction("void SetKeyboardFocusHere(int = 0)", WRAP_FN(ImGui_SetKeyboardFocusHere), asCALL_GENERIC);

    // Utilities
    engine->RegisterGlobalFunction("bool IsItemHovered(int = 0)", WRAP_FN(ImGui_IsItemHovered), asCALL_GENERIC); // TODO: update imgui -- flags omitted
    engine->RegisterGlobalFunction("bool IsItemActive()", WRAP_FN(ImGui_IsItemActive), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool IsItemClicked(int = 0)", WRAP_FN(ImGui_IsItemClicked), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool IsItemVisible()", WRAP_FN(ImGui_IsItemVisible), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool IsAnyItemHovered()", WRAP_FN(ImGui_IsAnyItemHovered), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool IsAnyItemActive()", WRAP_FN(ImGui_IsAnyItemActive), asCALL_GENERIC);
    engine->RegisterGlobalFunction("vector2 GetItemRectMin()", WRAP_FN(ImGui_GetItemRectMin), asCALL_GENERIC);
    engine->RegisterGlobalFunction("vector2 GetItemRectMax()", WRAP_FN(ImGui_GetItemRectMax), asCALL_GENERIC);
    engine->RegisterGlobalFunction("vector2 GetItemRectSize()", WRAP_FN(ImGui_GetItemRectSize), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetItemAllowOverlap()", WRAP_FN(ImGui_SetItemAllowOverlap), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool IsWindowFocused(int = 0)", WRAP_FN(ImGui_IsWindowFocused), asCALL_GENERIC); // TODO: update imgui -- flags omitted
    engine->RegisterGlobalFunction("bool IsWindowHovered(int = 0)", WRAP_FN(ImGui_IsWindowHovered), asCALL_GENERIC); // TODO: update imgui -- flags omitted
    engine->RegisterGlobalFunction("bool IsRectVisible(const vector2&)", WRAP_FN(ImGui_IsRectVisibleSize), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool IsRectVisible(const vector2&, const vector2&)", WRAP_FN(ImGui_IsRectVisible), asCALL_GENERIC);
    engine->RegisterGlobalFunction("float GetTime()", WRAP_FN(ImGui_GetTime), asCALL_GENERIC);
    engine->RegisterGlobalFunction("int GetFrameCount()", WRAP_FN(ImGui_GetFrameCount), asCALL_GENERIC);

    engine->RegisterGlobalFunction("vector2 CalcTextSize(const string&in, bool hide_text_after_double_hash = false, float wrap_width = -1.0f)", WRAP_FN(ImGui_CalcTextSize), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void CalcListClipping(int, float, int&inout, int&inout)", WRAP_FN(ImGui_CalcListClipping), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool BeginChildFrame(uint, const vector2&, int = 0)", WRAP_FN(ImGui_BeginChildFrame), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void EndChildFrame()", WRAP_FN(ImGui_EndChildFrame), asCALL_GENERIC);

    engine->RegisterGlobalFunction("int GetKeyIndex(int)", WRAP_FN(ImGui_GetKeyIndex), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool IsKeyDown(int)", WRAP_FN(ImGui_IsKeyDown), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool IsKeyPressed(int, bool = true)", WRAP_FN(ImGui_IsKeyPressed), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool IsKeyReleased(int)", WRAP_FN(ImGui_IsKeyReleased), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool IsMouseDown(int)", WRAP_FN(ImGui_IsMouseDown), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool IsMouseClicked(int, bool = false)", WRAP_FN(ImGui_IsMouseClicked), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool IsMouseDoubleClicked(int)", WRAP_FN(ImGui_IsMouseDoubleClicked), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool IsMouseReleased(int)", WRAP_FN(ImGui_IsMouseReleased), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool IsMouseDragging(int = 0, float = -1.0f)", WRAP_FN(ImGui_IsMouseDragging), asCALL_GENERIC);
    engine->RegisterGlobalFunction("bool IsMouseHoveringRect(const vector2&in, const vector2&in, bool = true)", WRAP_FN(ImGui_IsMouseHoveringRect), asCALL_GENERIC);
    engine->RegisterGlobalFunction("vector2 GetMousePos()", WRAP_FN(ImGui_GetMousePos), asCALL_GENERIC);
    engine->RegisterGlobalFunction("vector2 GetMousePosOnOpeningCurrentPopup()", WRAP_FN(ImGui_GetMousePosOnOpeningCurrentPopup), asCALL_GENERIC);
    engine->RegisterGlobalFunction("vector2 GetMouseDragDelta(int = 0, float = -1.0f)", WRAP_FN(ImGui_GetMouseDragDelta), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void ResetMouseDragDelta(int = 0)", WRAP_FN(ImGui_ResetMouseDragDelta), asCALL_GENERIC);
    engine->RegisterGlobalFunction("int GetMouseCursor()", WRAP_FN(ImGui_GetMouseCursor), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetMouseCursor(int)", WRAP_FN(ImGui_SetMouseCursor), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void CaptureKeyboardFromApp(bool = true)", WRAP_FN(ImGui_CaptureKeyboardFromApp), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void CaptureMouseFromApp(bool = true)", WRAP_FN(ImGui_CaptureMouseFromApp), asCALL_GENERIC);

    engine->RegisterGlobalFunction("string GetClipboardText()", WRAP_FN(ImGui_GetClipboardText), asCALL_GENERIC);
    engine->RegisterGlobalFunction("void SetClipboardText(const string&in)", WRAP_FN(ImGui_SetClipboardText), asCALL_GENERIC);

    // Data plotting - we wrap the 'getter func' variant to resemble the 'float*' variant.
    engine->RegisterGlobalFunction("void PlotLines(const string&in label, array<float>&in values, int values_count, int values_offset = 0, const string&in overlay_text = string(), float scale_min = FLT_MAX, float scale_max = FLT_MAX, vector2 graph_size = vector2(0,0))", WRAP_FN(ImGui_PlotLines), asCALL_GENERIC);

    engine->SetDefaultNamespace("");
}

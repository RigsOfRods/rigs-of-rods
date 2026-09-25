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

#include "AngelScriptBindings.h"
#include "OgreImGui.h"
#include "scriptarray/scriptarray.h"
#include "ScriptEngine.h"
#include "wrappers/ImGuiAngelscriptWrappers.h"

#include <angelscript.h>
#include <Ogre.h>
#include <string>

using namespace AngelScript;
using namespace Ogre;
using namespace std;
using namespace ImGuiAngelscriptWrappers;

void RoR::RegisterImGuiNativeBindings(AngelScript::asIScriptEngine* engine)
{
    // NOTE: enums and the ImDrawList object type are registered in RegisterImGuiCommonBindings()

    // ImDrawList object (global namespace)
    engine->RegisterObjectMethod("ImDrawList", "void AddLine(const vector2&in p1, const vector2&in p2, const color&in col, float thickness = 1.f)", asFUNCTION(ImDrawList_AddLine), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("ImDrawList", "void AddTriangle(const vector2&in p1, const vector2&in p2, const vector2&in p3, const color&in col, float thickness = 1.f)", asFUNCTION(ImDrawList_AddTriangle), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("ImDrawList", "void AddTriangleFilled(const vector2&in p1, const vector2&in p2, const vector2&in p3, const color&in col)", asFUNCTION(ImDrawList_AddTriangleFilled), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("ImDrawList", "void AddRect(const vector2&in p_min, const vector2&in p_max, const color&in col, float rounding = 0.0f, int rounding_corners = 15, float thickness = 1.f)", asFUNCTION(ImDrawList_AddRect), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("ImDrawList", "void AddRectFilled(const vector2&in p_min, const vector2&in p_max, const color&in col, float rounding = 0.0f, int rounding_corners = 15)", asFUNCTION(ImDrawList_AddRectFilled), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("ImDrawList", "void AddCircle(const vector2&in center, float radius, const color&in col, int num_segments = 12, float thickness = 1.f)", asFUNCTION(ImDrawList_AddCircle), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("ImDrawList", "void AddCircleFilled(const vector2&in center, float radius, const color&in col, int num_segments = 12)", asFUNCTION(ImDrawList_AddCircleFilled), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("ImDrawList", "void AddText(const vector2&in pos, const color&in col, const string&in text)", asFUNCTION(ImDrawList_AddText), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("ImDrawList", "void AddImage(const Ogre::TexturePtr&in tex, const vector2&in p_min, const vector2&in p_max, const vector2&in uv_min, const vector2&in uv_max, const color&in col)", asFUNCTION(ImDrawList_AddImage), asCALL_CDECL_OBJFIRST);
    // FUNCTIONS (namespace ImGui)
    engine->SetDefaultNamespace("ImGui");

    // > Windows
    engine->RegisterGlobalFunction("bool Begin(const string&in, bool&inout, int=0)", asFUNCTION(ImGui_Begin), asCALL_CDECL);
    engine->RegisterGlobalFunction("void End()", asFUNCTIONPR(ImGui::End, (), void), asCALL_CDECL);

    // > Child windows
    engine->RegisterGlobalFunction("bool BeginChild(const string&in, const vector2&in=vector2(0,0), bool=false, int=0)", asFUNCTION(ImGui_BeginChild), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool BeginChild(uint, const vector2&in=vector2(0,0), bool=false, int=0)", asFUNCTION(ImGui_BeginChildById), asCALL_CDECL);
    engine->RegisterGlobalFunction("void EndChild()", asFUNCTIONPR(ImGui::EndChild, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("ImDrawList@ GetWindowDrawList()", asFUNCTIONPR(ImGui::GetWindowDrawList, (), ImDrawList*), asCALL_CDECL);
    engine->RegisterGlobalFunction("void PushStyleVar(int index, float val)", asFUNCTION(ImGui_PushStyleVarFloat), asCALL_CDECL);
    engine->RegisterGlobalFunction("void PushStyleVar(int index, const vector2&in val)", asFUNCTION(ImGui_PushStyleVarVector2), asCALL_CDECL);
    engine->RegisterGlobalFunction("void PopStyleVar(int count = 1)", asFUNCTIONPR(ImGui::PopStyleVar, (int), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void PushStyleColor(int index, const color&in color)", asFUNCTION(ImGui_PushStyleColor), asCALL_CDECL);
    engine->RegisterGlobalFunction("void PopStyleColor(int count = 1)", asFUNCTIONPR(ImGui::PopStyleColor, (int), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetNextItemWidth(float)", asFUNCTIONPR(ImGui::SetNextItemWidth, (float), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetNextItemOpen(bool, ImGuiCond)", asFUNCTIONPR(ImGui::SetNextItemOpen, (bool, int), void), asCALL_CDECL);

    engine->RegisterGlobalFunction("vector2 GetContentRegionMax()", asFUNCTION(ImGui_GetContentRegionMax), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetContentRegionAvail()", asFUNCTION(ImGui_GetContentRegionAvail), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetContentRegionAvailWidth()", asFUNCTIONPR(ImGui::GetContentRegionAvailWidth, (), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetWindowContentRegionMin()", asFUNCTION(ImGui_GetWindowContentRegionMin), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetWindowContentRegionMax()", asFUNCTION(ImGui_GetWindowContentRegionMax), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetWindowRegionWidth()", asFUNCTIONPR(ImGui::GetWindowContentRegionWidth, (), float), asCALL_CDECL);

    engine->RegisterGlobalFunction("vector2 GetWindowPos()", asFUNCTION(ImGui_GetWindowPos), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetWindowSize()", asFUNCTION(ImGui_GetWindowSize), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetWindowWidth()", asFUNCTIONPR(ImGui::GetWindowWidth, (), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetWindowHeight()", asFUNCTIONPR(ImGui::GetWindowHeight, (), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsWindowCollapsed()", asFUNCTIONPR(ImGui::IsWindowCollapsed, (), bool), asCALL_CDECL);
  //  engine->RegisterGlobalFunction("bool IsWindowAppearing()", asFUNCTIONPR(ImGui::IsWindowAppearing, (), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetWindowFontScale(float)", asFUNCTIONPR(ImGui::SetWindowFontScale, (float), void), asCALL_CDECL);

    engine->RegisterGlobalFunction("void SetNextWindowPos(vector2, int=0, vector2=vector2(0,0))", asFUNCTION(ImGui_SetNextWindowPos), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetNextWindowSize(vector2)", asFUNCTION(ImGui_SetNextWindowSize), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetNextWindowContentSize(vector2)", asFUNCTION(ImGui_SetNextWindowContentSize), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetNextWindowCollapsed(bool)", asFUNCTION(ImGui_SetNextWindowCollapsed), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetNextWindowFocus()", asFUNCTION(ImGui_SetNextWindowFocus), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetWindowPos(vector2)", asFUNCTION(ImGui_SetWindowPos), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetWindowSize(vector2)", asFUNCTION(ImGui_SetWindowSize), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetWindowCollapsed(bool)", asFUNCTION(ImGui_SetWindowCollapsed), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetWindowFocus()", asFUNCTION(ImGui_SetWindowFocus), asCALL_CDECL);

    engine->RegisterGlobalFunction("void SetWindowPos(const string&in, vector2)", asFUNCTION(ImGui_SetWindowPosByName), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetWindowSize(const string&in, vector2)", asFUNCTION(ImGui_SetWindowSizeByName), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetWindowCollapsed(const string&in, bool)", asFUNCTION(ImGui_SetWindowCollapsedByName), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetWindowFocus(const string&in)", asFUNCTION(ImGui_SetWindowFocusByName), asCALL_CDECL);

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
    engine->RegisterGlobalFunction("void Dummy(vector2)", asFUNCTION(ImGui_Dummy), asCALL_CDECL);
    engine->RegisterGlobalFunction("void Indent(float = 0.0f)", asFUNCTIONPR(ImGui::Indent, (float), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void Unindent(float = 0.0f)", asFUNCTIONPR(ImGui::Unindent, (float), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void BeginGroup()", asFUNCTIONPR(ImGui::BeginGroup, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void EndGroup()", asFUNCTIONPR(ImGui::EndGroup, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetCursorPos()", asFUNCTION(ImGui_GetCursorPos), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetCursorPosX()", asFUNCTIONPR(ImGui::GetCursorPosX, (), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetCursorPosY()", asFUNCTIONPR(ImGui::GetCursorPosY, (), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetCursorPos(vector2)", asFUNCTION(ImGui_SetCursorPos), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetCursorPosX(float)", asFUNCTIONPR(ImGui::SetCursorPosX, (float), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetCursorPosY(float)", asFUNCTIONPR(ImGui::SetCursorPosY, (float), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetCursorStartPos()", asFUNCTION(ImGui_GetCursorStartPos), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetCursorScreenPos()", asFUNCTION(ImGui_GetCursorScreenPos), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetCursorScreenPos(vector2)", asFUNCTION(ImGui_SetCursorScreenPos), asCALL_CDECL);
    engine->RegisterGlobalFunction("void AlignTextToFramePadding()", asFUNCTIONPR(ImGui::AlignTextToFramePadding, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetTextLineHeight()", asFUNCTIONPR(ImGui::GetTextLineHeight, (), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetTextLineHeightWithSpacing()", asFUNCTIONPR(ImGui::GetTextLineHeightWithSpacing, (), float), asCALL_CDECL);
  //  engine->RegisterGlobalFunction("float GetFrameHeight()", asFUNCTIONPR(ImGui::GetFrameHeight, (), float), asCALL_CDECL);
  //  engine->RegisterGlobalFunction("float GetFrameHeightWithSpacing()", asFUNCTIONPR(ImGui::GetFrameHeightWithSpacing, (), float), asCALL_CDECL);

    // Columns (considered legacy in latest versions - superseded by Tables!)
    engine->RegisterGlobalFunction("void Columns(int = 1, const string&in = string(), bool = true)", asFUNCTION(ImGui_Columns), asCALL_CDECL);
    engine->RegisterGlobalFunction("void NextColumn()", asFUNCTION(ImGui_NextColumn), asCALL_CDECL);
    engine->RegisterGlobalFunction("int GetColumnIndex()", asFUNCTION(ImGui_GetColumnIndex), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetColumnWidth(int = -1)", asFUNCTION(ImGui_GetColumnWidth), asCALL_CDECL);
  //  engine->RegisterGlobalFunction("void SetColumnWidth(int, float)", asFUNCTIONPR([](int a, float b) {  ImGui::SetColumnWidth(a, b);  }, (int,float), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetColumnOffset(int = -1)", asFUNCTION(ImGui_GetColumnOffset), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetColumnOffset(int, float)", asFUNCTION(ImGui_SetColumnOffset), asCALL_CDECL);
    engine->RegisterGlobalFunction("int GetColumnsCount()", asFUNCTION(ImGui_GetColumnsCount), asCALL_CDECL);


    // Tab bars, tabs
    engine->RegisterGlobalFunction("bool BeginTabBar(const string&in, int = 0)", asFUNCTION(ImGui_BeginTabBar), asCALL_CDECL);
    engine->RegisterGlobalFunction("void EndTabBar()", asFUNCTION(ImGui::EndTabBar), asCALL_CDECL);
    // BeginTabItem() without X close button.
    engine->RegisterGlobalFunction("bool BeginTabItem(const string&in, int = 0)", asFUNCTION(ImGui_BeginTabItem), asCALL_CDECL);
    // BeginTabItem() with X close button.
    engine->RegisterGlobalFunction("bool BeginTabItem(const string&in, bool&inout, int = 0)", asFUNCTION(ImGui_BeginTabItemClosable), asCALL_CDECL);
    engine->RegisterGlobalFunction("void EndTabItem()", asFUNCTION(ImGui::EndTabItem), asCALL_CDECL);
    //engine->RegisterGlobalFunction("bool TabItemButton(const string&in, ImGuiTabItemFlags = 0)", asFUNCTIONPR([](const string& label, ImGuiTabItemFlags flags) { return ImGui::TabItemButton(label.c_str(), flags); }, (const string&, ImGuiTabItemFlags), bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetTabItemClosed(const string&in)", asFUNCTION(ImGui_SetTabItemClosed), asCALL_CDECL);


    // ID scopes
    engine->RegisterGlobalFunction("void PushID(const string&in)", asFUNCTION(ImGui_PushIDStr), asCALL_CDECL);
    engine->RegisterGlobalFunction("void PushID(int int_id)", asFUNCTION(ImGui_PushIDInt), asCALL_CDECL);
    engine->RegisterGlobalFunction("void PopID()", asFUNCTIONPR(ImGui::PopID, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("uint GetID(const string&in)", asFUNCTION(ImGui_GetID), asCALL_CDECL);

    // Widgets: Text
    engine->RegisterGlobalFunction("void Text(const string&in)", asFUNCTION(ImGui_Text), asCALL_CDECL);
    engine->RegisterGlobalFunction("void TextDisabled(const string&in)", asFUNCTION(ImGui_TextDisabled), asCALL_CDECL);
    engine->RegisterGlobalFunction("void TextColored(color col, const string&in)", asFUNCTION(ImGui_TextColored), asCALL_CDECL);
    engine->RegisterGlobalFunction("void TextWrapped(const string&in)", asFUNCTION(ImGui_TextWrapped), asCALL_CDECL);
    engine->RegisterGlobalFunction("void LabelText(const string&in, const string&in)", asFUNCTION(ImGui_LabelText), asCALL_CDECL);
    engine->RegisterGlobalFunction("void Bullet()", asFUNCTIONPR(ImGui::Bullet, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void BulletText(const string&in)", asFUNCTION(ImGui_BulletText), asCALL_CDECL);

    // Widgets: Main
    engine->RegisterGlobalFunction("bool Button(const string&in, vector2 = vector2(0,0))", asFUNCTION(ImGui_Button), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool SmallButton(const string&in)", asFUNCTION(ImGui_SmallButton), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool InvisibleButton(const string&in, vector2)", asFUNCTION(ImGui_InvisibleButton), asCALL_CDECL);
    engine->RegisterGlobalFunction("void Image(const Ogre::TexturePtr&in, vector2)", asFUNCTION(ImGui_Image), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool Checkbox(const string&in, bool&inout)", asFUNCTION(ImGui_Checkbox), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool CheckboxFlags(const string&in, uint&inout, uint)", asFUNCTION(ImGui_CheckboxFlags), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool RadioButton(const string&in, bool)", asFUNCTION(ImGui_RadioButton), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool RadioButton(const string&in, int&inout, int)", asFUNCTION(ImGui_RadioButtonInt), asCALL_CDECL);
    engine->RegisterGlobalFunction("void ProgressBar(float, vector2=vector2(-1,0), const string&in = \"\")", asFUNCTION(ImGui_ProgressBar), asCALL_CDECL);


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
    engine->RegisterGlobalFunction("bool DragFloat(const string&in, float&inout, float = 1.0f, float = 0.0f, float = 0.0f)", asFUNCTION(ImGui_DragFloat), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool DragFloat2(const string&in, vector2&inout)", asFUNCTION(ImGui_DragFloat2), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool DragFloat3(const string&in, vector3&inout)", asFUNCTION(ImGui_DragFloat3), asCALL_CDECL);
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
    engine->RegisterGlobalFunction("bool DragFloatRange2(const string&in, float&inout, float&inout, float = 0.0f, float = 1.0f)", asFUNCTION(ImGui_DragFloatRange2), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool DragIntRange2(const string&in, int&inout, int&inout, int, int)", asFUNCTION(ImGui_DragIntRange2), asCALL_CDECL);

    // Widgets: Input with Keyboard
    engine->RegisterGlobalFunction("bool InputText(const string&in, string&inout)", asFUNCTION(ImGui_InputText), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool InputTextMultiline(const string&in, string&inout, const vector2&in = vector2(0,0))", asFUNCTION(ImGui_InputTextMultiline), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool InputFloat(const string&, float&inout)", asFUNCTION(ImGui_InputFloat), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool InputFloat2(const string&, vector2&inout)", asFUNCTION(ImGui_InputFloat2), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool InputFloat3(const string&, vector3&inout)", asFUNCTION(ImGui_InputFloat3), asCALL_CDECL);
    /* --- TODO: Register Vector4
    engine->RegisterGlobalFunction("bool InputFloat4(const string&, Vector4&inout)", asFUNCTIONPR([](const string& id, Vector4& val) {
        return ImGui::InputFloat4(id.c_str(), &val.x_); }, (const string&, Vector4&),bool), asCALL_CDECL);
        */
    engine->RegisterGlobalFunction("bool InputInt(const string&, int&inout)", asFUNCTION(ImGui_InputInt), asCALL_CDECL);
    /*
    engine->RegisterGlobalFunction("bool InputInt2(const string&, IntVector2&inout)", asFUNCTIONPR([](const string& id, IntVector2& val) {
        return ImGui::InputInt2(id.c_str(), &val.x_); }, (const string&, IntVector2&),bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool InputInt3(const string&, IntVector3&inout)", asFUNCTIONPR([](const string& id, IntVector3& val) {
        return ImGui::InputInt3(id.c_str(), &val.x_); }, (const string&, IntVector3&), bool), asCALL_CDECL);
        */

    // Widgets: Sliders (tip: ctrl+click on a slider to input with keyboard. manually input values aren't clamped, can go off-bounds)
    engine->RegisterGlobalFunction("bool SliderFloat(const string&in, float&inout, float = 0.0f, float = 0.0f)", asFUNCTION(ImGui_SliderFloat), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool SliderFloat2(const string&in, vector2&inout, float, float)", asFUNCTION(ImGui_SliderFloat2), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool SliderFloat3(const string&in, vector3&inout, float, float)", asFUNCTION(ImGui_SliderFloat3), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool SliderInt(const string&in, int&inout, int = 0, int = 0)", asFUNCTION(ImGui_SliderInt), asCALL_CDECL);
/* --- TODO: Register Vector4
    engine->RegisterGlobalFunction("bool SliderFloat4(const string&in, Vector4&inout, float, float)", asFUNCTIONPR([](const string& n, Vector4& v, float mn, float mx) {
        return ImGui::SliderFloat4(n.c_str(), &v.x, mn, mx); }, (const string&, Vector4&,float,float),bool), asCALL_CDECL);

    engine->RegisterGlobalFunction("bool SliderInt2(const string&in, IntVector2&inout, int = 0, int = 0)", asFUNCTIONPR([](const string& n, IntVector2& v, int mn, int mx) {
        return ImGui::SliderInt2(n.c_str(), &v.x, mn, mx); }, (const string&, IntVector2&, int,int),bool), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool SliderInt3(const string&in, IntVector3&inout, int = 0, int = 0)", asFUNCTIONPR([](const string& n, IntVector3& v, int mn, int mx) {
        return ImGui::SliderInt3(n.c_str(), &v.x, mn, mx); }, (const string&, IntVector3&, int,int),bool), asCALL_CDECL);
        */

    // Widgets: Color Editor/Picker
    engine->RegisterGlobalFunction("bool ColorEdit3(const string&in, color&inout)", asFUNCTION(ImGui_ColorEdit3), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool ColorEdit4(const string&in, color&inout)", asFUNCTION(ImGui_ColorEdit4), asCALL_CDECL);
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
    engine->RegisterGlobalFunction("bool ColorButton(const string&in, color)", asFUNCTION(ImGui_ColorButton), asCALL_CDECL);

    // Widgets: Trees
    engine->RegisterGlobalFunction("bool TreeNode(const string&in)", asFUNCTION(ImGui_TreeNode), asCALL_CDECL);
    engine->RegisterGlobalFunction("void TreePush(const string&in)", asFUNCTION(ImGui_TreePush), asCALL_CDECL);
    engine->RegisterGlobalFunction("void TreePop()", asFUNCTIONPR(ImGui::TreePop, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void TreeAdvanceToLabelPos()", asFUNCTIONPR(ImGui::TreeAdvanceToLabelPos, (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetTreeNodeToLabelSpacing()", asFUNCTIONPR(ImGui::GetTreeNodeToLabelSpacing, (), float), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetNextTreeNodeOpen(bool)", asFUNCTION(ImGui_SetNextTreeNodeOpen), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool CollapsingHeader(const string&in)", asFUNCTION(ImGui_CollapsingHeader), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool CollapsingHeader(const string&in, bool&inout)", asFUNCTION(ImGui_CollapsingHeaderClosable), asCALL_CDECL);

    // Widgets: Selectable / Lists
    engine->RegisterGlobalFunction("bool Selectable(const string&in, bool = false)", asFUNCTION(ImGui_Selectable), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool ListBoxHeader(const string&in)", asFUNCTION(ImGui_ListBoxHeader), asCALL_CDECL);

    // Values
    engine->RegisterGlobalFunction("void Value(const string&in, bool)", asFUNCTION(ImGui_ValueBool), asCALL_CDECL);
    engine->RegisterGlobalFunction("void Value(const string&in, int)", asFUNCTION(ImGui_ValueInt), asCALL_CDECL);
    engine->RegisterGlobalFunction("void Value(const string&in, uint)", asFUNCTION(ImGui_ValueUint), asCALL_CDECL);
    engine->RegisterGlobalFunction("void Value(const string&in, float)", asFUNCTION(ImGui_ValueFloat), asCALL_CDECL);

    // Tooltips
    engine->RegisterGlobalFunction("void BeginTooltip()", asFUNCTIONPR(ImGui::BeginTooltip  , (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void EndTooltip()", asFUNCTIONPR(ImGui::EndTooltip      , (), void), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetTooltip(const string&in)", asFUNCTION(ImGui_SetTooltip), asCALL_CDECL);

    // Menus
    engine->RegisterGlobalFunction("bool BeginMainMenuBar()", asFUNCTION(ImGui_BeginMainMenuBar), asCALL_CDECL);
    engine->RegisterGlobalFunction("void EndMainMenuBar()", asFUNCTION(ImGui_EndMainMenuBar), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool BeginMenuBar()", asFUNCTION(ImGui_BeginMenuBar), asCALL_CDECL);
    engine->RegisterGlobalFunction("void EndMenuBar()", asFUNCTION(ImGui_EndMenuBar), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool BeginMenu(const string&in, bool = true)", asFUNCTION(ImGui_BeginMenu), asCALL_CDECL);
    engine->RegisterGlobalFunction("void EndMenu()", asFUNCTION(ImGui_EndMenu), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool MenuItem(const string&in, const string&in = string(), bool = false, bool = true)", asFUNCTION(ImGui_MenuItem), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool MenuItem(const string&in, const string&in, bool &inout, bool = true)", asFUNCTION(ImGui_MenuItemToggle), asCALL_CDECL);

    // Popups
    engine->RegisterGlobalFunction("void OpenPopup(const string&in)", asFUNCTION(ImGui_OpenPopup), asCALL_CDECL);
   /* engine->RegisterGlobalFunction("bool BeginPopup(const string&in, int = 0)", asFUNCTIONPR([](const string& a, int b) {
        return ImGui::BeginPopup(a.c_str(), (ImGuiWindowFlags)b);  }, (const string&, int), bool), asCALL_CDECL);*/ // FIXME: update imgui!
    engine->RegisterGlobalFunction("bool BeginPopup(const string&in, int = 0)", asFUNCTION(ImGui_BeginPopup), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool BeginPopupContextItem(const string&in = string(), int = 1)", asFUNCTION(ImGui_BeginPopupContextItem), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool BeginPopupContextWindow(const string&in = string(), int = 1, bool = true)", asFUNCTION(ImGui_BeginPopupContextWindow), asCALL_CDECL); // FIXME: update imgui! -- swapped args
    engine->RegisterGlobalFunction("bool BeginPopupContextVoid(const string&in = string(), int = 1)", asFUNCTION(ImGui_BeginPopupContextVoid), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool BeginPopupModal(const string&in, bool &inout = null, int = 0)", asFUNCTION(ImGui_BeginPopupModal), asCALL_CDECL);
    engine->RegisterGlobalFunction("void EndPopup()", asFUNCTION(ImGui_EndPopup), asCALL_CDECL);
/*    engine->RegisterGlobalFunction("bool OpenPopupOnItemClick(const string&in = string(), int = 1)", asFUNCTIONPR([](const string& a, int b) {
        return ImGui::OpenPopupOnItemClick(a.c_str(), b);  }, (const string&, int), bool), asCALL_CDECL);*/ // FIXME: update imgui!
 /*   engine->RegisterGlobalFunction("bool IsPopupOpen(const string&in)", asFUNCTIONPR([](const string& a) {
        return ImGui::IsPopupOpen(a.c_str());  }, (const string&), bool), asCALL_CDECL); */ // FIXME: update imgui!
    engine->RegisterGlobalFunction("void CloseCurrentPopup()", asFUNCTION(ImGui_CloseCurrentPopup), asCALL_CDECL);

    // Clip-rects
    engine->RegisterGlobalFunction("void PushClipRect(const vector2&, const vector2&, bool)", asFUNCTION(ImGui_PushClipRect), asCALL_CDECL);
    engine->RegisterGlobalFunction("void PopClipRect()", asFUNCTION(ImGui_PopClipRect), asCALL_CDECL);

    // Focus
 /*   engine->RegisterGlobalFunction("void SetItemDefaultFocus()", asFUNCTIONPR([]() {  ImGui::SetItemDefaultFocus();  }, (), void), asCALL_CDECL); */ // TODO update imgui
    engine->RegisterGlobalFunction("void SetKeyboardFocusHere(int = 0)", asFUNCTION(ImGui_SetKeyboardFocusHere), asCALL_CDECL);

    // Utilities
    engine->RegisterGlobalFunction("bool IsItemHovered(int = 0)", asFUNCTION(ImGui_IsItemHovered), asCALL_CDECL); // TODO: update imgui -- flags omitted
    engine->RegisterGlobalFunction("bool IsItemActive()", asFUNCTION(ImGui_IsItemActive), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsItemClicked(int = 0)", asFUNCTION(ImGui_IsItemClicked), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsItemVisible()", asFUNCTION(ImGui_IsItemVisible), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsAnyItemHovered()", asFUNCTION(ImGui_IsAnyItemHovered), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsAnyItemActive()", asFUNCTION(ImGui_IsAnyItemActive), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetItemRectMin()", asFUNCTION(ImGui_GetItemRectMin), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetItemRectMax()", asFUNCTION(ImGui_GetItemRectMax), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetItemRectSize()", asFUNCTION(ImGui_GetItemRectSize), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetItemAllowOverlap()", asFUNCTION(ImGui_SetItemAllowOverlap), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsWindowFocused(int = 0)", asFUNCTION(ImGui_IsWindowFocused), asCALL_CDECL); // TODO: update imgui -- flags omitted
    engine->RegisterGlobalFunction("bool IsWindowHovered(int = 0)", asFUNCTION(ImGui_IsWindowHovered), asCALL_CDECL); // TODO: update imgui -- flags omitted
    engine->RegisterGlobalFunction("bool IsRectVisible(const vector2&)", asFUNCTION(ImGui_IsRectVisibleSize), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsRectVisible(const vector2&, const vector2&)", asFUNCTION(ImGui_IsRectVisible), asCALL_CDECL);
    engine->RegisterGlobalFunction("float GetTime()", asFUNCTION(ImGui_GetTime), asCALL_CDECL);
    engine->RegisterGlobalFunction("int GetFrameCount()", asFUNCTION(ImGui_GetFrameCount), asCALL_CDECL);

    engine->RegisterGlobalFunction("vector2 CalcTextSize(const string&in, bool hide_text_after_double_hash = false, float wrap_width = -1.0f)", asFUNCTION(ImGui_CalcTextSize), asCALL_CDECL);
    engine->RegisterGlobalFunction("void CalcListClipping(int, float, int&inout, int&inout)", asFUNCTION(ImGui_CalcListClipping), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool BeginChildFrame(uint, const vector2&, int = 0)", asFUNCTION(ImGui_BeginChildFrame), asCALL_CDECL);
    engine->RegisterGlobalFunction("void EndChildFrame()", asFUNCTION(ImGui_EndChildFrame), asCALL_CDECL);

    engine->RegisterGlobalFunction("int GetKeyIndex(int)", asFUNCTION(ImGui_GetKeyIndex), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsKeyDown(int)", asFUNCTION(ImGui_IsKeyDown), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsKeyPressed(int, bool = true)", asFUNCTION(ImGui_IsKeyPressed), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsKeyReleased(int)", asFUNCTION(ImGui_IsKeyReleased), asCALL_CDECL);
    /*engine->RegisterGlobalFunction("int GetKeyPressedAmount(int, float, float)", asFUNCTIONPR([](int a, float b, float c) {  return ImGui::GetKeyPressedAmount(a, b, c);  }, (int,float,float), int), asCALL_CDECL);*/  // FIXME update imgui
    engine->RegisterGlobalFunction("bool IsMouseDown(int)", asFUNCTION(ImGui_IsMouseDown), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsMouseClicked(int, bool = false)", asFUNCTION(ImGui_IsMouseClicked), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsMouseDoubleClicked(int)", asFUNCTION(ImGui_IsMouseDoubleClicked), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsMouseReleased(int)", asFUNCTION(ImGui_IsMouseReleased), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsMouseDragging(int = 0, float = -1.0f)", asFUNCTION(ImGui_IsMouseDragging), asCALL_CDECL);
    engine->RegisterGlobalFunction("bool IsMouseHoveringRect(const vector2&in, const vector2&in, bool = true)", asFUNCTION(ImGui_IsMouseHoveringRect), asCALL_CDECL);
  /*  engine->RegisterGlobalFunction("bool IsMousePosValid(const Vector2&in)", asFUNCTIONPR([](const Vector2& a) {  auto v = ImVec2(a.x, a.y);  return ImGui::IsMousePosValid(&v);  }, (const Vector2&), bool), asCALL_CDECL); */ // FIXME update imgui
    engine->RegisterGlobalFunction("vector2 GetMousePos()", asFUNCTION(ImGui_GetMousePos), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetMousePosOnOpeningCurrentPopup()", asFUNCTION(ImGui_GetMousePosOnOpeningCurrentPopup), asCALL_CDECL);
    engine->RegisterGlobalFunction("vector2 GetMouseDragDelta(int = 0, float = -1.0f)", asFUNCTION(ImGui_GetMouseDragDelta), asCALL_CDECL);
    engine->RegisterGlobalFunction("void ResetMouseDragDelta(int = 0)", asFUNCTION(ImGui_ResetMouseDragDelta), asCALL_CDECL);
    engine->RegisterGlobalFunction("int GetMouseCursor()", asFUNCTION(ImGui_GetMouseCursor), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetMouseCursor(int)", asFUNCTION(ImGui_SetMouseCursor), asCALL_CDECL);
    engine->RegisterGlobalFunction("void CaptureKeyboardFromApp(bool = true)", asFUNCTION(ImGui_CaptureKeyboardFromApp), asCALL_CDECL);
    engine->RegisterGlobalFunction("void CaptureMouseFromApp(bool = true)", asFUNCTION(ImGui_CaptureMouseFromApp), asCALL_CDECL);


    engine->RegisterGlobalFunction("string GetClipboardText()", asFUNCTION(ImGui_GetClipboardText), asCALL_CDECL);
    engine->RegisterGlobalFunction("void SetClipboardText(const string&in)", asFUNCTION(ImGui_SetClipboardText), asCALL_CDECL);

    // Data plotting - we wrap the 'getter func' variant to resemble the 'float*' variant.
    engine->RegisterGlobalFunction("void PlotLines(const string&in label, array<float>&in values, int values_count, int values_offset = 0, const string&in overlay_text = string(), float scale_min = FLT_MAX, float scale_max = FLT_MAX, vector2 graph_size = vector2(0,0))", asFUNCTION(ImGui_PlotLines), asCALL_CDECL);

    engine->SetDefaultNamespace("");
}


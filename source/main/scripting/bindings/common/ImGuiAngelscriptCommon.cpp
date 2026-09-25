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

#include "AngelScriptBindings.h"
#include "OgreImGui.h"

#include <angelscript.h>

using namespace AngelScript;

void RoR::RegisterImGuiCommonBindings(AngelScript::asIScriptEngine* engine)
{
    // ENUMS (global namespace)
    // PLEASE maintain the same order as in 'doc/angelscript/Script2Game/AngelImGui/AngelImGui_enums.h'

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

    // Enumeration for PushStyleColor() / PopStyleColor()
    engine->RegisterEnum("ImGuiCol");
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_Text"                    , ImGuiCol_Text);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_TextDisabled"            , ImGuiCol_TextDisabled);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_WindowBg"                , ImGuiCol_WindowBg);              
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ChildBg"                 , ImGuiCol_ChildBg);               
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_PopupBg"                 , ImGuiCol_PopupBg);               
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_Border"                  , ImGuiCol_Border);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_BorderShadow"            , ImGuiCol_BorderShadow);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_FrameBg"                 , ImGuiCol_FrameBg);               
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_FrameBgHovered"          , ImGuiCol_FrameBgHovered);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_FrameBgActive"           , ImGuiCol_FrameBgActive);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_TitleBg"                 , ImGuiCol_TitleBg);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_TitleBgActive"           , ImGuiCol_TitleBgActive);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_TitleBgCollapsed"        , ImGuiCol_TitleBgCollapsed);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_MenuBarBg"               , ImGuiCol_MenuBarBg);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ScrollbarBg"             , ImGuiCol_ScrollbarBg);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ScrollbarGrab"           , ImGuiCol_ScrollbarGrab);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ScrollbarGrabHovered"    , ImGuiCol_ScrollbarGrabHovered);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ScrollbarGrabActive"     , ImGuiCol_ScrollbarGrabActive);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_CheckMark"               , ImGuiCol_CheckMark);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_SliderGrab"              , ImGuiCol_SliderGrab);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_SliderGrabActive"        , ImGuiCol_SliderGrabActive);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_Button"                  , ImGuiCol_Button);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ButtonHovered"           , ImGuiCol_ButtonHovered);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ButtonActive"            , ImGuiCol_ButtonActive);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_Header"                  , ImGuiCol_Header);                
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_HeaderHovered"           , ImGuiCol_HeaderHovered);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_HeaderActive"            , ImGuiCol_HeaderActive);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_Separator"               , ImGuiCol_Separator);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_SeparatorHovered"        , ImGuiCol_SeparatorHovered);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_SeparatorActive"         , ImGuiCol_SeparatorActive);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ResizeGrip"              , ImGuiCol_ResizeGrip);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ResizeGripHovered"       , ImGuiCol_ResizeGripHovered);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ResizeGripActive"        , ImGuiCol_ResizeGripActive);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_Tab"                     , ImGuiCol_Tab);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_TabHovered"              , ImGuiCol_TabHovered);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_TabActive"               , ImGuiCol_TabActive);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_TabUnfocused"            , ImGuiCol_TabUnfocused);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_TabUnfocusedActive"      , ImGuiCol_TabUnfocusedActive);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_PlotLines"               , ImGuiCol_PlotLines);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_PlotLinesHovered"        , ImGuiCol_PlotLinesHovered);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_PlotHistogram"           , ImGuiCol_PlotHistogram);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_PlotHistogramHovered"    , ImGuiCol_PlotHistogramHovered);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_TextSelectedBg"          , ImGuiCol_TextSelectedBg);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_DragDropTarget"          , ImGuiCol_DragDropTarget);
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_NavHighlight"            , ImGuiCol_NavHighlight);          
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_NavWindowingHighlight"   , ImGuiCol_NavWindowingHighlight); 
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_NavWindowingDimBg"       , ImGuiCol_NavWindowingDimBg);     
    engine->RegisterEnumValue("ImGuiCol", "ImGuiCol_ModalWindowDimBg"        , ImGuiCol_ModalWindowDimBg);      

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

    // ImDrawList object (global namespace)
    engine->RegisterObjectType("ImDrawList", sizeof(ImDrawList), asOBJ_REF | asOBJ_NOCOUNT);
}

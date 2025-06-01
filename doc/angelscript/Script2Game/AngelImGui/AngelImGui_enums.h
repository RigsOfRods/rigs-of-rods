
  // =================================================== //
  // THIS IS NOT A C++ HEADER! Only a dummy for Doxygen. //
  // =================================================== //

// NOTE: ImGui enums are in global namespace, not `ImGui::`
namespace Script2Game { // Dummy namespace, just to distinguish AngelScript from C++

/** \addtogroup ScriptSideAPIs
 *  @{
 */    

/** \addtogroup Script2Game
 *  @{
 */   

enum ImGuiStyleVar
{
    ImGuiStyleVar_Alpha               , //!< float     Alpha
    ImGuiStyleVar_WindowPadding       ,//!< ImVec2    WindowPadding
    ImGuiStyleVar_WindowRounding      , //!< float     WindowRounding
    ImGuiStyleVar_WindowBorderSize    ,  //!< float     WindowBorderSize
    ImGuiStyleVar_WindowMinSize       , //!< ImVec2    WindowMinSize
    ImGuiStyleVar_WindowTitleAlign    ,  //!< ImVec2    WindowTitleAlign
    ImGuiStyleVar_ChildRounding       , //!< float     ChildRounding
    ImGuiStyleVar_ChildBorderSize     , //!< float     ChildBorderSize
    ImGuiStyleVar_PopupRounding       , //!< float     PopupRounding
    ImGuiStyleVar_PopupBorderSize     , //!< float     PopupBorderSize
    ImGuiStyleVar_FramePadding        ,//!< ImVec2    FramePadding
    ImGuiStyleVar_FrameRounding       ,//!< float     FrameRounding
    ImGuiStyleVar_FrameBorderSize     , //!< float     FrameBorderSize
    ImGuiStyleVar_ItemSpacing         , //!< ImVec2    ItemSpacing
    ImGuiStyleVar_ItemInnerSpacing    ,  //!< ImVec2    ItemInnerSpacing
    ImGuiStyleVar_IndentSpacing       ,  //!< float     IndentSpacing
    ImGuiStyleVar_ScrollbarSize       ,  //!< float     ScrollbarSize
    ImGuiStyleVar_ScrollbarRounding   , //!< float     ScrollbarRounding
    ImGuiStyleVar_GrabMinSize         ,  //!< float     GrabMinSize
    ImGuiStyleVar_GrabRounding        ,  //!< float     GrabRounding
    ImGuiStyleVar_TabRounding         ,  //!< float     TabRounding
    ImGuiStyleVar_ButtonTextAlign     ,  //!< ImVec2    ButtonTextAlign
    ImGuiStyleVar_SelectableTextAlign , //!< ImVec2    SelectableTextAlign
}

enum ImGuiWindowFlags
{
    ImGuiWindowFlags_None                      
    ImGuiWindowFlags_NoTitleBar               ,   //!< Disable title-bar
    ImGuiWindowFlags_NoResize                 ,   //!< Disable user resizing with the lower-right grip
    ImGuiWindowFlags_NoMove                   ,   //!< Disable user moving the window
    ImGuiWindowFlags_NoScrollbar              ,   //!< Disable scrollbars (window can still scroll with mouse or programmatically)
    ImGuiWindowFlags_NoScrollWithMouse        ,   //!< Disable user vertically scrolling with mouse wheel. On child window, mouse wheel will be forwarded to the parent unless NoScrollbar is also set.
    ImGuiWindowFlags_NoCollapse               ,   //!< Disable user collapsing window by double-clicking on it
    ImGuiWindowFlags_AlwaysAutoResize         ,   //!< Resize every window to its content every frame
    ImGuiWindowFlags_NoBackground             ,   //!< Disable drawing background color (WindowBg, etc.) and outside border. Similar as using SetNextWindowBgAlpha(0.0f).
    ImGuiWindowFlags_NoSavedSettings          ,   //!< Never load/save settings in .ini file
    ImGuiWindowFlags_NoMouseInputs            ,   //!< Disable catching mouse, hovering test with pass through.
    ImGuiWindowFlags_MenuBar                  ,   //!< Has a menu-bar
    ImGuiWindowFlags_HorizontalScrollbar      ,   //!< Allow horizontal scrollbar to appear (off by default). You may use SetNextWindowContentSize(ImVec2(width,0.0f)); prior to calling Begin() to specify width. Read code in imgui_demo in the "Horizontal Scrolling" section.
    ImGuiWindowFlags_NoFocusOnAppearing       ,   //!< Disable taking focus when transitioning from hidden to visible state
    ImGuiWindowFlags_NoBringToFrontOnFocus    ,   //!< Disable bringing window to front when taking focus (e.g. clicking on it or programmatically giving it focus)
    ImGuiWindowFlags_AlwaysVerticalScrollbar  ,   //!< Always show vertical scrollbar (even if ContentSize.y < Size.y)
    ImGuiWindowFlags_AlwaysHorizontalScrollbar,   //!< Always show horizontal scrollbar (even if ContentSize.x < Size.x)
    ImGuiWindowFlags_AlwaysUseWindowPadding   ,   //!< Ensure child windows without border uses style.WindowPadding (ignored by default for non-bordered child windows, because more convenient)
    ImGuiWindowFlags_NoNavInputs              ,   //!< No gamepad/keyboard navigation within the window
    ImGuiWindowFlags_NoNavFocus               ,   //!< No focusing toward this window with gamepad/keyboard navigation (e.g. skipped by CTRL+TAB)
    ImGuiWindowFlags_UnsavedDocument          ,   //!< Append '*' to title without affecting the ID, as a convenience to avoid using the ### operator. When used in a tab/docking context, tab is selected on closure and closure is deferred by one frame to allow code to cancel the closure (with a confirmation popup, etc.) without flicker.
    ImGuiWindowFlags_NoNav                    ,   //!< ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus,
    ImGuiWindowFlags_NoDecoration             ,   //!< ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse,
    ImGuiWindowFlags_NoInputs                 ,   //!< ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus,
}

/// Enumeration for AngelImGui::PushStyleColor() / AngelImGui::PopStyleColor()
enum ImGuiCol
{
    ImGuiCol_Text,
    ImGuiCol_TextDisabled,
    ImGuiCol_WindowBg,              //!< Background of normal windows
    ImGuiCol_ChildBg,               //!< Background of child windows
    ImGuiCol_PopupBg,               //!< Background of popups, menus, tooltips windows
    ImGuiCol_Border,
    ImGuiCol_BorderShadow,
    ImGuiCol_FrameBg,               //!< Background of checkbox, radio button, plot, slider, text input
    ImGuiCol_FrameBgHovered,
    ImGuiCol_FrameBgActive,
    ImGuiCol_TitleBg,
    ImGuiCol_TitleBgActive,
    ImGuiCol_TitleBgCollapsed,
    ImGuiCol_MenuBarBg,
    ImGuiCol_ScrollbarBg,
    ImGuiCol_ScrollbarGrab,
    ImGuiCol_ScrollbarGrabHovered,
    ImGuiCol_ScrollbarGrabActive,
    ImGuiCol_CheckMark,
    ImGuiCol_SliderGrab,
    ImGuiCol_SliderGrabActive,
    ImGuiCol_Button,
    ImGuiCol_ButtonHovered,
    ImGuiCol_ButtonActive,
    ImGuiCol_Header,                //!< Header* colors are used for CollapsingHeader, TreeNode, Selectable, MenuItem
    ImGuiCol_HeaderHovered,
    ImGuiCol_HeaderActive,
    ImGuiCol_Separator,
    ImGuiCol_SeparatorHovered,
    ImGuiCol_SeparatorActive,
    ImGuiCol_ResizeGrip,
    ImGuiCol_ResizeGripHovered,
    ImGuiCol_ResizeGripActive,
    ImGuiCol_Tab,
    ImGuiCol_TabHovered,
    ImGuiCol_TabActive,
    ImGuiCol_TabUnfocused,
    ImGuiCol_TabUnfocusedActive,
    ImGuiCol_PlotLines,
    ImGuiCol_PlotLinesHovered,
    ImGuiCol_PlotHistogram,
    ImGuiCol_PlotHistogramHovered,
    ImGuiCol_TextSelectedBg,
    ImGuiCol_DragDropTarget,
    ImGuiCol_NavHighlight,          //!< Gamepad/keyboard: current highlighted item
    ImGuiCol_NavWindowingHighlight, //!< Highlight window when using CTRL+TAB
    ImGuiCol_NavWindowingDimBg,     //!< Darken/colorize entire screen behind the CTRL+TAB window list, when active
    ImGuiCol_ModalWindowDimBg,      //!< Darken/colorize entire screen behind a modal window, when one is active
}

/// Enumateration for SetWindow***(), SetNextWindow***(), SetNextItem***() functions; Represent a condition.
/// Important: Treat as a regular enum! Do NOT combine multiple values using binary operators! All the functions above treat 0 as a shortcut to ImGuiCond_Always.
enum ImGuiCond
{
    ImGuiCond_Always      ,   //!< Set the variable
    ImGuiCond_Once        ,   //!< Set the variable once per runtime session (only the first call with succeed)
    ImGuiCond_FirstUseEver,   //!< Set the variable if the object/window has no persistently saved data (no entry in .ini file)
    ImGuiCond_Appearing   ,   //!< Set the variable if the object/window is appearing after being hidden/inactive (or the first time)
};

/// Flags for AngelImGui::BeginTabBar()
enum ImGuiTabBarFlags
{
    ImGuiTabBarFlags_None                           ,
    ImGuiTabBarFlags_Reorderable                    , //!< Allow manually dragging tabs to re-order them + New tabs are appended at the end of list
    ImGuiTabBarFlags_AutoSelectNewTabs              , //!< Automatically select new tabs when they appear
    ImGuiTabBarFlags_TabListPopupButton             , //!< Disable buttons to open the tab list popup
    ImGuiTabBarFlags_NoCloseWithMiddleMouseButton   , //!< Disable behavior of closing tabs (that are submitted with p_open != NULL) with middle mouse button. You can still repro this behavior on user's side with if (IsItemHovered() && IsMouseClicked(2)) *p_open = false.
    ImGuiTabBarFlags_NoTabListScrollingButtons      , //!< Disable scrolling buttons (apply when fitting policy is ImGuiTabBarFlags_FittingPolicyScroll)
    ImGuiTabBarFlags_NoTooltip                      , //!< Disable tooltips when hovering a tab
    ImGuiTabBarFlags_FittingPolicyResizeDown        , //!< Resize tabs when they don't fit
    ImGuiTabBarFlags_FittingPolicyScroll            , //!< Add scroll buttons when tabs don't fit
    ImGuiTabBarFlags_FittingPolicyMask_             , //!< ImGuiTabBarFlags_FittingPolicyResizeDown | ImGuiTabBarFlags_FittingPolicyScroll,
    ImGuiTabBarFlags_FittingPolicyDefault_          , //!< ImGuiTabBarFlags_FittingPolicyResizeDown
};

// Flags for AngelImGui::BeginTabItem()
enum ImGuiTabItemFlags
{
    ImGuiTabItemFlags_None                          ,
    ImGuiTabItemFlags_UnsavedDocument               ,   //!< Append '*' to title without affecting the ID, as a convenience to avoid using the ### operator. Also: tab is selected on closure and closure is deferred by one frame to allow code to undo it without flicker.
    ImGuiTabItemFlags_SetSelected                   ,   //!< Trigger flag to programmatically make the tab selected when calling BeginTabItem()
    ImGuiTabItemFlags_NoCloseWithMiddleMouseButton  ,   //!< Disable behavior of closing tabs (that are submitted with p_open != NULL) with middle mouse button. You can still repro this behavior on user's side with if (IsItemHovered() && IsMouseClicked(2)) *p_open = false.
    ImGuiTabItemFlags_NoPushId                      ,   //!< Don't call PushID(tab->ID)/PopID() on BeginTabItem()/EndTabItem()
};

/// @}    //addtogroup Script2Game
/// @}    //addtogroup ScriptSideAPIs

} // namespace Script2Game (dummy, just to distinguish AngelScript from C++)
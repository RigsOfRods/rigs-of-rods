
// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

void frameStep(float dt)
{
    if (ImGui::Begin("Example", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
    
        drawExampleTabs();
        
        ImGui::End();
    }

}

// settings
bool demotabsReorderable = false;
bool demotabsNoTooltip = true;
// closable tabs
bool demotabOpentabChecks = true;
bool demotabOpentabEmpty = true;

void drawExampleTabs()
{
    // TABS, TAB BARS:
    //bool BeginTabBar(const string&in, int TabBarFlags = 0)
    //void EndTabBar()
    //bool BeginTabItem(const string&in, int = 0)                             // BeginTabItem() without X close button.
    //bool BeginTabItem(const string&in, bool&out , int TabItemFlags = 0)     // BeginTabItem() with X close button.
    //void EndTabItem()
    //bool TabItemButton(const string&in, int TabItemFlags = 0)
    //void SetTabItemClosed(const string&in)

    // IMGUI TAB BAR FLAGS:
    //ImGuiTabBarFlags_Reorderable); // // Allow manually dragging tabs to re-order them + New tabs are appended at the end of list
    //ImGuiTabBarFlags_AutoSelectNewTabs); // // Automatically select new tabs when they appear
    //ImGuiTabBarFlags_TabListPopupButton); // // Disable buttons to open the tab list popup
    //ImGuiTabBarFlags_NoCloseWithMiddleMouseButton); // // Disable behavior of closing tabs (that are submitted with p_open != NULL) with middle mouse button. You can still repro this behavior on user's side with if (IsItemHovered() && IsMouseClicked(2)) *p_open = false.
    //ImGuiTabBarFlags_NoTabListScrollingButtons); // // Disable scrolling buttons (apply when fitting policy is ImGuiTabBarFlags_FittingPolicyScroll)
    //ImGuiTabBarFlags_NoTooltip); // // Disable tooltips when hovering a tab
    //ImGuiTabBarFlags_FittingPolicyResizeDown); // // Resize tabs when they don't fit
    //ImGuiTabBarFlags_FittingPolicyScroll); // // Add scroll buttons when tabs don't fit  
    // // ============================================================================
    
    int barflags =  
         ImGuiTabBarFlags_NoCloseWithMiddleMouseButton 
        | ImGuiTabBarFlags_NoTabListScrollingButtons
        | ImGuiTabBarFlags_FittingPolicyResizeDown;
    if (demotabsReorderable)
        barflags = barflags | ImGuiTabBarFlags_Reorderable; 
    if (demotabsNoTooltip)    
        barflags = barflags |= ImGuiTabBarFlags_NoTooltip;
        
    if (ImGui::BeginTabBar("demotabs", barflags))
    {

        if (ImGui::BeginTabItem("Text"))
        {
            // just text
            ImGui::TextDisabled(">1");
            ImGui::TextDisabled(">>2");
            ImGui::TextDisabled(">>>3");
            ImGui::TextDisabled(">>>>4\n>>>>>5\n>>>>>>6");
            
            ImGui::EndTabItem();
        }
        

        if (ImGui::BeginTabItem("Reset"))
        {
            // boring text
            if (ImGui::Button("Reset closed tabs"))
            {
                demotabOpentabChecks=true;
                demotabOpentabEmpty=true;
            }
            
            ImGui::EndTabItem();
        }        
        if ( ImGui::BeginTabItem("Checks", /*inout:*/demotabOpentabChecks))
        {
            // chekcboxes
            ImGui::Checkbox("reorderable", /*inout:*/demotabsReorderable);
            ImGui::Checkbox("no tooltip", /*inout:*/demotabsNoTooltip);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Empty", /*inout:*/demotabOpentabEmpty))
        {
            // empty
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    
    ImGui::Separator();
    ImGui::Text("nothing interesting down here...");
}


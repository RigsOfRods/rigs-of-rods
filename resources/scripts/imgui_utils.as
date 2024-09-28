/// \title UI tools based on DearIMGUI
/// \brief Hyperlink drawing
/// Functions:
/// `void ImHyperlink(string url, string caption, bool tooltip=true)` ~ Full-featured hypertext with tooltip showing full URL.
/// `void ImDummyHyperlink(string caption)` ~ Looks and behaves (mouuse cursor) like a hypertext, but doesn't open URL.
/// `ImDrawList@ ImGetDummyFullscreenWindow(string name) ~ Convenience helper for ad-hoc drawing on screen.
/// Classes:
/// `CloseWindowPrompt` ~ Convenient handler of the [X] close button - draws "Really exit?" prompt and then exits the script.
// --------------------------------------------------------------------------

// By convention, all includes have suffix _utils and namespace matching filename.
namespace imgui_utils
{

    /// Looks and behaves (mouuse cursor) like a hypertext, but doesn't open URL.
    void ImDummyHyperlink(string caption)
    {
        const color LINKCOLOR = color(0.3, 0.5, 0.9, 1.0);
        ImGui::PushStyleColor(0, LINKCOLOR);  //Text
        vector2 cursorBefore=ImGui::GetCursorScreenPos();
        ImGui::Text(caption);
        vector2 textSize = ImGui::CalcTextSize(caption);
        ImGui::GetWindowDrawList().AddLine(
             cursorBefore+vector2(0,textSize.y), cursorBefore+textSize,  //from-to
             LINKCOLOR);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetMouseCursor(7);//Hand cursor
        }
        ImGui::PopStyleColor(1); //Text
    }

    /// Full-featured hypertext with tooltip showing full URL.
    void ImHyperlink(string url, string caption="", bool tooltip=true)
    {
        if (caption == "") { caption = url; tooltip=false; }
        ImDummyHyperlink(caption);
        if (ImGui::IsItemClicked())
        {
            game.openUrlInDefaultBrowser(url);
        }
        if (tooltip && ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImDummyHyperlink(url);
            ImGui::EndTooltip();
        }
    }
    
    /// Convenience helper for ad-hoc drawing on screen.
    ImDrawList@ ImGetDummyFullscreenWindow(const string&in name)
    {
        // Dummy fullscreen window to draw to
        int windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar| ImGuiWindowFlags_NoInputs 
                         | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
        ImGui::SetNextWindowPos(vector2(0,0));
        ImGui::SetNextWindowSize(game.getDisplaySize());
        ImGui::PushStyleColor(ImGuiCol_WindowBg, color(0.f,0.f,0.f,0.f)); // Fully transparent background!
        bool windowOpen = true;
        ImGui::Begin(name, windowOpen, windowFlags);
        ImDrawList@ drawlist = ImGui::GetWindowDrawList();
        ImGui::End();
        ImGui::PopStyleColor(1); // WindowBg

        return drawlist;    
    }    

    /// Convenient handler of the [X] close button - draws "Really exit?" prompt and then exits the script.
    class CloseWindowPrompt
    {
        //Config:
        color cfgCloseWindowPromptBgColor(0.55f, 0.5f, 0.1f, 1.f);
        color cfgCloseWindowPromptTextColor(0.1, 0.1, 0.1, 1.0);
        string cfgPromptText = "Terminate the script?";
        bool cfgCloseImmediatelly = false;
        vector2 cfgPromptBoxPadding(3, 3);
        //State:
        bool windowOpen = true;

        void draw() // Works best when drawn as first thing on top of the window.
        {
            // from <imgui.h>
            const int    ImGuiCol_Text = 0;        
            bool exitRequested = false;
        
            if (this.windowOpen)
            {
                return;  // nothing to draw right now
            }
            else if (!this.cfgCloseImmediatelly)
            {
                ImGui::AlignTextToFramePadding();
                
                // Draw colored background
                
                vector2 cursor = ImGui::GetCursorScreenPos();
                vector2 topLeft =  cursor - cfgPromptBoxPadding;
                vector2 bottomRight = cursor + cfgPromptBoxPadding + vector2(    ImGui::GetWindowContentRegionMax().x,     ImGui::GetTextLineHeightWithSpacing());
                ImGui::GetWindowDrawList().AddRectFilled(topLeft, bottomRight, cfgCloseWindowPromptBgColor);
                
                // Draw text with contrasting color
                ImGui::PushStyleColor(ImGuiCol_Text, cfgCloseWindowPromptTextColor);
                ImGui::Text(cfgPromptText);
                ImGui::PopStyleColor(); // Text
                ImGui::SameLine();
                
                // Draw yes/no buttons
                if(ImGui::SmallButton("yes"))
                {
                    exitRequested = true;
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("no"))
                {
                    this.windowOpen=true;
                }
            }
            else
            {
                exitRequested = true;
            }
            
            // Close the script if requested
            if (exitRequested)
            {
                game.pushMessage(MSG_APP_UNLOAD_SCRIPT_REQUESTED, { {'id', thisScript} }); // `thisScript` is global variable set by the game.            
            }
        }        
    }
} // namespace imgui_utils


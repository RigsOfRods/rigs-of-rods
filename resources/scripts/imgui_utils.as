/// \title UI tools based on DearIMGUI
/// \brief Hyperlink drawing
/// Functions:
/// `void ImHyperlink(string url, string caption, bool tooltip=true)` ~ Full-featured hypertext with tooltip showing full URL.
/// `void ImDummyHyperlink(string caption)` ~ Looks and behaves (mouuse cursor) like a hypertext, but doesn't open URL.
// --------------------------------------------------------------------------

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


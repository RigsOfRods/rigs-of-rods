#include "AppDraw.h"
using namespace ckeys;

//  draw utils
//------------------------------------------------------------------

//  write out text
int AppDraw::Txt(int x, int y, bool draw)
{
    // TODO: bold text
    if (draw)
    {
        ImVec2 orig_pos = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(x, y));
        ImGui::TextColored(clr, "%s", str.c_str());
        ImGui::SetCursorPos(orig_pos);
    }
    return (int) ImGui::CalcTextSize(str.c_str()).x;
}

//  clear rect
void AppDraw::Rect(int x, int y,  int sx, int sy,
		  uint8_t r, uint8_t g, uint8_t b)
{
    ImDrawList* drawlist = ImGui::GetWindowDrawList();
    drawlist->AddRectFilled(ImVec2(x,y), ImVec2(x+sx, y+sy), ImColor(r,g,b,1));
}

//  frame rect, inefficient
void AppDraw::Frame(int x, int y,  int sx, int sy,  int d,
		uint8_t r, uint8_t g, uint8_t b)
{
	Rect(x,   y,    sx-d, y+d,  r, g, b);  // top
	Rect(x,   sy-d, sx-d, sy,   r, g, b);  // bottom
	Rect(x,   y,    x+d,  sy-d, r, g, b);  // left
	Rect(sx-d,y,    sx,   sy,   r, g, b);  // right
}

/// \title 2D GRID VIEWER
/// \brief Generic scrolling & zooming UI for drawing via ImDrawList
// ===================================================

class GridViewer
{
  // CFG:
  string childWindowID;
  float zoomMax = 100.f;
  float zoomStep = 1.f;
  int hAxis=0;
  int vAxis=1;
  array<color> axisLineColors = { color(1,0,0,1), color(0.2,0.3,1,1), color(0.1, 0.9, 0.05, 1) };
  vector2 scrollRegionPadding = vector2(50,50);
  vector2 controlsBoxPadding = vector2(4,4);
  int gridSizeMax = 10000;

  // STATE:
  float zoom = 1.f;
  vector2 contentRegionSize;
  vector2 midWindowScreenPos;
  vector2 childScreenPos;
  vector2 nodesMin;
  vector2 nodesMax;
  int gridSize = 25;
  int gridSizeStep = 5;

  // FUNCS:
  void begin(vector2 size) {
    int flags = 1 << 11; // _HorizontalScrollbar
    this.childScreenPos=ImGui::GetCursorScreenPos();
    ImGui::BeginChild(childWindowID, size, true, flags);
    contentRegionSize=ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin();
    midWindowScreenPos = ImGui::GetWindowPos()+ contentRegionSize/2;
    nodesMin=vector2(FLT_MAX, FLT_MAX);
    nodesMax=vector2(-999999, -999999);
  }

  void drawControls() {
    ImGui::TextDisabled(this.childWindowID);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(65);
    ImGui::SliderFloat('Zoom', zoom, zoomStep, zoomMax);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(65);
    ImGui::SliderInt("Grid", gridSize, gridSizeStep, gridSizeMax);
  }

  vector2 projectPos(vector3 posIn) {
    // zoom only, without scrolling
    vector2 pos= midWindowScreenPos+vector2(posIn[hAxis], posIn[vAxis])*zoom;
    nodesMin=vector2(fmin(nodesMin.x, pos.x), fmin(nodesMin.y, pos.y));
    nodesMax=vector2(fmax(nodesMax.x, pos.x), fmax(nodesMax.y, pos.y));
    // add scrolling
    pos += vector2(
      ImGui::GetScrollMaxX() - 2*ImGui::GetScrollX(),
      ImGui::GetScrollMaxY() - 2*ImGui::GetScrollY());
    return pos;
  }

  void drawAxisLines() {
    vector2 origin = this.projectPos(vector3(0,0,0));
    ImGui::GetWindowDrawList().AddLine(origin+vector2(0, -9999), origin+vector2(0,9999), axisLineColors[vAxis]);
    ImGui::GetWindowDrawList().AddLine(origin+vector2(-9999, 0), origin+vector2(9999, 0), axisLineColors[hAxis]);
  }

  void end() {
    drawAxisLines();
    // draw dummies - force scroll region
    ImGui::SetCursorPos((nodesMin-scrollRegionPadding) - childScreenPos); ImGui::Dummy(vector2(1,1));
    ImGui::SetCursorPos((nodesMax+scrollRegionPadding) - childScreenPos); ImGui::Dummy(vector2(1,1));

    // controls go on the top of the child, scrolling is cancelled out.
    ImGui::SetCursorPos(vector2(ImGui::GetScrollX(), ImGui::GetScrollY()) + controlsBoxPadding);
    drawControls();
    ImGui::EndChild();
  }

  float fmax(float a, float b) { return a>b?a:b; }
  float fmin(float a, float b) { return a<b?a:b; }
}
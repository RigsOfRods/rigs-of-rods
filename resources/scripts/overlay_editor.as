/// \title OGRE Overlay editor
/// \brief PROTOTYPE; can manage loaded OGRE overlays and edit elements
///
/// Roadmap:
/// --------------
/// - [x] Diagnostic-level UI showing current state (active overlay/element etc...)
/// - [x] UI list of list existing overlays with Show/Hide button
/// - [x] UI button + textbox to create new overlay
/// - [x] Click and drag control to add new element
/// - [ ] Selected element highlight with mouse-drag boxes for position+scale
/// - [ ] Selected element properties box with material and other properties, editable or not
/// - [ ] UI list of elements, mouse hover highlights the element on screen 
/// - [ ] Mouse selection of elements, mouse hover on element highlights the item in UI list
/// - [ ] Export of OGRE-compatible .overlay script
//
// APIdoc hints (authoritative source is https://developer.rigsofrods.org):
// -----------------
// Ogre::OverlayManager (ref, nocount, use .getSingleton())
//   .getByName(string) --> Ogre::Overlay      ~ will calmly return NULL if overlay doesn't exist.
//   .create(string) --> Ogre::Overlay                ~ throws exception if the name already exists!
//   .hasOverlayElement(string) --> bool
//   .getOverlayElement(string) --> Ogre::OverlayElement      ~ will throw exception if not found
//   .createOverlayElement(string type, string name) --> Ogre::OverlayElement          ~ throws exception if the name already exists!
//   .getOverlays() --> array<Ogre::Overlay@>@
// Ogre::Overlay (ref, nocount)
//   .getName() --> string
//   .add2D(OverlayElement@) --> void
//   .show() --> void
//   .hide() --> void
// Ogre::OverlayElement (ref, nocount)
//   .show() --> void
//   .hide() --> void
//   .setPosition(float x, float y) --> void
//   .setDimensions(float w, float h) --> void
//   .setMetricsMode(Ogre::GuiMetricsMode) --> void
//   .getLeft/Top/Width/Height() --> float
//   .setLeft/Top/Width/Height(float) --> void
//   .setMaterialName(string name, string resourceGroup)
// ========================================

enum CursorMode { 
  OE_CURSORMODE_SELECT, 
  OE_CURSORMODE_ADDELEMENT,
  OE_CURSORMODE_DRAWINGELEMENT
}

const float OE_MOUSERECT_ROUNDING = 0.f;
const float OE_MOUSERECT_THICKNESS = 4.f;
const int OE_MOUSE_LMB = 0;
const string OE_RGN_AUTODETECT = "OgreAutodetect";

class OverlayEditor {

  CursorMode mCursorMode = OE_CURSORMODE_SELECT;
  array<string> mCursorModeNames = {'Select', 'AddElement', 'DrawingElement'};
  string mPendingElementType;
  Ogre::Overlay@ mSelectedOverlay;
  Ogre::OverlayElement@ mSelectedElement;
  vector2 mMouseDragStart = vector2(-1, -1);
  string mNewOverlayNameBuf;
  string mNewElementMaterialNameBuf = "tracks/wheelface";
  int mElementCounter = 10;
  string mErrorString;

  void updateMouse() {
    if (mCursorMode == OE_CURSORMODE_ADDELEMENT && ImGui::IsMouseClicked(OE_MOUSE_LMB)) {
      mCursorMode = OE_CURSORMODE_DRAWINGELEMENT;
      mMouseDragStart = game.getMouseScreenPosition();
    } else if (mCursorMode == OE_CURSORMODE_DRAWINGELEMENT) {
      if (!ImGui::IsMouseDown(OE_MOUSE_LMB)) {
        this.addPendingElement();
        this.resetCursorMode();
      }
    }
  }

  protected void addPendingElement() {
    if (@mSelectedOverlay == null) { mErrorString = "addPendingElement(): no active overlay!"; return; }
    vector2 mouseCurPos = game.getMouseScreenPosition();
    string elemName = mSelectedOverlay.getName()+"-Element"+(mElementCounter++)+"-"+mPendingElementType;
    @mSelectedElement = Ogre::OverlayManager::getSingleton().createOverlayElement(mPendingElementType, elemName);
    if (@mSelectedElement == null) { mErrorString = "addPendingElement(): failed to create!"; return; }
    mSelectedOverlay.add2D(mSelectedElement);
    mSelectedElement.setMetricsMode(Ogre::GMM_PIXELS);
    mSelectedElement.setLeft(mMouseDragStart.x);
    mSelectedElement.setTop(mMouseDragStart.y);
    mSelectedElement.setWidth(mouseCurPos.x - mMouseDragStart.x);
    mSelectedElement.setHeight(mouseCurPos.y - mMouseDragStart.y);
    mSelectedElement.setMaterialName(mNewElementMaterialNameBuf, OE_RGN_AUTODETECT);
    
  }

  protected void resetCursorMode() {
    mCursorMode = OE_CURSORMODE_SELECT;
    mPendingElementType = "";
    mMouseDragStart = vector2(-1, -1);
  }

  void drawUI()  {
    this.drawUIMouseKnobs();
    ImGui::Begin("Overlay editor PROTOTYPE", true, 0);
    this.drawUIStatusInfo();
    ImGui::Separator();
    this.drawUIToolbox();
    ImGui::Separator();
    this.drawUIOverlayList();
     if (@mSelectedOverlay!=null) { this.drawUIOverlayElementList(mSelectedOverlay); }
    ImGui::End();
  }

  void drawOverlayProperties(Ogre::Overlay@ ov) {
     if (@ov == null) { mErrorString="drawOverlayProperties(): overlay is null!"; }
     ImGui::Text("Name: "+ov.getName());
     ImGui::Text("ZOrder: "+ov.getZOrder());
     ImGui::Text("Visible: "+ov.isVisible());
     ImGui::Text("ScrollX: "+ov.getScrollX());
     ImGui::Text("ScrollY: "+ov.getScrollY());
     ImGui::Text("Rotate: "+ov.getRotate().valueRadians());
     ImGui::Text("ScaleX: "+ov.getScaleX());
     ImGui::Text("ScaleY: "+ov.getScaleY());     
  }

  protected void drawUIMouseKnobs() {
    if (mCursorMode == OE_CURSORMODE_DRAWINGELEMENT) {
      ImDrawList@ drawlist = this.getDummyFullscreenWindow("OverlayEditorKnobs");
      drawlist.AddRect(mMouseDragStart, game.getMouseScreenPosition(), color(0.7, 0.8, 0.3, 1), OE_MOUSERECT_ROUNDING, OE_MOUSERECT_THICKNESS);
    }
  }

  protected void drawUIToolbox() {
    ImGui::TextDisabled("Toolbox");
    if (@mSelectedOverlay == null) { ImGui::Text("No overlay selected!"); return; }
    if (ImGui::Button("Add Panel (draw a rectangle using mouse)")) {
      mCursorMode = OE_CURSORMODE_ADDELEMENT;
      mPendingElementType = "Panel";
    }
    ImGui::InputText("Material", mNewElementMaterialNameBuf);
  }

  protected void drawUICreateOverlay() {
    ImGui::TextDisabled("Create new overlay");
    ImGui::InputText("Name", mNewOverlayNameBuf);
    ImGui::SameLine();
    if (ImGui::Button("Create")) { @mSelectedOverlay = Ogre::OverlayManager::getSingleton().create(mNewOverlayNameBuf); }
  }

  protected void drawUIStatusInfo() {
    ImGui::PushID("statusBox");
    ImGui::TextDisabled("Status:");
    ImGui::Text("Active overlay: "+(@mSelectedOverlay != null ? mSelectedOverlay.getName() : "~none"));
    if (@mSelectedOverlay != null) { 
      ImGui::SameLine();
      ImGui::TextDisabled("(hover for info)");
      if (ImGui::IsItemHovered()) { 
         ImGui::BeginTooltip();
         this.drawOverlayProperties(mSelectedOverlay);
         ImGui::EndTooltip();
      }
      ImGui::SameLine();
      if (ImGui::SmallButton("Deselect")) { @mSelectedOverlay = null; }
    }
    ImGui::Text("Active element: "+(@mSelectedElement != null ? mSelectedElement.getName() : "~none"));
    ImGui::Text("Cursor mode: " + mCursorModeNames[mCursorMode]);
    if (mCursorMode != OE_CURSORMODE_SELECT) {
      ImGui::SameLine(); if (ImGui::SmallButton("Reset")) { this.resetCursorMode(); }
    }
    ImGui::Text("Mouse drag start: X="+mMouseDragStart.x+" Y="+mMouseDragStart.y);
    ImGui::Text("Pending element type: "+mPendingElementType);
    ImGui::Text("Error string: "+mErrorString);
    ImGui::PopID(); // "statusBox"
  }

  protected void drawUIOverlayListRow(Ogre::Overlay@ ov) {
    ImGui::Bullet();
    ImGui::SameLine(); ImGui::Text(ov.getName());
    ImGui::SameLine();
    // Show/hide btn
    if (ov.isVisible()) {
      if (ImGui::SmallButton("Hide")) { ov.hide(); }
    } else {
      if (ImGui::SmallButton("Show")) { ov.show(); }
    }
    // Select/deselect btn
    ImGui::SameLine();
    if (@mSelectedOverlay != @ov) {
      if (ImGui::SmallButton("Select")) { @mSelectedOverlay = @ov; }
    } else {
      if (ImGui::SmallButton("Deselect")) { @mSelectedOverlay = null; }
    }
  }

  protected void drawUIOverlayList() {
    array<Ogre::Overlay@> @ovList = Ogre::OverlayManager::getSingleton().getOverlays();
    if (ImGui::CollapsingHeader("Manage Overlays ("+ovList.length()+")")) {
      this.drawUICreateOverlay();
      ImGui::Separator();
      for (uint i=0; i<ovList.length(); i++) {
        this.drawUIOverlayListRow(ovList[i]);
      }
    }
  }

  protected void drawUIOverlayElementDetails(Ogre::OverlayElement@ elem) {
    ImGui::Text("Name: "+elem.getName());
    ImGui::Text("");
    ImGui::Text("Visible:"+elem.isVisible());
    ImGui::Text("(Position) Left: "+elem.getLeft()+", Top: "+elem.getTop());
    ImGui::Text("(Size) Width: "+elem.getWidth()+", Heigh: t"+elem.getHeight());
    ImGui::Text("Material: " + elem.getMaterialName());
  }

  protected void drawUIOverlayElementListRow(Ogre::OverlayElement@ elem) {

    ImGui::Bullet();
    ImGui::SameLine();
    ImGui::Text(elem.getName());
    ImGui::SameLine();
    ImGui::TextDisabled(elem.getMaterialName());
    
      ImGui::SameLine();
      ImGui::TextDisabled("(hover for info)");
      if (ImGui::IsItemHovered()) { 
         ImGui::BeginTooltip();
         this.drawUIOverlayElementDetails(elem);
         ImGui::EndTooltip();
      }
     
    ImGui::SameLine();
    // Show/hide btn
    if (elem.isVisible()) {
      if (ImGui::SmallButton("Hide")) { elem.hide(); }
    } else {
      if (ImGui::SmallButton("Show")) { elem.show(); }
    }
    // Select/deselect btn
    ImGui::SameLine();
    if (@mSelectedElement != @elem) {
      if (ImGui::SmallButton("Select")) { @mSelectedElement = @elem; }
    } else {
      if (ImGui::SmallButton("Deselect")) { @mSelectedElement = null; }
    }
  }

  protected void drawUIOverlayElementList(Ogre::Overlay@ ov) {
    if (@ov == null) { mErrorString = "drawUIOverlayElementList(): overlay is null!"; return; }
    array<Ogre::OverlayElement@> @oeList = ov.get2DElements();
    if (ImGui::CollapsingHeader("Manage Elements ("+oeList.length()+")")) {
      for (uint i=0; i<oeList.length(); i++) {
        this.drawUIOverlayElementListRow(oeList[i]);
      }
    }
  }

  ImDrawList@ getDummyFullscreenWindow(const string&in name)
  {
    // Dummy fullscreen window to draw to
    int window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar| ImGuiWindowFlags_NoInputs 
                     | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::SetNextWindowPos(vector2(0,0));
    ImGui::SetNextWindowSize(game.getDisplaySize());
    ImGui::PushStyleColor(/*ImGuiCol_WindowBg*/2, color(0.f,0.f,0.f,0.f)); // Fully transparent background!
    ImGui::Begin(name, /*open:*/true, window_flags);
    ImDrawList@ drawlist = ImGui::GetWindowDrawList();
    ImGui::End();
    ImGui::PopStyleColor(1); // WindowBg

    return drawlist;    
  }
}

// %%%%%%%%%%%%%%% TEST %%%%%%%%%%%%%%%%%%

OverlayEditor editor;

void main() {
  game.log("Overlay editor is running!");
}
void frameStep(float dt) {
  editor.updateMouse();
  editor.drawUI();
}

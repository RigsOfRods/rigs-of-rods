/// \title OGRE Overlay editor
/// \brief PROTOTYPE; can manage loaded OGRE overlays and edit elements
///
/// Roadmap:
/// --------------
/// - [x] Diagnostic-level UI showing current state (active overlay/element etc...)
/// - [x] UI list of list existing overlays with Show/Hide button
/// - [x] UI button + textbox to create new overlay
/// - [x] UI button to destroy selected overlay
/// - [x] UI list of list existing elements with Show/Hide button
/// - [x] Click and drag control to add new element
/// - [x] UI button to destroy selected element
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
//   .destroyOverlayElement(string name, bool isTemplate=false) --> void
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
    string elemName = this.composeUniqueElementName(mSelectedOverlay.getName(), mPendingElementType);
    @mSelectedElement = Ogre::OverlayManager::getSingleton().createOverlayElement(mPendingElementType, elemName);
    if (@mSelectedElement == null) { mErrorString = "addPendingElement(): failed to create!"; return; }
    mSelectedOverlay.add2D(mSelectedElement);
    mSelectedElement.setMetricsMode(Ogre::GMM_PIXELS);
    mSelectedElement.setLeft(mMouseDragStart.x);
    mSelectedElement.setTop(mMouseDragStart.y);
    mSelectedElement.setWidth(mouseCurPos.x - mMouseDragStart.x);
    mSelectedElement.setHeight(mouseCurPos.y - mMouseDragStart.y);
    mSelectedElement.setMaterialName(mNewElementMaterialNameBuf, OE_RGN_AUTODETECT);
    mSelectedElement.show();
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
    if (@ov == null) { mErrorString="drawOverlayProperties(): overlay is null!"; return; }
    ImGui::TextDisabled("Ogre::Overlay details");
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
    if (ImGui::Button("Create")) { 
      @mSelectedOverlay = Ogre::OverlayManager::getSingleton().create(mNewOverlayNameBuf);
      mSelectedOverlay.show();
    }
  }

  protected void drawUIStatusInfo() {
    ImGui::PushID("statusBox");
    ImGui::TextDisabled("Status:");
    // active overlay
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
    // active element
    ImGui::Text("Active element: "+(@mSelectedElement != null ? mSelectedElement.getName() : "~none"));
    if (@mSelectedElement != null) {
      ImGui::SameLine();
      if (mSelectedElement.isVisible()) {
        if (ImGui::SmallButton("Hide##hideActiveElem")) { mSelectedElement.hide(); }
      } else {
        if (ImGui::SmallButton("Show##showActiveElem")) { mSelectedElement.show(); }
      }
      ImGui::SameLine();
      if (ImGui::SmallButton("Deselect##deselectActiveElem")) { @mSelectedElement = null; }
    }
    // cursor mode
    ImGui::Text("Cursor mode: " + mCursorModeNames[mCursorMode]);
    if (mCursorMode != OE_CURSORMODE_SELECT) {
      ImGui::SameLine(); if (ImGui::SmallButton("Reset")) { this.resetCursorMode(); }
    }
    // other
    ImGui::Text("Mouse drag start: X="+mMouseDragStart.x+" Y="+mMouseDragStart.y);
    ImGui::Text("Pending element type: "+mPendingElementType);
    ImGui::Text("Error string: "+mErrorString);
    ImGui::PopID(); // "statusBox"
  }

  protected void drawUIOverlayListRow(Ogre::Overlay@ ov) {
    ImGui::PushID(ov.getName());
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
    ImGui::PopID(); // ov.getName()
  }

  protected void drawUIOverlayList() {
    array<Ogre::Overlay@> @ovList = Ogre::OverlayManager::getSingleton().getOverlays();
    if (ImGui::CollapsingHeader("Manage Overlays")) {
      this.drawUIDestroySelectedOverlay();
      this.drawUICreateOverlay();
      ImGui::Separator();
      ImGui::TextDisabled("Total items: "+ovList.length());
      for (uint i=0; i<ovList.length(); i++) {
        this.drawUIOverlayListRow(ovList[i]);
      }
    }
  }

  protected void drawUIDestroySelectedOverlay() {
    if (@mSelectedOverlay != null && ImGui::Button("Destroy selected overlay")) {
      Ogre::OverlayManager::getSingleton().destroy(mSelectedOverlay);
      @mSelectedOverlay = null;
      @mSelectedElement = null;
    }
  }

  protected void drawUIOverlayElementDetails(Ogre::OverlayElement@ elem) {
    if (@elem == null) { mErrorString = "drawUiOverlayElementDetails(): elem is null!"; return; }
    ImGui::TextDisabled("Ogre::OverlayElement details");
    ImGui::Text("Name: "+elem.getName());
    ImGui::Text("TypeName: ~TBD~");
    ImGui::Text("Visible: "+elem.isVisible());
    ImGui::Text("(Position) Left: "+elem.getLeft()+", Top: "+elem.getTop());
    ImGui::Text("(Size) Width: "+elem.getWidth()+", Height: "+elem.getHeight());
    ImGui::Text("Material: " + elem.getMaterialName());
  }

  protected void drawUIOverlayElementListRow(Ogre::OverlayElement@ elem) {
    ImGui::PushID(elem.getName());
    ImGui::Bullet();
    ImGui::SameLine();
    ImGui::Text(elem.getName());
    ImGui::SameLine();
    // hover text
    ImGui::SameLine();
    ImGui::TextDisabled("(hover for info)");
    if (ImGui::IsItemHovered()) { 
       ImGui::BeginTooltip();
       this.drawUIOverlayElementDetails(elem);
       ImGui::EndTooltip();
    }
    // Show/hide btn
    ImGui::SameLine();
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
    ImGui::PopID(); // elem.getName()
  }

  protected void drawUIOverlayElementList(Ogre::Overlay@ ov) {
    if (@ov == null) { mErrorString = "drawUIOverlayElementList(): overlay is null!"; return; }
    if (ImGui::CollapsingHeader("Manage Elements")) {
      drawUIDestroySelectedElement();
      array<Ogre::OverlayElement@> @elemList = ov.get2DElements();
      ImGui::TextDisabled("Total items: "+elemList.length());
      for (uint i=0; i<elemList.length(); i++) {
        this.drawUIOverlayElementListRow(elemList[i]);
      }
    }
  }

  protected void drawUIDestroySelectedElement() {
    if (@mSelectedElement != null) {
      if (ImGui::Button("Destroy selected element")) {
        Ogre::OverlayManager::getSingleton().destroyOverlayElement(mSelectedElement);
        @mSelectedElement = null;
      }
      ImGui::Separator();
    }
  }

  private string composeUniqueElementName(const string&in ovName, const string&in typeName) {
    string elemName;
    do {
      elemName = ovName+"-Elem"+(mElementCounter++)+"("+typeName+")";
    } while (Ogre::OverlayManager::getSingleton().hasOverlayElement(elemName));
    return elemName;
  }

  private ImDrawList@ getDummyFullscreenWindow(const string&in name)
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

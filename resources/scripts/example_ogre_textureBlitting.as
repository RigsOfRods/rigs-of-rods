// \title Experiment - a visual texture painting tool
// \brief Demo of `Ogre::HardwarePixelBuffer` bindings
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

#include "gridviewer_utils.as"

GridViewer gDstViewer;
GridViewer gSrcViewer;
Ogre::Image gSrcImg;
Ogre::TexturePtr gSrcTex; // only for display, not blitting ATM
Ogre::TexturePtr gDstTex;
Ogre::HardwarePixelBufferPtr pixbuf;
bool gDrawingSrcBox=false;
bool gDrawingDstBox=false;
vector2 gDrawingBoxScreenStart(0,0);
bool gWasMouseDown=false;
box gSrcBox = box(0,0,0,0); // (left top right bottom)
box gDstBox = box(0,0,0,0); // (left top right bottom)
color gColorImageOutline = color(0.7,0.4,0.3,1);
float gBottomBarHeight = 50;
color gColorViewerImageTint(1,1,1,1); // white
bool gIsMouseDown = false;

float fmax(float a, float b) { return a>b?a:b; }
float fmin(float a, float b) { return a<b?a:b; }

string makeID(string name)
{
    // to avoid clash with leftover scene nodes created before, we include the NID in the name - using automatic global var `thisScript`.
    return "example-OGRE-ManuaObject.as(NID:"+thisScript+"): "+name;
}



void setupTexBlitTool()
{
    gSrcViewer.childWindowID="Source image";
    gSrcViewer.hAxis = 0; // X
    gSrcViewer.vAxis = 1; // Y
    
    gDstViewer.childWindowID="Dest. texture";
    gDstViewer.hAxis = 0; // X
    gDstViewer.vAxis = 1; // Y
    
    
    // prepare SOURCE image
    gSrcImg = game.loadImageResource("sign-roadnarrows.dds", "TexturesRG");
    game.log("DBG img loaded: w="+gSrcImg.getWidth()+" h="+gSrcImg.getHeight()+ " numFaces="+gSrcImg.getNumFaces()+" numMipmaps="+gSrcImg.getNumMipmaps());
    gSrcTex = Ogre::TextureManager::getSingleton().load("sign-roadnarrows.dds", "TexturesRG"); // not for blitting, only for display ATM   
    
    // prepare DESTINATION image
    // the autostrasse sign tex - we receive the one already loaded.
    gDstTex = Ogre::TextureManager::getSingleton().load("character.dds", "TexturesRG");
    pixbuf = gDstTex.getBuffer(/*cubemap face index:*/0, /*mipmap:*/0);  
}

void drawTexGridViewer(GridViewer@ aThisViewer, vector2 aViewerSize, Ogre::TexturePtr&in imgTex, bool&inout aDrawingBox, box&inout aThisBox)
{
    // Common for DST and SRC viewers
    // ------------------------------
    
    aThisViewer.begin(aViewerSize);
    
    // draw the image
    vector2 imgImgMin = aThisViewer.localToScreenPos(vector3(0,0,0));
    vector2 imgImgMax = aThisViewer.localToScreenPos(vector3(imgTex.getWidth(),imgTex.getHeight(),1));
    ImGui::GetWindowDrawList().AddImage(imgTex, imgImgMin, imgImgMax, vector2(0,0), vector2(1,1), gColorViewerImageTint);
    
    // draw debug rect where image should be; thick orange
    ImGui::GetWindowDrawList().AddRect(imgImgMin, imgImgMax, gColorImageOutline, 0.f, 0, 4.f);
    
    vector2 mouseposV2 = ImGui::GetMousePos();
    //vector3 mouseposV3= vector3(mouseposV2.x, mouseposV2.y, 0);
    
    
    if (aThisViewer.isChildWindowHovered && !aDrawingBox && !gWasMouseDown && gIsMouseDown)
    {
        aDrawingBox=true;
        gDrawingBoxScreenStart = mouseposV2;
    }
    
    if (aDrawingBox)
    {
        vector3 boxStart = aThisViewer.screenToLocalPos(gDrawingBoxScreenStart);
        vector3 boxEnd = aThisViewer.screenToLocalPos(mouseposV2);
        aThisBox.left = int(fmin(boxStart.x, boxEnd.x));
        aThisBox.top = int(fmin(boxStart.y, boxEnd.y));
        aThisBox.right = int( fmax(boxStart.x, boxEnd.x));
        aThisBox.bottom = int(fmax(boxStart.y, boxEnd.y));
        
        if(!aThisViewer.isChildWindowHovered || !gIsMouseDown)
        {
            aDrawingBox=false;
            gDrawingBoxScreenStart = vector2(0,0);
        }
    }  
    
    // for contrast, draw thick black and then thin yellow
    vector2 boxTL = aThisViewer.localToScreenPos(vector3(aThisBox.left, aThisBox.top, 0));
    vector2 boxBR = aThisViewer.localToScreenPos(vector3(aThisBox.right, aThisBox.bottom, 0));
    ImGui::GetWindowDrawList().AddRect(boxTL, boxBR, color(0,0,0,1), 0.f, 0, 5.f);
    ImGui::GetWindowDrawList().AddRect(boxTL, boxBR, color(0.9, 1, 0.1, 1), 0.f, 0, 1.f);
    
    aThisViewer.end();
}

void drawTexBlitGridViews()
{
    vector2 windowSize = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin();    
    float spaceUnderViewer = 99;
    vector2 panelSize = vector2(windowSize.x / 2, windowSize.y - (gBottomBarHeight));
    vector2 viewerSize = vector2(windowSize.x / 2, windowSize.y - (gBottomBarHeight+spaceUnderViewer));
    
    if (ImGui::BeginChild("leftPane",panelSize))
    {    
        drawTexGridViewer(gDstViewer, viewerSize, gDstTex, gDrawingDstBox, gDstBox);
        
        ImGui::Text("Mouse hover="+gDstViewer.isChildWindowHovered+" Drawing box="+gDrawingDstBox);
        ImGui::Text("Dst box: left="+gDstBox.left+" top="+gDstBox.top+" right="+gDstBox.right+" bottom="+gDstBox.bottom);      
        ImGui::Text("DBG gDrawingBoxScreenStart: X="+gDrawingBoxScreenStart.x+" Y="+gDrawingBoxScreenStart.y);
    }
    ImGui::EndChild(); // "leftPane"
    
    ImGui::SameLine();
    
    if (ImGui::BeginChild("rightPane", panelSize))
    {
        drawTexGridViewer(gSrcViewer, viewerSize, gSrcTex, gDrawingSrcBox, gSrcBox);
        
        ImGui::Text("Mouse hover="+gSrcViewer.isChildWindowHovered+"  Drawing box="+gDrawingSrcBox);
        ImGui::Text("Src box: left="+gSrcBox.left+" top="+gSrcBox.top+" right="+gSrcBox.right+" bottom="+gSrcBox.bottom); 
    }
    ImGui::EndChild(); // "rightPane"
    
    gWasMouseDown = ImGui::IsMouseDown(0);
}

void drawBottomBar()
{
    ImGui::TextDisabled("Right-click and drag mouse to define source/dest. boxes.");
    ImGui::SameLine();
    if (ImGui::Button("blit!"))
    {
        Ogre::PixelBox srcPixbox = gSrcImg.getPixelBox(0,0); // getPixelBox(cubemapFaceIndex, mipmapIndex)
        game.log("DBG srcPixbox: width="+srcPixbox.getWidth()+" height="+srcPixbox.getHeight()+" depth="+srcPixbox.getDepth());
        
        // let the MAGIC happen
        
        game.log("DBG blitFromMemory()");
        pixbuf.blitFromMemory(srcPixbox, gDstBox);
        
    }
}

// ================= entry points ===================

void main()
{
    game.log("Experimental texture paiting UI");
    setupTexBlitTool();
}

void frameStep(float dt)
{
    gIsMouseDown = ImGui::IsMouseDown(1);
    
    if (ImGui::Begin("Example", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
        
        drawTexBlitGridViews();
        ImGui::Separator();
        drawBottomBar();   
        
        ImGui::End();
    }    
    
    gWasMouseDown = gIsMouseDown;
}

// \title Experiment - a visual vertex/texcoord display tool
// \brief Demo of `__getVertexPositions()` & `__getVertexTexcoords()` functions.
// ===================================================

#include "gridviewer_utils.as"

GridViewer gTexcoordsViewer;
GridViewer gVertsViewer;
Ogre::MeshPtr gMesh;
Ogre::TexturePtr gTex;
array<vector3>@ gVertPositions = null;
array<vector2>@ gVertTexcoords = null;
int gVertViewAlongAxis = 2; // X=0, Y=1, Z=2
float gBottomBarHeight = 50;
color gColorViewerImageTint(1,1,1,1); // white
color gVertColor(0.1f, 0.9f, 0.3f, 1);
color gTexcoordColor(0.8,1,0,1);
float gVertRadius = 1.8;
float gTexcoordRadius = 1.6;
int gVertNumSeg = 5;
int gTexcoordNumSeg = 5;
string gErrorStr = "";
bool gShowRawTexcoords=false;

float fmax(float a, float b) { return a>b?a:b; }
float fmin(float a, float b) { return a<b?a:b; }



void setupTool()
{
    gVertsViewer.childWindowID="Verts";
    gVertsViewer.hAxis = 0; // X
    gVertsViewer.vAxis = 1; // Y
    
    gTexcoordsViewer.childWindowID="Texcoords";
    gTexcoordsViewer.hAxis = 0; // X
    gTexcoordsViewer.vAxis = 1; // Y
    
    gMesh = Ogre::MeshManager::getSingleton().load("character.mesh", "MeshesRG");
    if (gMesh.isNull())
    {
        gErrorStr = "couldn't load mesh";
        return;
    }
    gTex = Ogre::TextureManager::getSingleton().load("character.dds", "TexturesRG");
    if (gTex.isNull())
    {
        gErrorStr = "couldn't load tex";
        return;
    }
    
    game.log("Num. submeshes: " + gMesh.getSubMeshes().length());
    if (gMesh.getSubMeshes().length() == 0)
    {
        gErrorStr = "no submeshes";
        return;
    }
    
    Ogre::SubMesh@ submesh = gMesh.getSubMeshes()[0];
    if (@submesh == null)
    {
        gErrorStr = "submesh is null";
        return;
    }
    
    @gVertPositions = submesh.__getVertexPositions();
    if (@gVertPositions == null)
    {
        gErrorStr = "vert positions array is null";
        return;
    }
    
    @gVertTexcoords = submesh.__getVertexTexcoords(0);
    if (@gVertTexcoords == null)
    {
        gErrorStr = "vert texcoords array is null";
        return;
    }    
}

void drawGridViews()
{
    vector2 windowSize = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin();    
    float spaceUnderViewer = 50;
    vector2 panelSize = vector2(windowSize.x / 2, windowSize.y - (gBottomBarHeight));
    vector2 viewerSize = vector2(windowSize.x / 2, windowSize.y - (gBottomBarHeight+spaceUnderViewer));
    
    if (ImGui::BeginChild("leftPane",panelSize))
    {    
        gVertsViewer.begin(viewerSize);
        
        // Draw the vert positions
        for (uint i=0; i < gVertPositions.length(); i++)
        {
            vector2 screenPos = gVertsViewer.localToScreenPos(gVertPositions[i]);
            ImGui::GetWindowDrawList().AddCircleFilled(screenPos, gVertRadius, gVertColor, gVertNumSeg);   
        }
        
        gVertsViewer.end();
        
        if (@gVertPositions != null)
        {
            ImGui::TextColored( gVertColor, "Num verts: "+gVertPositions.length());
        }   
    }
    ImGui::EndChild(); // "leftPane"
    
    ImGui::SameLine();
    
    if (ImGui::BeginChild("rightPane", panelSize))
    {
        if (gShowRawTexcoords)
        {
            if (ImGui::BeginChild("rawTexcoordsPane", viewerSize))
            {
                for (uint i=0; i < gVertTexcoords.length(); i++)
                {
                    ImGui::Text("["+i+"]    X(U)="+formatFloat(gVertTexcoords[i].x, "",5, 3)+"    Y(V)="+formatFloat(gVertTexcoords[i].y, "", 5, 3));
                }
            }
            ImGui::EndChild();  //"rawTexcoordsPane"
        }
        
        else
        {
            gTexcoordsViewer.begin(viewerSize);
            
            // draw the image
            vector2 screenMin = gTexcoordsViewer.localToScreenPos(vector3(0,0,0));
            vector2 screenMax = gTexcoordsViewer.localToScreenPos(vector3(gTex.getWidth(),gTex.getHeight(),1));
            ImGui::GetWindowDrawList().AddImage(gTex, screenMin, screenMax, vector2(0,0), vector2(1,1), gColorViewerImageTint);  
            
            // Draw the texcoords
            for (uint i=0; i < gVertTexcoords.length(); i++)
            {
                vector2 screenPos = gTexcoordsViewer.localToScreenPos(vector3(gVertTexcoords[i].x*gTex.getWidth(), gVertTexcoords[i].y*gTex.getHeight(), 0));
                ImGui::GetWindowDrawList().AddCircleFilled(screenPos, gTexcoordRadius, gTexcoordColor, gTexcoordNumSeg);            
            }
            
            gTexcoordsViewer.end();
        }
        
        if (@gVertTexcoords != null)
        {
            ImGui::TextColored(gTexcoordColor, "Num Texcoords: "+gVertTexcoords.length());
            ImGui::Checkbox("Show raw texcoords", gShowRawTexcoords);
        }        
    }
    ImGui::EndChild(); // "rightPane"
}

void drawBottomBar()
{
    ImGui::TextDisabled("Select viewing axis for the verts");   ImGui::SameLine();
    ImGui::RadioButton("X", gVertViewAlongAxis, 0);             ImGui::SameLine();
    ImGui::RadioButton("Y", gVertViewAlongAxis, 1);             ImGui::SameLine();
    ImGui::RadioButton("Z", gVertViewAlongAxis, 2); 
    
    switch (gVertViewAlongAxis)
    {
        case 0: gVertsViewer.hAxis=2; gVertsViewer.vAxis=1; break; // view along X: horizontal=Z, vertical=Y
        case 1: gVertsViewer.hAxis=0; gVertsViewer.vAxis=1; break; // view along Y: horizontal=Z, vertical=X
        case 2: gVertsViewer.hAxis=0; gVertsViewer.vAxis=1; break; // view along Z: horizontal=X, vertical=Y
        default: ImGui::Text("ERROR: INVALID AXIS!!"); break;
    }
    
    
}

// ================= entry points ===================

void main()
{
    game.log("Experimental vertex position/texcoord display UI");
    setupTool();
}

void frameStep(float dt)
{
    if (gErrorStr == "")
    {
        drawGridViews();
    }
    else
    {
        ImGui::TextDisabled("E R R O R !!");
        ImGui::TextColored(color(1, 0.2, 0, 1), gErrorStr);
    }
    ImGui::Separator();
    drawBottomBar();   
}

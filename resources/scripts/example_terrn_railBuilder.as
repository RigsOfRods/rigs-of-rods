// RAIL BUILDER EXPERIMENT
// This script contains a datasheet of ODEF railroad pieces with info how to snap them.
// 
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

// #region Datasheet of available pieces, Key = odef name, Value = ObjPieceInfo
class ObjPieceSpec
{
    array<vector3> snapPoints;
    float heightBias;
ObjPieceSpec( float _hBias,  array<vector3> _snapPoints) { heightBias=_hBias; snapPoints=_snapPoints;}
};
dictionary objPieces = {
{'rail1t1mstrt',   ObjPieceSpec(0.4f, array<vector3> = {vector3(1,1,1), vector3(-1,-1,-1)})  },
{'rail1t5mstrt',   ObjPieceSpec(0.4f, array<vector3> = {vector3(1,1,1), vector3(-1,-1,-1)})  }
};

// #endregion

// #region Spawned object instances
class ObjInstanceInfo
{ 
    string name; // Unique ID for this instance
    string pieceName;
    vector3 pos; 
    vector3 rot;
ObjInstanceInfo(string _name, vector3 _pos, vector3 _rot) {name=_name; pos=_pos; rot=_rot;}
};
dictionary objInstances; // instanceName ->ObjInstanceInfo
uint spawnedObjSequentialNum = 0;
// #endregion


string nextPiece = 'rail1t1mstrt';
string selectedPieceInst = ''; // instance name of selected piece; '' means 'use rorbot's position'
int selectedPieceSnapPoint = -1; // snap point index; -1 means none selected, use rorbot's position

void addPieceBySelection()
{
    // spawn a piece selected on UI; determine placement and record info for drawing and editing
    // --------------------------------------------------------
    ObjPieceSpec@ nextPieceSpec = cast<ObjPieceSpec>(objPieces[nextPiece]);
if (@nextPieceSpec == null) { game.log("ERROR addPieceBySelection(): nextPieceSpec null!"); return; }
    
    vector3 instPos, instRot;
    if (selectedPieceInst == '')
    {
        instPos = game.getPersonPosition() ;
        instRot = vector3(0, -game.getPersonRotation().valueDegrees(), 0);
    }
    else
    {
        ObjInstanceInfo@ selPieceInfo = cast<ObjInstanceInfo>(objInstances[selectedPieceInst]);
    if (@selPieceInfo == null) { game.log("ERROR addPieceBySelection(): selPieceInfo null!"); return; }
        instPos = selPieceInfo.pos;
        instRot = selPieceInfo.rot;
    }    
    instPos.y += nextPieceSpec.heightBias;
    string instName = nextPiece+"_S"+(spawnedObjSequentialNum++)+"N"+thisScript;
    game.spawnObject(nextPiece, instName, instPos, instRot, "", false);
    ObjInstanceInfo@ instInfo = ObjInstanceInfo (instName, instPos, instRot);
    instInfo.pieceName = nextPiece;
    game.log("DBG addPieceBySelection(): setting instance info for key '"+instName+"' (is null: "+(instInfo is null)+")");
    @objInstances[instName] = @instInfo;
    // paranoid dbg test
    
    ObjInstanceInfo@ instInfo2 =cast<ObjInstanceInfo>(@objInstances[instName]);
    if (@instInfo2 == null)
    {
        game.log("ERR addPieceBySelection(): inst info null test failed");
    }
    // paranoid n.2
    ObjInstanceInfo@ instInfo3 = null;
    bool instInfo3valid=objInstances.get(instName, @instInfo3);
    game.log("DBG addPieceBySelection(): instInfo3valid="+instInfo3valid+" (isnull:"+(instInfo3 is null)+")");
    // END paranoid
    selectedPieceInst = instName;
}

void drawInstanceHandles(ObjInstanceInfo@ instInfo)
{
    // draw on-screen handles for origin-point and snap-points of a single rail piece
    // ---------------------------------------
    ImDrawList@ drawlist = imgui_utils::ImGetDummyFullscreenWindow("rail builder handles");
    
    vector2 originScreenPos;
    if (game.getScreenPosFromWorldPos(instInfo.pos, /*out:*/originScreenPos))
    {
        drawlist.AddCircleFilled(originScreenPos, 4, color(1,0,0,1));
    }
    
    // Then look up piece spec and draw snap-points
    ObjPieceSpec@ pieceSpec = cast<ObjPieceSpec>(objPieces[instInfo.pieceName]);
    if (@pieceSpec == null)
    {
        ImGui::Text("DBG piece spec null!");
    }
    else
    {
        ImGui::Text("DBG num snappoints:"+pieceSpec.snapPoints.length());
        for (uint i=0; i<pieceSpec.snapPoints.length(); i++)
        {
            vector2 snapScreenPos;
            if (game.getScreenPosFromWorldPos(instInfo.pos+pieceSpec.snapPoints[i], /*out:*/snapScreenPos))
            {
                drawlist.AddCircleFilled(snapScreenPos, 4, color(1,1,0,1));
            } else {
                ImGui::Text("DBG snap off screen");
            }
        }
    }
    
}

void drawInstancesAll()
{
    ImGui::TextDisabled("Num instances: "+ objInstances.getKeys().length());
    ImGui::TextDisabled("Note: The _S#N# suffix is there to make names unique (sequece number, script unit ID)");
    array<string>@ keys = objInstances.getKeys();
    for (uint iKey = 0; iKey < keys.length(); iKey++)
    {
        ImGui::PushID(iKey);
        
        ObjInstanceInfo@ instInfo = cast<ObjInstanceInfo>(objInstances[keys[iKey]]);
        ImGui::Bullet(); ImGui::SameLine(); ImGui::TextDisabled(""+iKey+'/'+keys.length()+": "); ImGui::SameLine();
        if (@instInfo == null)
        {
            ImGui::Text("ERR  instance info null ");
        }
        else
        {
            ImGui::Text('"'+instInfo.name+'" ~ position X="'+instInfo.pos.x+" Y(up)="+instInfo.pos.y+" Z="+instInfo.pos.z);
            drawInstanceHandles(instInfo);
        }
        
        ImGui::PopID(); //iKey
    }
}

void frameStep(float dt)
{
    //drawInstancesAll();
    if (ImGui::Begin("~ ~ ~   R A I L   B U I L D E R ~ ~ ~", closeBtnHandler.windowOpen, 0))
    {
        // Draw the "Terminate this script?" prompt on the top (if not disabled by config).
        closeBtnHandler.draw();
        
        drawInstancesAll();
        ImGui::Text("Next piece: '" + nextPiece + "'");
        ImGui::Text("Selected piece: '" + selectedPieceInst + "' (Snap# "+selectedPieceSnapPoint+")");
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset##selection"))
        {
            selectedPieceInst = '';
            selectedPieceSnapPoint = -1;
        }
        if (ImGui::Button('Add piece!'))
        {
            addPieceBySelection();
        }
        
        ImGui::End();
    }
}

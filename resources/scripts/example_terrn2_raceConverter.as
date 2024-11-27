/// \title terrn2 race converter
/// \brief imports races from scripts and generates race-def files.
/// uses new section in terrn2 format: [Races]
/// ^ each line is a tobj-like file with any extension (i.e. *.race) which is loaded by the race system.
///
/// The program flow of this script got a little crazy;
/// ^ I wanted a fluidly updating UI, performing just one step (1 doc conversion / 1 file write) per frame.
/// ==================================================

// The race system
#include "races.as"
racesManager races;

// document viewer+editor
#include "genericdoc_utils.as"
genericdoc_utils::GenericDocEditor gdEditor;

// Window [X] button handler
#include "imgui_utils.as" 
imgui_utils::CloseWindowPrompt closeBtnHandlerUnique;

enum Stage // in order of processing 
{
    STAGE_INIT, // detects races
    STAGE_CONVERT, // converts all races to GenericDocument race-defs
    STAGE_FIXTERRN2, // modify .terrn2 file - add [Races], remove [Scripts]
    STAGE_IDLE, // Waiting for button press
    STAGE_PUSHMSG, // request game to create project
    STAGE_GETPROJECT, // fetch created project from modcache
    STAGE_WRITERACES,
    STAGE_WRITETERRN2, 
    STAGE_DONE,
    STAGE_ERROR
}
Stage stage = STAGE_INIT;
string error = "";
string projectName = "";
CacheEntryClass@ projectEntry;
array<GenericDocumentClass@> convertedRaces;
array<string> convertedRaceFileNames;

string fileBeingWritten = ""; 
GenericDocumentClass@ convertedTerrn2;
int topWrittenRace = -1;

//#region Game Callbacks

void main()
{
    // one-time config
    gdEditor.gdeCloseBtnHandler.cfgCloseImmediatelly=true; 
}

void frameStep(float dt)
{
    // === DRAW UI ===
    if (    ImGui::Begin("Race import", closeBtnHandlerUnique.windowOpen, /*flags:*/0))
    {
        // Draw the "Terminate this script?" prompt on the top (if not disabled by config).
        closeBtnHandlerUnique.draw();
        drawUI();
        ImGui::End();
    }
    
    
    // === PERFORM IMPORT STEP ===
    advanceImportOneStep();
}

//#endregion

//#region UI drawing
void drawUI()
{
    // Draw document window
    if (@gdEditor.displayedDocument != null)
    {
        gdEditor.drawSeparateWindow();
    }
    
    drawDetectedRaces();
    if (@convertedTerrn2 != null)
    {
        if (ImGui::SmallButton("Preview terrn2"))
        {
            gdEditor.setDocument(convertedTerrn2, projectName+'.terrn2');
        }
    }
    ImGui::Separator();
    switch(stage)
    {
        case STAGE_IDLE:
        {
            if (@game.getTerrain() != null   && stage == STAGE_IDLE )
            {
                if (  ImGui::Button("Convert races from script to terrn2 [Races]"))
                {
                    stage = STAGE_PUSHMSG;
                }   
            }
            break;       
        }     
        case STAGE_ERROR:
        {
            ImGui::Text("ERROR! "+error);
            break;
        }
        case STAGE_PUSHMSG:
        {
            ImGui::Text("Performing MSG_EDI_CREATE_PROJECT_REQUESTED"); 
            break;
        }
        case STAGE_WRITERACES:
        {
            ImGui::TextDisabled("Writing .race files:");
            ImGui::Text(fileBeingWritten);
            break;
        }
        case STAGE_WRITETERRN2:
        {
            ImGui::TextDisabled("OverWriting terrn2 file");
            ImGui::Text(projectName + ".terrn2");
            break;
        }
        case STAGE_DONE:
        {
            ImGui::Text('DONE');
            break;
        }
        case STAGE_GETPROJECT:
        {
            ImGui::Text('WARNING - stuck in stage GETPROJECT (name: "'+projectName+'")');
            break;
        }
        default:
        {
            break;
        }
    }
    
    
}

void drawDetectedRaces()
{
    ImGui::PushID("drawDetectedRaces");
    if (@races == null) 
    {
        ImGui::Text("ERR: races null");
        return;
    }
    
    if (@races.raceList == null) 
    {
        ImGui::Text("ERR raceList null");
        return;
    }
    
    ImGui::Text("Found " + races.raceList.length() + " races");
    for (uint i=0; i < races.raceList.length(); i++)
    {
        ImGui::PushID(i);
        raceBuilder@ race = races.raceList[i];
        if (@race == null)
        {
            ImGui::Text("ERR racebuilder null!");
            continue;
        }
        ImGui::Bullet();
        ImGui::Text(race.raceName);
        ImGui::SameLine();
        ImGui::TextDisabled(race.checkPointsCount + " checkpoints");
        if (@convertedRaces[i] != null)
        {
            ImGui::SameLine();
            if (  ImGui::SmallButton("Preview"))
            {
                gdEditor.setDocument(convertedRaces[i], races.raceList[i].raceName);
                
            }
        }
        ImGui::PopID(); // i
    }
    ImGui::PopID(); //"drawDetectedRaces"
}

//#endregion

//#region STAGE_INIT
void initializeRacesData()
{
    // find the terrain script
    array<int> nids = game.getRunningScripts();
    int terrnScriptNid = -1;
    for (uint i = 0; i < nids.length(); i++)
    {
        int nid = nids[i];
        dictionary@ info = game.getScriptDetails(nid);
        int category = int(info['scriptCategory']);
        if (category == SCRIPT_CATEGORY_TERRAIN)
        {
            terrnScriptNid = nid;
        }
    }
    
    if (terrnScriptNid == -1)
    {
        stage = STAGE_ERROR;
        error = "Terrain script not found!";
        return;
    }
    
    // moment of truth - retrieve the races using new API `game.getScriptVariable()`
    int result = game.getScriptVariable(terrnScriptNid, 'races', races);
    if (result < 0)
    {
        stage = STAGE_ERROR;
        error = " game.getScriptVariable() returned "+result;
        return;
    }
    
    if (@races == null)
    {
        stage = STAGE_ERROR;
        error = " game.getScriptVariable() reported 'OK' but did not fetch the data";
        return;  
    }
    
    for (uint i=0; i < races.raceList.length(); i++)
    {
        raceBuilder@ race = races.raceList[i];
        convertedRaces.insertLast(null);
        convertedRaceFileNames.insertLast(""); 
    }
}
//#endregion

//#region STAGE_CONVERT
bool convertNextRace()
{
    
    // seek first unconverted race; convert it; break loop
    for (uint i=0; i < races.raceList.length(); i++)
    {
        if (@convertedRaces[i] == null)
        {
            @convertedRaces[i] = convertSingleRace(races.raceList[i]);
            convertedRaceFileNames[i] = generateRaceFileName(races.raceList[i].raceName);
            return true;
        }
    }
    
    return false;            
}

// HELPERS:

const string BADCHARS="\\/:%* ";
const string GOODCHAR="_";
string generateRaceFileName(string raceName)
{
    string filename =  raceName + ".race";
    for (uint i=0; i<filename.length(); i++)
    {
        for (uint j=0; j < BADCHARS.length(); j++)
        {
            if (filename[i] == BADCHARS[j])
            {
                filename[i] = GOODCHAR[0];
            }
        }
    }
    //    game.log("DBG generateRaceFileName("+raceName+") --> " + filename);
    return filename;
}

void appendKeyValuePair( GenericDocContextClass@ ctx, string key, string value)
{
    ctx.appendTokKeyword( key);
    ctx.appendTokString(value);
    ctx.appendTokLineBreak();
}

void appendKeyValuePair( GenericDocContextClass@ ctx, string key, int value)
{
    ctx.appendTokKeyword( key);
    ctx.appendTokInt(value);
    ctx.appendTokLineBreak();
}

GenericDocumentClass@ convertSingleRace(raceBuilder@ race)
{
    GenericDocumentClass doc;
    GenericDocContextClass ctx(doc);
    
    ctx.appendTokComment( " ~~ New 'race-def' format (file extension: .race). ~~");ctx.appendTokLineBreak();
    ctx.appendTokComment( " Each race file specifies a single race");ctx.appendTokLineBreak();
    ctx.appendTokComment( " In .terrn2 file, list the race files under new section [Races]");ctx.appendTokLineBreak();
    ctx.appendTokComment( " Filenames must include extension and end with = (like scripts do)");ctx.appendTokLineBreak();
    ctx.appendTokComment( " Race system supports alternating paths!");ctx.appendTokLineBreak();
    ctx.appendTokComment( " Checkpoint format: checkpointNum(1+), altpathNum(1+), x, y, z, rotX, rotY, rotZ, objName(override, optional)");ctx.appendTokLineBreak();
    ctx.appendTokComment( " By convention, the checkpoint meshes are oriented sideways (facing X axis)");ctx.appendTokLineBreak();
    ctx.appendTokLineBreak();
    
    appendKeyValuePair(ctx, "race_name", race.raceName);
    appendKeyValuePair(ctx, "race_laps", race.laps);
    appendKeyValuePair(ctx, "race_checkpoint_object", race.exporterCheckpointObjName);
    appendKeyValuePair(ctx, "race_start_object", race.exporterStartObjName);
    appendKeyValuePair(ctx, "race_finish_object", race.exporterFinishObjName);
    
    ctx.appendTokLineBreak();
    
    ctx.appendTokKeyword( "begin_checkpoints");
    
    ctx.appendTokLineBreak();
    
    for (uint i=0; i < race.checkpoints.length(); i++)
    {
        uint numAltPaths = race.getRealInstanceCount(int(i));
        for (uint j = 0; j < numAltPaths; j++)
        {
            ctx.appendTokInt((i+1)); // checkpointNum (1+)
            ctx.appendTokInt((j+1)); // altpathNum (1+)
            const double[] args = race.checkpoints[i][j];
            ctx.appendTokFloat(args[0]); // pos X
            ctx.appendTokFloat(args[1]); // pos Y
            ctx.appendTokFloat(args[2]); // pos Z
            ctx.appendTokFloat(args[3]); // rot X
            ctx.appendTokFloat(args[4]); // rot Y
            ctx.appendTokFloat(args[5]); // rot Z
            string defaultObjName = (i==0) 
            ? race.exporterStartObjName 
            : (i==race.checkpoints.length()-1) ? race.exporterFinishObjName : race.exporterCheckpointObjName;
            string actualObjName = race.objNames[i][j];
            if (actualObjName != defaultObjName)
            {
                ctx.appendTokString(actualObjName);
            }
            ctx.appendTokLineBreak();
        }       
    }
    
    ctx.appendTokKeyword( "end_checkpoints");
    ctx.appendTokLineBreak();
    
    return doc;
}

//#endregion

//#region STAGE_FIXTERRN2
string BRACE="[";
void fixupTerrn2Document()
{
    TerrainClass@ terrain = game.getTerrain();
    if (@terrain == null)
    {
        game.log("fixupTerrn2Document(): no terrain loaded, nothing to do!");
        return;
    }
    @convertedTerrn2 =     GenericDocumentClass();
    
    if (!convertedTerrn2.loadFromResource(terrain.getTerrainFileName(), terrain.getTerrainFileResourceGroup(), genericdoc_utils::FLAGSET_TERRN2))
    {
        game.log("fixupTerrn2Document(): could not load terrn2 document, nothing to do!");
        return;
    }
    
    GenericDocContextClass ctx(convertedTerrn2);
    
    // Delete section [Scripts] and all it contains
    bool inSectionScripts = false;
    while (!ctx.endOfFile())
    {
        // entering section [Scripts]
        if (!inSectionScripts && ctx.isTokKeyword() && ctx.getTokKeyword()=="[Scripts]")
        {
            inSectionScripts=true;
        }
        // leaving section [Scripts]
        else if (inSectionScripts && ctx.isTokKeyword() && ctx.getTokKeyword()[0]==BRACE[0])
        {
            
            inSectionScripts=false;
        }
        
        // erase all script info
        if (inSectionScripts)
        {
            ctx.eraseToken();
        }
        // ... and also fix up the terrain name while at it.
        else if (ctx.isTokKeyword() && ctx.getTokKeyword() == "Name")
        {
            ctx.setTokString(1, projectName);
            // make it a little less obvious that GenericDocument actually breaks the terrain name into pieces
            // ^ just hard erase everything between the new name and a linebreak
            ctx.moveNext();
            ctx.moveNext();
            while (!ctx.isTokLineBreak())
            {
                ctx.eraseToken();
            }
        }
        ctx.seekNextLine();
    }
    
    // Append section [Races]
    ctx.appendTokLineBreak();
    ctx.appendTokKeyword("[Races]");
    ctx.appendTokLineBreak();
    for (uint i=0; i < convertedRaceFileNames.length(); i++)
    {
        ctx.appendTokString(convertedRaceFileNames[i]);
        ctx.appendTokLineBreak();
    }
    
    
    
}
//#endregion

// nothing for STAGE_IDLE

//#region STAGE_PUSHMSG
void pushMsgRequestCreateProject()
{
    TerrainClass@ terrain = game.getTerrain();
    projectName = terrain.getTerrainName() + " [Races] ~"+thisScript;
    
    // Fetch terrain's modcache entry
    CacheEntryClass@ src_entry = modcache.findEntryByFilename(LOADER_TYPE_TERRAIN, /*partial:*/false, terrain.getTerrainFileName());
    if (@src_entry == null)
    {
        error = "Not found in modcache!!";
        stage = STAGE_ERROR;
    }
    
    // request project to be created from that cache entry
    game.pushMessage(MSG_EDI_CREATE_PROJECT_REQUESTED, {
    {'name', projectName},
    {'source_entry', src_entry}
    });
}
//#endregion

//#region STAGE_GETPROJECT
void getProject()
{
    @projectEntry = modcache.findEntryByFilename(LOADER_TYPE_TERRAIN, /*partial:*/false, projectName+'.terrn2');
    if (@projectEntry != null)
    {
        stage = STAGE_WRITERACES;
    }
    else
    {
        stage=STAGE_ERROR;
        error="Could not load project entry";
    }
}
//#endregion

//#region STAGE_WRITERACES
void writeNextRace()
{
    for (uint i=0; i < races.raceList.length(); i++)
    {
        if (int(i) <= topWrittenRace)
        {
            continue; // already handled
        }               
        
        if (fileBeingWritten == "")
        {
            string filename =  convertedRaceFileNames[i];
            fileBeingWritten = filename;
            //                    game.log('DBG WRITERACES ['+i+']: fileBeingWritten='+fileBeingWritten);
            break;
        }
        else
        {
            // write the ' .race' file in separate frame so the user sees the correct filename on screen.
            if (@projectEntry == null)
            {
                stage=STAGE_ERROR;
                error="null project entry while generating files";
                break;
            }
            if (@convertedRaces[i] == null)
            {
                stage=STAGE_ERROR;
                error="null converted race at position ["+i+"]";
                break;
            }
            
            convertedRaces[i].saveToResource(fileBeingWritten, projectEntry.resource_group);
            topWrittenRace=int(i);
            
            //                    game.log('DBG GENFILES ['+i+']: saved file '+fileBeingWritten);
            fileBeingWritten = "";
            if (i == races.raceList.length()-1)
            {
                stage=STAGE_WRITETERRN2;
            }
            break;
        }             
    }
}
//#endregion

//#region STAGE_WRITETERRN2
void writeTerrn2()
{
    forceExportINI(convertedTerrn2);
    
    // delete original file (GenericDocument cannot overwrite)
    game.deleteResource(projectName+".terrn2", projectEntry.resource_group);
    
    // write out modified file
    convertedTerrn2.saveToResource(projectName+".terrn2", projectEntry.resource_group);
}

void forceExportINI(GenericDocumentClass@ doc)
{
    // hack to force GenericDocument export INI: pass over all non-braced keywords and append = to them.
    GenericDocContextClass ctx(doc); //reset context
    while (!ctx.endOfFile())
    {
        if (ctx.isTokKeyword() && ctx.getTokKeyword()[0]!=BRACE[0] )
        {
            ctx.setTokKeyword(0,ctx.getTokKeyword()+"=");
        }
        else if (ctx.isTokString() && ctx.isTokLineBreak()) 
        {
            // treat strings on line start as keywords (those are keywords that decayed to strings due to some chars)
            ctx.setTokKeyword(0,ctx.getTokString()+"=");
        }
        ctx.moveNext();
    }
}
//#endregion




void advanceImportOneStep()
{
    switch (stage)
    {
        case STAGE_INIT:
        {
            initializeRacesData();
            stage = STAGE_CONVERT;
            break;
        }
        case STAGE_PUSHMSG:
        {
            pushMsgRequestCreateProject();
            stage = STAGE_GETPROJECT;
            break;
        }      
        
        case STAGE_GETPROJECT:
        {
            getProject();
            break;
        }
        
        case STAGE_CONVERT:
        {
            if (!convertNextRace())
            {
                stage = STAGE_FIXTERRN2;
            }
            break;
        }
        
        case STAGE_WRITERACES:
        {
            
            writeNextRace();
            break;
        }
        
        case STAGE_FIXTERRN2:
        {
            fixupTerrn2Document();
            stage = STAGE_IDLE;
            break;
        }
        
        case STAGE_WRITETERRN2:
        {
            writeTerrn2();
            stage = STAGE_DONE;
            break;
        }
        
        default: 
        break;        
    }
}











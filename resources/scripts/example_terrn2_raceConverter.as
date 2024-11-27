/// \title terrn2 race converter
/// \brief imports races from scripts and generates race-def files.
/// new section in terrn2 format: [Races]
/// ^ each line is a tobj-like file with any extension (i.e. *.race) which is loaded by the race system.
///
/// The program flow of this script got a little crazy;
/// I wanted a fluidly updating UI, performing just one step (1 doc conversion / 1 file write) per frame.
/// ==================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

// The race system
#include "races.as"
racesManager races;

enum Stage // in order of processing
{
    STAGE_INIT, // detects races
    STAGE_CONVERT, // converts all races to GenericDocument race-defs
    STAGE_IDLE, // Waiting for button press
    STAGE_PUSHMSG, // request game to create project
    STAGE_GETPROJECT, // fetch created project from modcache
    STAGE_GENFILES, // write .race files
    STAGE_FIXTERRN2, // modify .terrn2 file - add [Races], remove [Scripts]
    STAGE_DONE,
    STAGE_ERROR
}
Stage stage = STAGE_INIT;
string error = "";
string projectName = "";
CacheEntryClass@ projectEntry;
array<GenericDocumentClass@> convertedRaces;
array<string> convertedAndSavedRaces;  // filenames (we write the ' .race' file in separate frame so the user sees the correct filename on screen).
GenericDocumentClass@ g_displayed_document = null;
string g_displayed_doc_filename;
string fileBeingWritten = ""; 


void drawUI()
{
    // Draw document window
    if (@g_displayed_document != null)
    {
        drawDocumentWindow();
    }
    
    drawDetectedRaces();
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
        case STAGE_GENFILES:
        {
            ImGui::TextDisabled("Writing .race files:");
            ImGui::Text(fileBeingWritten);
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
                g_displayed_doc_filename = race.raceName;
                @g_displayed_document = @convertedRaces[i] ;
            }
        }
        ImGui::PopID(); // i
    }
    ImGui::PopID(); //"drawDetectedRaces"
}


void drawDocumentWindow()
{
    ImGui::PushID("document view");
    string caption = "Document view (" + g_displayed_doc_filename + ")";
    bool documentOpen = true;
    ImGui::Begin(caption, documentOpen, /*flags:*/0);
    
    GenericDocContextClass reader(g_displayed_document);
    while (!reader.endOfFile())
    {
        switch (reader.tokenType())
        {
            // These tokens are always at start of line
            case TOKEN_TYPE_KEYWORD:
            ImGui::TextColored(color(1.f, 1.f, 0.f, 1.f), reader.getTokKeyword());
            break;
            case TOKEN_TYPE_COMMENT:
            ImGui::TextDisabled(";" + reader.getTokComment());
            break;
            
            // Linebreak is implicit in DearIMGUI, no action needed
            case TOKEN_TYPE_LINEBREAK:
            break;
            
            // Other tokens come anywhere - delimiting logic is needed
            default:
            if (reader.getPos() != 0 && reader.tokenType(-1) != TOKEN_TYPE_LINEBREAK)
            {
                ImGui::SameLine();
                string delimiter = (reader.tokenType(-1) == TOKEN_TYPE_KEYWORD) ? " " : ", ";
                ImGui::Text(delimiter);
                ImGui::SameLine();
            }
            
            switch (reader.tokenType())
            {
                case TOKEN_TYPE_STRING:
                ImGui::TextColored(color(0.f, 1.f, 1.f, 1.f), "\"" + reader.getTokString() + "\"");
                break;
                case TOKEN_TYPE_NUMBER:
                ImGui::Text("" + reader.getTokFloat());
                break;
                case TOKEN_TYPE_BOOL:
                ImGui::TextColored(color(1.f, 0.f, 1.f, 1.f), ""+reader.getTokBool());
                break;
            }
        }
        
        reader.moveNext();
    }
    
    ImGui::End();
    
    // Handle window X button
    if (!documentOpen)
    {
        g_displayed_doc_filename = "";
        @g_displayed_document = null;
    }
    
    ImGui::PopID(); //"document view"
}

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
        convertedAndSavedRaces.insertLast("~");  // ~ means "update UI and redner frame first, then do the writing"
    }
}

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
            stage = STAGE_GETPROJECT;
            break;
        }      
        
        case STAGE_GETPROJECT:
        {
            @projectEntry = modcache.findEntryByFilename(LOADER_TYPE_TERRAIN, /*partial:*/false, projectName+'.terrn2');
            if (@projectEntry != null)
            {
                stage = STAGE_GENFILES;
            }
            break;
        }
        
        case STAGE_CONVERT:
        {
            bool convertedOne = false;
            for (uint i=0; i < races.raceList.length(); i++)
            {
                if (@convertedRaces[i] == null)
                {
                    @convertedRaces[i] = convertSingleRace(races.raceList[i]);
                    convertedOne = true;
                    break;
                }
            }
            
            if (!convertedOne)
            {
                stage = STAGE_IDLE;
            }
            break;
        }
        
        case STAGE_GENFILES:
        {
            for (uint i=0; i < races.raceList.length(); i++)
            {
                if (convertedAndSavedRaces[i] == "~")
                {
                    string filename = generateRaceFileName(races.raceList[i].raceName);
                    fileBeingWritten = filename;
                    convertedAndSavedRaces[i] =""; // ready to write file
                    //                    game.log('DBG GENFILES ['+i+']: fileBeingWritten='+fileBeingWritten);
                    break;
                }
                else if (convertedAndSavedRaces[i] == "")
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
                    convertedAndSavedRaces[i] = fileBeingWritten;
                    //                    game.log('DBG GENFILES ['+i+']: saved file '+fileBeingWritten);
                    if (i == races.raceList.length()-1)
                    {
                        stage=STAGE_FIXTERRN2;
                    }
                    break;
                }             
            }
            break;
        }
        
        case STAGE_FIXTERRN2:
        {
            fixupTerrn2Document();
            stage = STAGE_DONE;
            break;
        }
        
        default: 
        break;        
    }
}

string BRACE="[";
void fixupTerrn2Document()
{
    GenericDocumentClass terrn2;
    int flags = GENERIC_DOCUMENT_OPTION_ALLOW_NAKED_STRINGS
    | GENERIC_DOCUMENT_OPTION_ALLOW_SLASH_COMMENTS
    | GENERIC_DOCUMENT_OPTION_ALLOW_HASH_COMMENTS
    | GENERIC_DOCUMENT_OPTION_ALLOW_SEPARATOR_EQUALS
    | GENERIC_DOCUMENT_OPTION_ALLOW_BRACED_KEYWORDS;
    terrn2.loadFromResource(projectName+".terrn2", projectEntry.resource_group, flags);
    GenericDocContextClass ctx(terrn2);
    
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
    for (uint i=0; i < convertedAndSavedRaces.length(); i++)
    {
        ctx.appendTokKeyword(convertedAndSavedRaces[i]);
        ctx.appendTokLineBreak();
    }
    
    forceExportINI(terrn2);
    
    // delete original file (GenericDocument cannot overwrite)
    game.deleteResource(projectName+".terrn2", projectEntry.resource_group);
    
    // write out modified file
    terrn2.saveToResource(projectName+".terrn2", projectEntry.resource_group);
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

void appendKeyValuePair( GenericDocContextClass@ ctx, string key, string value)
{
    ctx.appendTokKeyword( key);
    ctx.appendTokString(value);
    ctx.appendTokLineBreak();
}

void appendKeyValuePair( GenericDocContextClass@ ctx, string key, int value)
{
    ctx.appendTokKeyword( key);
    ctx.appendTokFloat(value);
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
            ctx.appendTokFloat((i+1)); // checkpointNum (1+)
            ctx.appendTokFloat((j+1)); // altpathNum (1+)
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

void frameStep(float dt)
{
    // === DRAW UI ===
    if (    ImGui::Begin("Race import", closeBtnHandler.windowOpen, /*flags:*/0))
    {
        // Draw the "Terminate this script?" prompt on the top (if not disabled by config).
        closeBtnHandler.draw();
        drawUI();
        ImGui::End();
    }
    
    
    // === PERFORM IMPORT STEP ===
    advanceImportOneStep();
}


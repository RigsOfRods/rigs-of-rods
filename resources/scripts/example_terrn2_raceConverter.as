/// \title terrn2 race converter
/// \brief imports races from scripts and generates challenge files.
/// new section in terrn2 format: [Challenges]
/// each line is a tobj-like file with any extension (i.e. *.race) which is loaded by the race system.
/// ==================================================

enum Stage
{
   STAGE_IDLE,
   STAGE_PUSHMSG,
   STAGE_GENFILES,
   STAGE_DONE,
   STAGE_ERROR
}
Stage stage = STAGE_IDLE;
string error = "";

void frameStep(float dt)
{
    // === DRAW UI ===

    switch(stage)
    {
        case STAGE_IDLE:
            if (@game.getTerrain() != null 
                && stage == STAGE_IDLE
                && ImGui::Button("Convert races from script to terrn2 [Challenges]"))
            {
                stage = STAGE_PUSHMSG;
            }   
            break;            
        case STAGE_ERROR:
            ImGui::Text("ERROR! "+error);
            break;
        case STAGE_PUSHMSG:
            ImGui::Text("Performing MSG_EDI_CREATE_PROJECT_REQUESTED"); 
            break;
        default:
            break;
    }

    // === PERFORM IMPORT STEP ===
    switch (stage)
    {
        case STAGE_PUSHMSG:
            {
                // Fetch terrain's modcache entry
                CacheEntryClass@ src_entry = modcache.findEntryByFilename(LOADER_TYPE_TERRAIN, /*partial:*/false, game.getTerrain().getTerrainFileName());
                if (@src_entry == null)
                {
                    error = "Not found in modcache!!";
                    stage = STAGE_ERROR;
                }
            
                // request project to be created from that cache entry
                game.pushMessage(MSG_EDI_CREATE_PROJECT_REQUESTED, {
                {'name', game.getTerrain().getTerrainName() + " [Challenges]"},
                {'source_entry', src_entry}
                });
                stage = STAGE_GENFILES;
            }      
            break;
        default: 
            break;        
    }
}

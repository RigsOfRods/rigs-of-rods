// Prototype truck editor, Oct 2023
// (sorry about the indentation mess, I scribbled this in 'script_editor.as')
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

GenericDocumentClass@ displayedDocument = null;
int hoveredTokenPos = -1;
int focusedTokenPos = -1;
string errorString;
string nameOfProjectPendingCreation; // name of the truck file which will become available in modcache.
CacheEntryClass@ projectEntry;



// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    if (@projectEntry == null)
    {
        // check if the actor is already a project
        BeamClass@ actor = game.getCurrentTruck();
        if (@actor != null)
        {
            CacheEntryClass@ entry =  modcache.findEntryByFilename(LOADER_TYPE_ALLBEAM, /*partial:*/false, actor.getTruckFileName());
            if (@entry != null)
            {
                @projectEntry = @entry;
                loadDocument();
            }
        }
    }
    checkForCreatedProject();
    drawWindow();
}



void drawWindow()
{
    
    string caption = "RigEditor";
    if (@projectEntry != null)
    {
        caption += " ("+projectEntry.fname+")";
    }
    int flags = ImGuiWindowFlags_MenuBar;
    if (ImGui::Begin(caption, closeBtnHandler.windowOpen, flags))
    {
        closeBtnHandler.draw();
        if (ImGui::BeginMenuBar())
        {
            drawDocumentControls(); // <- menubar
            ImGui::EndMenuBar();
        }
        if (@displayedDocument != null)
        {
            vector2 size = ImGui::GetWindowSize() - vector2(25, 200);
            ImGui::BeginChild("docBody", size);
            drawDocumentBody();
            ImGui::EndChild();
            
            if ( focusedTokenPos > -1)
            {
                ImGui::Separator();
                drawTokenEditPanel();
            }
        }
        
        ImGui::End();
    }
    
}

void drawTokenEditPanel()
{
    GenericDocContextClass reader(displayedDocument);
    while (!reader.endOfFile() && reader.getPos() != uint(focusedTokenPos)) reader.moveNext();
    if (reader.endOfFile())
    {
        ImGui::Text("EOF!!");
    }
    else
    {
        ImGui::TextDisabled("Token pos: "); ImGui::SameLine(); ImGui::Text("" + reader.getPos());
        ImGui::TextDisabled("Token type: "); ImGui::SameLine(); ImGui::Text(tokenTypeStr(reader.tokenType()));
    }
}

string tokenTypeStr(TokenType t)
{
    switch (t)
    {
        case TOKEN_TYPE_NUMBER: return "Number";
        case TOKEN_TYPE_STRING: return "String";
        case TOKEN_TYPE_BOOL:  return "Boolean";
        case TOKEN_TYPE_COMMENT: return "Comment";
        case TOKEN_TYPE_LINEBREAK: return "Line break";
        case TOKEN_TYPE_KEYWORD: return "Keyword";
    }
    return "?";
}

void createProjectFromActorAsync(BeamClass@ actor )
{
    // Fetch the current actor cache entry
    CacheEntryClass@ src_entry = modcache.findEntryByFilename(LOADER_TYPE_ALLBEAM, /*partial:*/false, actor.getTruckFileName());
    if (@src_entry == null)
    errorString = "Failed to load cache entry!";
    
    // request project to be created from that cache entry
    string proj_name = "project1";
    game.pushMessage(MSG_EDI_CREATE_PROJECT_REQUESTED, {
    {'name', proj_name},
    {'source_entry', src_entry}
    });
    
    // Now we have to wait until next frame for the project to be created
    nameOfProjectPendingCreation = proj_name + "." + src_entry.fext; // there's no notification event yet, we must poll.
}

void loadDocument()
{
    GenericDocumentClass@ doc = GenericDocumentClass();
    int flags = GENERIC_DOCUMENT_OPTION_ALLOW_NAKED_STRINGS
    | GENERIC_DOCUMENT_OPTION_ALLOW_SLASH_COMMENTS
    | GENERIC_DOCUMENT_OPTION_FIRST_LINE_IS_TITLE
    | GENERIC_DOCUMENT_OPTION_ALLOW_SEPARATOR_COLON
    | GENERIC_DOCUMENT_OPTION_PARENTHESES_CAPTURE_SPACES;
    if (!doc.loadFromResource(projectEntry.fname, projectEntry.resource_group, flags))
    {
        errorString = "Project file failed to load!";
        return;
    }
    @displayedDocument = doc;
}

void loadAndFixupDocument()
{
    loadDocument();
    
    // fixup the document.. 
    
    GenericDocContextClass@ ctx = GenericDocContextClass(displayedDocument);
    // >> seek the name
    while (!ctx.isTokString()) {
        ctx.seekNextLine(); 
    }
    // >> change the name
    if (!ctx.endOfFile()) {
        ctx.setTokString(0, projectEntry.dname);
    }
    // >> seek fileinfo 
    while (!ctx.isTokKeyword() || ctx.getTokKeyword() != 'fileinfo') {
        ctx.seekNextLine(); 
    }
    // change the fileinfo param #2 categoryid
    if (!ctx.endOfFile(2)) {
        ctx.setTokFloat(2, 8990); // special "Project" category
    }
}

void checkForCreatedProject()
{
    // there's no notification event for completed request, we must poll like this.
    if (nameOfProjectPendingCreation != "")
    {
        @projectEntry =  modcache.findEntryByFilename(LOADER_TYPE_ALLBEAM, /*partial:*/false, nameOfProjectPendingCreation);
        if (@projectEntry != null)
        {
            // success!! stop polling and open the document.
            nameOfProjectPendingCreation="";
            loadAndFixupDocument();
        }
    }
}

void drawDocumentControls()
{
    if (errorString != "")
    {
        ImGui::Text("ERROR! " + errorString);
        return;
    }
    else if (@displayedDocument != null)
    {
        ImGui::TextDisabled("Click tokens with mouse to edit. Spawn as usual (use category 'Projects')");
        ImGui::SameLine();
        if (ImGui::SmallButton("Save file"))
        {
            displayedDocument.saveToResource(projectEntry.fname, projectEntry.resource_group);
        }
    }
    else   if (nameOfProjectPendingCreation != "") 
    {
        ImGui::Text ("Waiting for project to be created...");
    }
    else
    {
        BeamClass@ actor = game.getCurrentTruck();
        if (@actor != null)
        {
            // Actor name and "View document" button
            ImGui::PushID("actor");
            ImGui::AlignTextToFramePadding();
            ImGui::Text("You are driving " + actor.getTruckName());
            ImGui::SameLine();
            if (@displayedDocument == null)
            {
                if (ImGui::SmallButton("Import as project"))
                {
                    createProjectFromActorAsync(actor);
                }
            }
            
            ImGui::PopID(); //"actor"
        }
        else
        {
            ImGui::Text("You are on foot. Spawn a vehicle to access it's definition file.");
        }
    }
}

void drawDocumentBody()
{
    ImGui::PushID("docBody");
    bool hover_found = false;
    
    GenericDocContextClass reader(displayedDocument);
    while (!reader.endOfFile())
    {
        
        
        switch (reader.tokenType())
        {
            // These tokens are always at start of line
            case TOKEN_TYPE_KEYWORD:
            ImGui::TextColored(tokenColor(reader), reader.getTokKeyword());
            break;
            case TOKEN_TYPE_COMMENT:
            ImGui::TextColored(tokenColor(reader), ";" + reader.getTokComment());
            break;
            
            // Linebreak is implicit in DearIMGUI, no action needed
            case TOKEN_TYPE_LINEBREAK:
            if (reader.getPos() != 0 && reader.tokenType(-1) != TOKEN_TYPE_LINEBREAK)
            {
                ImGui::SameLine();
            }
            ImGui::TextColored(tokenColor(reader), "<br>");
            //  ImGui::SameLine();  ImGui::Text(""); // hack to fix highlight of last token on line.
            break;
            
            // Other tokens come anywhere - delimiting logic is needed
            default:
            if (reader.getPos() != 0 && reader.tokenType(-1) != TOKEN_TYPE_LINEBREAK)
            {
                ImGui::SameLine();
                //    string delimiter = (reader.tokenType(-1) == TOKEN_TYPE_KEYWORD) ? " " : ", ";
                //    ImGui::Text(delimiter);
                //    ImGui::SameLine();
            }
            
            switch (reader.tokenType())
            {
                case TOKEN_TYPE_STRING:
                ImGui::TextColored(tokenColor(reader), "\"" + reader.getTokString() + "\"");
                break;
                case TOKEN_TYPE_NUMBER:
                ImGui::TextColored(tokenColor(reader), "" + reader.getTokFloat());
                break;
                case TOKEN_TYPE_BOOL:
                ImGui::TextColored(tokenColor(reader), ""+reader.getTokBool());
                break;
            }
        }
        
        if (ImGui::IsItemHovered()) { 
            hoveredTokenPos = reader.getPos() ;
            hover_found = true;
        }
        
        if (ImGui::IsItemClicked(0))
        {
            focusedTokenPos = reader.getPos(); 
        }
        
        reader.moveNext();
        
    }
    
    if (!hover_found) 
    {
        hoveredTokenPos = -1;
    }
    
    ImGui::PopID(); // "docBody"
}

color tokenColor(GenericDocContextClass@ reader)
{
    if (focusedTokenPos > -1 && reader.getPos() == uint(focusedTokenPos))
    {
        return color(0.9f, 0.1f, 0.1f, 1.f);
    }
    
    if (hoveredTokenPos > -1 && reader.getPos() == uint(hoveredTokenPos))
    {
        return color(0.1f, 1.f, 0.1f, 1.f);
    }
    
    switch (reader.tokenType())
    {
        case TOKEN_TYPE_KEYWORD:            return color(1.f, 1.f, 0.f, 1.f);
        case TOKEN_TYPE_COMMENT:               return color(0.5f, 0.5f, 0.5f, 1.f);
        case TOKEN_TYPE_STRING:               return color(0.f, 1.f, 1.f, 1.f);
        case TOKEN_TYPE_NUMBER:                return color(0.9, 0.9, 0.9, 1.f);
        case TOKEN_TYPE_BOOL:              return color(1.f, 0.f, 1.f, 1.f);      
        case TOKEN_TYPE_LINEBREAK:      return color(0.66f, 0.55f, 0.33f, 1.f);
    } // end switch
    return color(0.9, 0.9, 0.9, 1.f);
} // end tokenColor()








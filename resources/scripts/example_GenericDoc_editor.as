// Prototype truck editor, Oct 2023
// (sorry about the indentation mess, I scribbled this in 'script_editor.as')
// ===================================================

class RigEditor
{
     // ---- variables -----

    GenericDocumentClass@ g_displayed_document = null;
    string g_displayed_doc_filename;
    int m_hovered_token_pos = -1;
    int m_focused_token_pos = -1;
    string m_error_str;
    string m_project_creation_pending; // name of the truck file which will become available in modcache.
    CacheEntryClass@ m_project_entry;

    // ---- functions ----

    void drawWindow()
    {

        string caption = "RigEditor (" + g_displayed_doc_filename + ")";
       int flags = ImGuiWindowFlags_MenuBar;
        ImGui::Begin(caption, /*open:*/true, flags);
          if (ImGui::BeginMenuBar())
         {
             this.drawDocumentControls(); // <- menubar
             ImGui::EndMenuBar();
    }
        if (@g_displayed_document != null)
       {
        vector2 size = ImGui::GetWindowSize() - vector2(25, 200);
       ImGui::BeginChild("docBody", size);
       this.drawDocumentBody();
       ImGui::EndChild();

         if ( m_focused_token_pos > -1)
         {
            ImGui::Separator();
            this.drawTokenEditPanel();
         }
       }

       ImGui::End();

    }

    void drawTokenEditPanel()
    {
        GenericDocContextClass reader(g_displayed_document);
      while (!reader.endOfFile() && reader.getPos() != uint(m_focused_token_pos)) reader.moveNext();
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
         CacheEntryClass@ src_entry = cache.findEntryByFilename(LOADER_TYPE_ALLBEAM, /*partial:*/false, actor.getTruckFileName());
         if (@src_entry == null)
            m_error_str = "Failed to load cache entry!";

       // request project to be created from that cache entry
       string proj_name = "project1";
       game.pushMessage(MSG_EDI_CREATE_PROJECT_REQUESTED, {
           {'name', proj_name},
           {'source_entry', src_entry}
       });

       // Now we have to wait until next frame for the project to be created
       m_project_creation_pending = proj_name + "." + src_entry.fext; // there's no notification event yet, we must poll.
    }

   void loadAndFixupDocument()
   {
        GenericDocumentClass@ doc = GenericDocumentClass();
        int flags = GENERIC_DOCUMENT_OPTION_ALLOW_NAKED_STRINGS
                  | GENERIC_DOCUMENT_OPTION_ALLOW_SLASH_COMMENTS
                  | GENERIC_DOCUMENT_OPTION_FIRST_LINE_IS_TITLE
                  | GENERIC_DOCUMENT_OPTION_ALLOW_SEPARATOR_COLON
                  | GENERIC_DOCUMENT_OPTION_PARENTHESES_CAPTURE_SPACES;
        if (doc.loadFromResource(m_project_entry.fname, m_project_entry.resource_group, flags))
        {
            @g_displayed_document = @doc;
            g_displayed_doc_filename = m_project_entry.fname;
        }
        else
        {
            m_error_str = "Project file failed to load!";
        }

        // fixup the document.. set name
        GenericDocContextClass@ ctx = GenericDocContextClass(g_displayed_document);
        // >> seek the name
        while (ctx.tokenType() != TOKEN_TYPE_STRING)
        ctx.seekNextLine();
        // >> change the name
        ctx.setTokString(0, m_project_entry.dname);

        // TBD... also change category to "8990 Project" ... now it's temporarily set in the CacheEntry via code but next cacheregen will verify it.
   }

    void checkForCreatedProject()
    {
        // there's no notification event for completed request, we must poll like this.
         if (m_project_creation_pending != "")
         {
              @m_project_entry =  cache.findEntryByFilename(LOADER_TYPE_ALLBEAM, /*partial:*/false, m_project_creation_pending);
             if (@m_project_entry != null)
              {
                  // success!! stop polling and open the document.
                  m_project_creation_pending="";
                  this.loadAndFixupDocument();
               }
          }
     }

    void drawDocumentControls()
    {
        if (m_error_str != "")
        {
            ImGui::Text("ERROR! " + m_error_str);
            return;
        }
        else if (@g_displayed_document != null)
        {
            ImGui::TextDisabled("Click tokens with mouse to edit. Spawn as usual (use category 'Projects')");
            ImGui::SameLine();
            if (ImGui::SmallButton("Save file"))
            {
             g_displayed_document.saveToResource(m_project_entry.fname, m_project_entry.resource_group);
            }
        }
        else if (m_project_creation_pending != "") 
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
                if (@g_displayed_document == null)
                {
                    if (ImGui::SmallButton("Import as project"))
                    {
                       this.createProjectFromActorAsync(actor);
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

        GenericDocContextClass reader(g_displayed_document);
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
            m_hovered_token_pos = reader.getPos() ;
           hover_found = true;
        }

        if (ImGui::IsItemClicked(0))
       {
           m_focused_token_pos = reader.getPos(); 
        }
            
            reader.moveNext();

        }

        if (!hover_found) 
       {
           m_hovered_token_pos = -1;
        }
        
        ImGui::PopID(); // "docBody"
    }

   color tokenColor(GenericDocContextClass@ reader)
  {
     if (m_focused_token_pos > -1 && reader.getPos() == uint(m_focused_token_pos))
    {
        return color(0.9f, 0.1f, 0.1f, 1.f);
     }

     if (m_hovered_token_pos > -1 && reader.getPos() == uint(m_hovered_token_pos))
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

    void update(float dt)
   {
       this.checkForCreatedProject();
       this.drawWindow();
    }
}

RigEditor rigEditor;

// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    rigEditor.update(dt);
}



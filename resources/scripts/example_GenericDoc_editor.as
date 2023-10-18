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
}

   ImGui::End();

}

// helper of `drawWindow()`
void drawDocumentControls()
{

    if (@g_displayed_document != null)
   {
       ImGui::TextDisabled("Document loaded OK.");
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
                if (ImGui::Button("View document"))
                {
                    GenericDocumentClass@ doc = GenericDocumentClass();
                    int flags = GENERIC_DOCUMENT_OPTION_ALLOW_NAKED_STRINGS
                              | GENERIC_DOCUMENT_OPTION_ALLOW_SLASH_COMMENTS
                              | GENERIC_DOCUMENT_OPTION_FIRST_LINE_IS_TITLE
                              | GENERIC_DOCUMENT_OPTION_ALLOW_SEPARATOR_COLON
                              | GENERIC_DOCUMENT_OPTION_PARENTHESES_CAPTURE_SPACES;
                    if (doc.loadFromResource(actor.getTruckFileName(), actor.getTruckFileResourceGroup(), flags))
                    {
                        @g_displayed_document = @doc;
                        g_displayed_doc_filename = actor.getTruckFileName();
                    }
                }
            }
            else
            {
                if (ImGui::Button("Close document"))
                {
                    @g_displayed_document = null;
                    g_displayed_doc_filename = "";
                }
            }
            ImGui::PopID(); //"actor"
        }
        else
        {
            ImGui::Text("You are on foot");
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
           ImGui::SameLine();  ImGui::Text(""); // hack to fix highlight of last token on line.
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
        case TOKEN_TYPE_COMMENT:             return color(0.5f, 0.5f, 0.5f, 1.f);
            case TOKEN_TYPE_STRING:               return color(0.f, 1.f, 1.f, 1.f);
            case TOKEN_TYPE_NUMBER:                return color(0.9, 0.9, 0.9, 1.f);
            case TOKEN_TYPE_BOOL:              return color(1.f, 0.f, 1.f, 1.f);       
         } // end switch
     return color(0.9, 0.9, 0.9, 1.f);
     } // end tokenColor()
}

RigEditor rigEditor;

// `frameStep()` runs every frame; `dt` is delta time in seconds.
void frameStep(float dt)
{
    rigEditor.drawWindow();
}



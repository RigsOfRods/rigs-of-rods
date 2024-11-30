/// \title GenericDocument viewing/editing
/// \brief  IMGUI-based editor of tokenized GenericDocument
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"


// By convention, all includes have filename '*_utils' and namespace matching filename.
namespace genericdoc_utils
{
    const int FLAGSET_RACE
    = GENERIC_DOCUMENT_OPTION_ALLOW_NAKED_STRINGS;

    const int FLAGSET_TOBJ
    = GENERIC_DOCUMENT_OPTION_ALLOW_NAKED_STRINGS
    | GENERIC_DOCUMENT_OPTION_ALLOW_SLASH_COMMENTS;

    const int FLAGSET_TERRN2
    = GENERIC_DOCUMENT_OPTION_ALLOW_NAKED_STRINGS
    | GENERIC_DOCUMENT_OPTION_ALLOW_SLASH_COMMENTS
    | GENERIC_DOCUMENT_OPTION_ALLOW_HASH_COMMENTS
    | GENERIC_DOCUMENT_OPTION_ALLOW_SEPARATOR_EQUALS
    | GENERIC_DOCUMENT_OPTION_ALLOW_BRACED_KEYWORDS;

    const int FLAGSET_TRUCK
    = GENERIC_DOCUMENT_OPTION_ALLOW_NAKED_STRINGS
    | GENERIC_DOCUMENT_OPTION_ALLOW_SLASH_COMMENTS
    | GENERIC_DOCUMENT_OPTION_FIRST_LINE_IS_TITLE // unique for RigDef format
    | GENERIC_DOCUMENT_OPTION_ALLOW_SEPARATOR_COLON
    | GENERIC_DOCUMENT_OPTION_PARENTHESES_CAPTURE_SPACES;

    shared string tokenTypeStr(TokenType t)
    {
        switch (t)
        {
            case TOKEN_TYPE_FLOAT: return "Float";
            case TOKEN_TYPE_INT: return "Integer";
            case TOKEN_TYPE_STRING: return "String";
            case TOKEN_TYPE_BOOL:  return "Boolean";
            case TOKEN_TYPE_COMMENT: return "Comment";
            case TOKEN_TYPE_LINEBREAK: return "Line break";
            case TOKEN_TYPE_KEYWORD: return "Keyword";
        }
        return "?"; // `default:` does not do, compiler compilains
    }

    class GenericDocEditor
    {
        int hoveredTokenPos = -1;
        int focusedTokenPos = -1;
        string errorString;
        // Document window state
        GenericDocumentClass@ displayedDocument = null;
        string displayedDocFilename;
        string windowTitleOverride = ""; // defaults to filename
        imgui_utils::CloseWindowPrompt gdeCloseBtnHandler;

        GenericDocEditor()
        {
            this.gdeCloseBtnHandler.cfgTerminateWholeScript = false;
            this.gdeCloseBtnHandler.cfgPromptText = "Really close this document? You will lose any unsaved changes.";
        }

        void setDocument(GenericDocumentClass@ doc, string filename = "")
        {
            @displayedDocument = @doc;
            displayedDocFilename = filename;
            this.resetTokenEditPanel();
        }

        void loadDocument(string filename, string rgname, int flags)
        {
            GenericDocumentClass@ doc = GenericDocumentClass();

            if (doc.loadFromResource(filename, rgname, flags))
            {
                @displayedDocument = @doc;
                displayedDocFilename = filename;
                this.resetTokenEditPanel();
            }
        }

        void resetTokenEditPanel()
        {
            hoveredTokenPos = -1;
            focusedTokenPos = -1;
        }

        void drawSeparateWindow()
        {
            string caption = windowTitleOverride;
            if (caption == "")
            {
                caption = displayedDocFilename + " ~ GenericDocViewer";
            }
            int flags = 0;// ImGuiWindowFlags_MenuBar;

            if (ImGui::Begin(caption, this.gdeCloseBtnHandler.windowOpen, flags))
            {
                gdeCloseBtnHandler.draw();


                if (@displayedDocument != null)
                {
                    vector2 size = ImGui::GetWindowSize() - vector2(20, 150);
                    ImGui::BeginChild("docBody", size);
                    drawDocumentBody();
                    ImGui::EndChild();


                    ImGui::Separator();

                    drawTokenEditPanel();

                }

                ImGui::End();

            }

            if (gdeCloseBtnHandler.exitRequested)
            {
                @displayedDocument = null;
                displayedDocFilename = "";
                this.resetTokenEditPanel();
                gdeCloseBtnHandler.reset(); // <-- somewhat clumsy, don't forget!
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
                        case TOKEN_TYPE_FLOAT:
                        {
                            ImGui::Text("" + reader.getTokFloat());
                            break;
                        }
                        case TOKEN_TYPE_INT:
                        {
                            ImGui::Text("" + reader.getTokInt());
                            break;
                        }
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
                case TOKEN_TYPE_FLOAT:                return color(0.9, 0.8, 0.9, 1.f);
                case TOKEN_TYPE_INT:                return color(0.9, 0.9, 0.8, 1.f);
                case TOKEN_TYPE_BOOL:              return color(1.f, 0.f, 1.f, 1.f);
                case TOKEN_TYPE_LINEBREAK:      return color(0.66f, 0.55f, 0.33f, 1.f);
            } // end switch
            return color(0.9, 0.9, 0.9, 1.f);
        }

        void drawTokenEditPanel()
        {
            if ( focusedTokenPos == -1)
            {
                ImGui::TextDisabled("Use left mouse button to select tokens.");
                return; // nothing to draw
            }
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

    } // class GenericDocEditor

} // namespace genericdoc_utils





// Project Rigs of Rods (http://www.rigsofrods.org)
//             SCRIPT EDITOR
// Documentation: http://developer.rigsofrods.org
// -----------------------------------------------

ScriptEditor editor;

// Callback functions for the game:
// --------------------------------

void main() // Invoked by the game on startup
{
    game.log("Script editor started!");
    game.registerForEvent(SE_ANGELSCRIPT_MSGCALLBACK);
}


void frameStep(float dt) // Invoked regularly by the game; the parameter is elapsed time in seconds.
{
    editor.draw();
}


void eventCallbackEx(scriptEvents ev, // Invoked by the game when a registered event is triggered
    int arg1, int arg2ex, int arg3ex, int arg4ex,
    string arg5ex, string arg6ex, string arg7ex, string arg8ex)
{
    if (ev == SE_ANGELSCRIPT_MSGCALLBACK)
    {
        editor.onEventAngelScriptMsg(
            /*id:*/arg1, /*eType:*/arg2ex, /*row:*/arg3ex, /*col:*/arg4ex,
            /*sectionName:*/arg5ex, /*msg:*/arg6ex);
    }
}

// The script editor logic:
// ------------------------

class ScriptEditor
{
    void draw()
    {
        int flags = ImGuiWindowFlags_MenuBar;
        ImGui::Begin("Script editor", /*open:*/true, flags);
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Dummy menu item")) {}
                ImGui::EndMenu();
            }        
            if (ImGui::Button("|> RUN"))
            {
                this.runBuffer();
            }
            
            ImGui::EndMenuBar();
        }
        ImGui::InputTextMultiline("", this.buffer);
        ImGui::End();
    }
    
    void runBuffer()
    {
        game.pushMessage(MSG_APP_LOAD_SCRIPT_REQUESTED, {
            {'filename', 'Script Editor'}, // Because we supply the buffer, this will serve only as display name
            {'buffer', this.buffer },
            {'category', SCRIPT_CATEGORY_CUSTOM}
        });
    }
    
    void onEventAngelScriptMsg(int scriptUnitId, int msgType, int row, int col, string sectionName, string message)
    {
        game.log("DBG onEventAngelScriptMsg(): scriptUnitId:" + scriptUnitId
            + ", msgType:" + msgType + ", row:" + row + ", col:" + col // ints
            + ", sectionName:" + sectionName + ", message:" + message); // strings
            
        messages.insertLast({ {'type', msgType}, {'row',row}, {'col', col}, {'sectionName', sectionName}, {'message', message} });
    }
    
    string buffer;
    array<dictionary> messages;
}

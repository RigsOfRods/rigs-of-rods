// EXAMPLE - querying key states and events
// ===================================================

// Window [X] button handler
#include "imgui_utils.as"
imgui_utils::CloseWindowPrompt closeBtnHandler;

string keyEventLog;  // keys pressed history
string keyEventSeparator;
uint HISTORY_LEN = 100;

void frameStep(float dt)
{
    if (ImGui::Begin("Example", closeBtnHandler.windowOpen, 0))
    {
        closeBtnHandler.draw();
    
        drawBody();
        
        ImGui::End();
    }
}

void showKeyState(keyCodes kc, string desc)
{
   // Draw the immediate state
   ImGui::Text(desc); ImGui::SameLine(); ImGui::Text(""+inputs.isKeyDownEffective(kc));

   // remember the keypress event
   if (inputs.isKeyDownValueBounce(kc)) { 
     keyEventLog = desc + keyEventSeparator + keyEventLog;
     keyEventSeparator = ", ";
    // History len cap
     if (keyEventLog.length() > HISTORY_LEN) { keyEventLog.resize(HISTORY_LEN); }
  }
}

void drawBody()
{
    ImGui::TextDisabled("===[   K E Y   S T A T E S :   ]===");
    
    showKeyState(KC_F1, "F1"); ImGui::SameLine(); ImGui::Dummy(vector2(10, 1));
    showKeyState(KC_F2, "F2"); ImGui::SameLine(); ImGui::Dummy(vector2(10, 1));
    showKeyState(KC_F3, "F3"); ImGui::SameLine(); ImGui::Dummy(vector2(10, 1));
    showKeyState(KC_F4, "F4");

   ImGui::TextDisabled("===[   K E Y   H I S T O R Y :   ]===");

   ImGui::Text(keyEventLog);
}

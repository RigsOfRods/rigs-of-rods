// EXAMPLE - querying key states and events
// ===================================================

string keyEventLog;  // keys pressed history
string keyEventSeparator;
int HISTORY_LEN = 100;

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

void frameStep(float dt)
{
    ImGui::TextDisabled("===[   K E Y   S T A T E S :   ]===");
    
    showKeyState(KC_F1, "F1"); ImGui::SameLine(); ImGui::Dummy(vector2(10, 1));
    showKeyState(KC_F2, "F2"); ImGui::SameLine(); ImGui::Dummy(vector2(10, 1));
    showKeyState(KC_F3, "F3"); ImGui::SameLine(); ImGui::Dummy(vector2(10, 1));
    showKeyState(KC_F4, "F4");

   ImGui::TextDisabled("===[   K E Y   H I S T O R Y :   ]===");

   ImGui::Text(keyEventLog);
}

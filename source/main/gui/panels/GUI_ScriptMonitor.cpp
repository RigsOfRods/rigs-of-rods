/*
    This source file is part of Rigs of Rods
    Copyright 2021 tritonas00
    For more information, see http://www.rigsofrods.org/
    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.
    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/


#include "GUI_ScriptMonitor.h"

#include "Actor.h"
#include "ContentManager.h"
#include "ScriptEngine.h"

#include <Ogre.h>
#include "OgreImGui.h" 

using namespace RoR;
using namespace GUI;

void ScriptMonitor::Draw()
{
    // Table setup
    ImGui::Columns(3);
    ImGui::SetColumnWidth(0, 30);
    ImGui::SetColumnWidth(1, 200);
    ImGui::SetColumnWidth(2, 100);

    // Header
    ImGui::TextDisabled("ID");
    ImGui::NextColumn();
    ImGui::TextDisabled("File name");
    ImGui::NextColumn();
    ImGui::TextDisabled("Options");
    
    ImGui::Separator();

    ScriptUnitId_t id_to_reload = SCRIPTUNITID_INVALID;
    ScriptUnitId_t id_to_stop = SCRIPTUNITID_INVALID;

    for (auto& pair : App::GetScriptEngine()->getScriptUnits())
    {
        ScriptUnitId_t id = pair.first;
        ScriptUnit const& unit = pair.second;
        ImGui::NextColumn();
        ImGui::TextDisabled("%d", id);
        ImGui::NextColumn();
        ImGui::Text("%s", unit.scriptName.c_str());
        ImGui::NextColumn();
        switch (unit.scriptCategory)
        {
        case ScriptCategory::TERRAIN:
            ImGui::Text("(terrain)");
            break;

        case ScriptCategory::CUSTOM:
            if (ImGui::Button("Reload"))
            {
                // Reloading here would mess up the hashmap we're iterating (plus invalidate the iterator).
                id_to_reload = id;
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop"))
            {
                // Stopping here would mess up the hashmap we're iterating (plus invalidate the iterator).
                id_to_stop = id;
            }
            break;
        default:;
        }
    }

    ImGui::Columns(1); // reset

    if (id_to_reload != SCRIPTUNITID_INVALID)
    {
        // Back up the data, the ScriptUnit object will be invalidated!
        ScriptUnit const& unit = App::GetScriptEngine()->getScriptUnit(id_to_reload);
        std::string filename = unit.scriptName;
        ScriptCategory category = unit.scriptCategory;
        App::GetScriptEngine()->unloadScript(id_to_reload);
        App::GetScriptEngine()->loadScript(filename, category);
    }

    if (id_to_stop != SCRIPTUNITID_INVALID)
    {
        App::GetScriptEngine()->unloadScript(id_to_stop);
    }
}

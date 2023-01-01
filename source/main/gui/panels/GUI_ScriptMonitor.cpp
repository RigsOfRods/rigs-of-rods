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
#include "GameContext.h"
#include "ScriptEngine.h"
#include "Utils.h"

#include <Ogre.h>
 

using namespace RoR;
using namespace GUI;
using namespace Ogre;

void ScriptMonitor::Draw()
{
    // Table setup
    ImGui::Columns(3);
    ImGui::SetColumnWidth(0, 25);
    ImGui::SetColumnWidth(1, 200);
    ImGui::SetColumnWidth(2, 200);

    // Header
    ImGui::TextDisabled(_LC("ScriptMonitor", "ID"));
    ImGui::NextColumn();
    ImGui::TextDisabled(_LC("ScriptMonitor", "File name"));
    ImGui::NextColumn();
    ImGui::TextDisabled(_LC("ScriptMonitor", "Options"));
    
    this->DrawCommentedSeparator(_LC("ScriptMonitor", "Active"));

    StringVector autoload = StringUtil::split(App::app_custom_scripts->getStr(), ",");
    for (auto& pair : App::GetScriptEngine()->getScriptUnits())
    {
        ScriptUnitId_t id = pair.first;
        ImGui::PushID(id);

        ScriptUnit const& unit = pair.second;
        ImGui::NextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%d", id);
        ImGui::NextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", unit.scriptName.c_str());
        ImGui::NextColumn();
        switch (unit.scriptCategory)
        {
        case ScriptCategory::ACTOR:
            ImGui::Text("({} [%u] '%s')", _LC("ScriptMonitor", "actor"), unit.associatedActor->ar_vector_index, unit.associatedActor->getTruckName().c_str());
            break;

        case ScriptCategory::TERRAIN:
            ImGui::Text("%s", _LC("ScriptMonitor", "(terrain)"));
            break;

        case ScriptCategory::CUSTOM:
        {
            if (ImGui::Button(_LC("ScriptMonitor", "Reload")))
            {
                App::GetGameContext()->PushMessage(Message(MSG_APP_UNLOAD_SCRIPT_REQUESTED, new ScriptUnitId_t(id)));
                LoadScriptRequest* req = new LoadScriptRequest();
                req->lsr_category = unit.scriptCategory;
                req->lsr_filename = unit.scriptName;
                App::GetGameContext()->ChainMessage(Message(MSG_APP_LOAD_SCRIPT_REQUESTED, req));
            }
            ImGui::SameLine();
            if (ImGui::Button(_LC("ScriptMonitor", "Stop")))
            {
                App::GetGameContext()->PushMessage(Message(MSG_APP_UNLOAD_SCRIPT_REQUESTED, new ScriptUnitId_t(id)));
            }

            ImGui::SameLine();
            bool autoload_set = std::find(autoload.begin(), autoload.end(), unit.scriptName) != autoload.end();
            if (ImGui::Checkbox(_LC("ScriptMonitor", "Autoload"), &autoload_set))
            {
                if (autoload_set)
                    CvarAddFileToList(App::app_custom_scripts, unit.scriptName);
                else
                    CvarRemoveFileFromList(App::app_custom_scripts, unit.scriptName);
            }
            break;
        }
        default:;
        }

        ImGui::PopID(); // ScriptUnitId_t id
    }

    if (App::app_recent_scripts->getStr() != "")
    {
        // Prepare display list for recent scripts
        m_recent_displaylist.clear();
        StringVector recent = StringUtil::split(App::app_recent_scripts->getStr(), ",");
        for (String& filename : recent)
        {
            bool is_running = std::find_if(
                App::GetScriptEngine()->getScriptUnits().begin(),
                App::GetScriptEngine()->getScriptUnits().end(),
                [filename](ScriptUnitMap::const_iterator::value_type pair) { return filename == pair.second.scriptName; })
                != App::GetScriptEngine()->getScriptUnits().end();
            if (!is_running)
            {
                m_recent_displaylist.push_back(filename);
            }
        }

        // Draw recent scripts from the displaylist
        if (m_recent_displaylist.size() > 0)
        {
            this->DrawCommentedSeparator(_LC("ScriptMonitor", "Recent"));

            for (String& filename : m_recent_displaylist)
            {
                ImGui::PushID(filename.c_str());

                ImGui::NextColumn();
                ImGui::NextColumn(); // skip "ID"
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", filename.c_str());
                ImGui::NextColumn();
                float cursorx = ImGui::GetCursorPosX();
                if (ImGui::Button(_LC("ScriptMonitor", "Load")))
                {
                    LoadScriptRequest* req = new LoadScriptRequest();
                    req->lsr_category = ScriptCategory::CUSTOM;
                    req->lsr_filename = filename;
                    App::GetGameContext()->PushMessage(Message(MSG_APP_LOAD_SCRIPT_REQUESTED, req));
                }

                ImVec2 rem_size = ImGui::CalcTextSize(_LC("ScriptMonitor", "Remove"));
                ImGui::SameLine();
                ImGui::SetCursorPosX(((cursorx + 190) - rem_size.x) - 2*ImGui::GetStyle().FramePadding.x);
                if (ImGui::SmallButton(_LC("ScriptMonitor", "Remove")))
                {
                    CvarRemoveFileFromList(App::app_recent_scripts, filename);
                }

                ImGui::PopID(); // filename.c_str()
            }
        }
    }

    ImGui::Columns(1); // reset
}

void ScriptMonitor::DrawCommentedSeparator(const char* text)
{
    ImGui::NextColumn(); // begin new row
    ImGui::NextColumn(); // skip ID column
    ImVec2 pos = ImGui::GetCursorScreenPos() + ImVec2(10.f, 0.f);
    ImGui::Dummy(ImVec2(0.1f, 2.5f));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.1f, 2.5f));
    ImDrawList* drawlist = ImGui::GetWindowDrawList();
    ImVec2 padding(5.f, 0.f);
    ImVec2 rect_max = pos + padding*2 + ImGui::CalcTextSize(text);
    drawlist->AddRectFilled(pos, rect_max, ImColor(ImGui::GetStyle().Colors[ImGuiCol_PopupBg]), ImGui::GetStyle().WindowRounding);
    drawlist->AddText(pos + padding, ImColor(ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]), text);
    ImGui::NextColumn(); // skip Name column
}

/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2019 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#include "GUI_MainSelector.h"

#include "Application.h"
#include "BeamFactory.h"
#include "CacheSystem.h"
#include "ContentManager.h"
#include "GUIManager.h"
#include "GUI_LoadingWindow.h"
#include "InputEngine.h"
#include "Language.h"
#include "MainMenu.h"
#include "RoRFrameListener.h"
#include "RoRPrerequisites.h"
#include "SkinManager.h"
#include "Utils.h"

#include <MyGUI.h>
#include <imgui.h>
#include <imgui_internal.h>

using namespace RoR;
using namespace GUI;

// MAIN SELECTOR WINDOW
// --------------------
// KEY CONTROLS
//   tab: toggle search box focus (FIXME: currently, only focusing works)
//   arrow left/right: if search box is not in focus, select previous(left) or next(right) category in "categories" combobox, wrap around when at either end.
//   arrow up/down: select prev/next entry. When already on top/bottom item, wrap to other end of the list.
//   enter: activate highlighted entry
// SEARCHING
//   The result list is sorted descending by 'score'.
//   Syntax 'abcdef': searches fulltext (ingoring case) in: name, filename, description, author name/mail (in this order, with descending rank) and returns rank+string pos as score
//   Syntax 'AREA:abcdef': searches (ignoring case) in AREA: 'guid'(guid string), 'author' (name/email), 'wheels' (string "WHEELCOUNTxPROPWHEELCOUNT"), 'file' (filename); returns string pos as score

void MainSelector::Show(LoaderType type, std::string const& filter_guid)
{
    m_loader_type = type;
    m_selected_category = m_last_selected_category[type]; // Bounds are checked in UpdateDisplayLists()
    m_search_method = CacheSearchMethod::NONE;
    m_search_input.Clear();
    m_search_string.clear();
    m_filter_guid = filter_guid;
    this->UpdateDisplayLists();
    if (m_last_selected_entry[m_loader_type] < m_display_entries.size())
    {
        m_selected_entry = m_last_selected_entry[m_loader_type];
    }
}

void MainSelector::Draw()
{
    GUIManager::GuiTheme const& theme = App::GetGuiManager()->GetTheme();

    ImGuiWindowFlags win_flags = ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowPosCenter(ImGuiSetCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, 550.f), ImGuiSetCond_FirstUseEver);
    bool keep_open = true;
    if (!ImGui::Begin(_L("Loader"), &keep_open, win_flags))
    {
        return;
    }

    App::GetGuiManager()->RequestGuiCaptureKeyboard(true);

    // category keyboard control
    const int num_categories = static_cast<int>(m_display_categories.size());
    if (!m_searchbox_was_active)
    {
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)))
        {
            m_selected_category = (m_selected_category + 1) % num_categories; // select next item and wrap around at bottom.
            m_last_selected_category[m_loader_type] = m_selected_category;
            this->UpdateDisplayLists();
        }
        else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)))
        {
            m_selected_category = (m_selected_category > 0) ? (m_selected_category - 1) : (num_categories - 1); // select prev. item and wrap around on top
            m_last_selected_category[m_loader_type] = m_selected_category;
            this->UpdateDisplayLists();
        }
    }

    // category combobox
    ImGui::PushItemWidth(LEFT_PANE_WIDTH);
    if (ImGui::Combo(
        "##SelectorCategory", &m_selected_category,
        &MainSelector::CatComboItemGetter, &m_display_categories, num_categories))
    {
        m_last_selected_category[m_loader_type] = m_selected_category;
        this->UpdateDisplayLists();
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();

    // search box
    ImGui::PushItemWidth(ImGui::GetWindowWidth() - 
        (LEFT_PANE_WIDTH + 2*(ImGui::GetStyle().WindowPadding.x) + ImGui::GetStyle().ItemSpacing.x));
    if (!m_searchbox_was_active && ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Tab)))
    {
        ImGui::SetKeyboardFocusHere();
    }
    if (ImGui::InputText("##SelectorSearch", m_search_input.GetBuffer(), m_search_input.GetCapacity()))
    {
        m_selected_category = 0; // 'All'
        // `m_last_selected_category` intentionally not updated
        this->UpdateSearchParams();
        this->UpdateDisplayLists();
    }
    // TODO: Remove focus using tab key (not possible with DearIMGUI?)
    m_searchbox_was_active = ImGui::IsItemActive();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY()
        + (ImGui::GetStyle().WindowPadding.y - ImGui::GetStyle().ItemSpacing.y));
    ImGui::PopItemWidth();
    ImGui::Separator();

    // left
    ImGui::BeginChild("left pane", ImVec2(LEFT_PANE_WIDTH, 0), true);
    const int num_entries = static_cast<int>(m_display_entries.size());
    bool scroll_to_selected = false;
    // Entry list: handle keyboard
    if (m_selected_entry != -1) // -1 indicates empty entry-list
    {
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)))
        {
            m_selected_entry = (m_selected_entry + 1) % num_entries; // select next item and wrap around at bottom.
            m_last_selected_entry[m_loader_type] = m_selected_entry;
            scroll_to_selected = true;
        }
        else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)))
        {
            m_selected_entry = (m_selected_entry > 0) ? (m_selected_entry - 1) : (num_entries - 1); // select prev. item and wrap around on top
            m_last_selected_entry[m_loader_type] = m_selected_entry;
            scroll_to_selected = true;
        }
    }
    // Entry list: display
    for (int i = 0; i < num_entries; ++i)
    {
        DisplayEntry& d_entry = m_display_entries[i];
        bool is_selected = (i == m_selected_entry);
        if (ImGui::Selectable(d_entry.sde_entry->dname.c_str(), &is_selected))
        {
            m_selected_entry = i;
            m_last_selected_entry[m_loader_type] = m_selected_entry;
            m_selected_sectionconfig = 0;
        }
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) // Left doubleclick
        {
            m_selected_entry = i;
            m_last_selected_entry[m_loader_type] = m_selected_entry;
            m_selected_sectionconfig = 0;
            this->Apply();
        }
        if (is_selected && scroll_to_selected)
        {
            ImGui::SetScrollHere();
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();

    // right
    ImGui::BeginGroup();

    ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing())); // Leave room for 1 line below us

    if (m_selected_entry != -1)
    {
        DisplayEntry& sd_entry = m_display_entries[m_selected_entry];

        // Preview image
        if (!sd_entry.sde_entry->filecachename.empty())
        {
            ImVec2 cursor_pos = ImGui::GetCursorPos();
            Ogre::TexturePtr preview_tex =
                Ogre::TextureManager::getSingleton().load(
                    sd_entry.sde_entry->filecachename, Ogre::RGN_DEFAULT);
            if (preview_tex)
            {
                // Scale the image
                ImVec2 max_size = (ImGui::GetWindowSize() * PREVIEW_SIZE_RATIO);
                ImVec2 size(preview_tex->getWidth(), preview_tex->getHeight());
                size *= max_size.x / size.x; // Fit size along X
                if (size.y > max_size.y) // Reduce size along Y if needed
                {
                    size *= max_size.y / size.y;
                }
                // Draw the image
                ImGui::SetCursorPos((cursor_pos + ImGui::GetWindowSize()) - size);
                ImGui::Image(reinterpret_cast<ImTextureID>(preview_tex->getHandle()), size);
                ImGui::SetCursorPos(cursor_pos);
            }
        }

        // Title and description
        ImGui::TextColored(theme.highlight_text_color, "%s", sd_entry.sde_entry->dname.c_str());
        ImGui::TextWrapped("%s", sd_entry.sde_entry->description.c_str());
        ImGui::Separator();

        // Details
        for (AuthorInfo const& author: sd_entry.sde_entry->authors)
        {
            ImGui::TextDisabled("%s", _L("Author(s): "));
            ImGui::SameLine();
            ImGui::TextColored(theme.highlight_text_color, "%s [%s]", author.name.c_str(), author.type.c_str());
        }
        this->DrawAttrInt(_L("Version: "), sd_entry.sde_entry->version);
        if (sd_entry.sde_entry->wheelcount > 0)
        {
            ImGui::Text("%s", _L("Wheels: ")); 
            ImGui::SameLine();
            ImGui::TextColored(theme.highlight_text_color, "%dx%d", sd_entry.sde_entry->wheelcount, sd_entry.sde_entry->propwheelcount);
        }
        if (sd_entry.sde_entry->truckmass > 0)
        {
            ImGui::Text("%s", _L("Mass: ")); 
            ImGui::SameLine();
            ImGui::TextColored(theme.highlight_text_color, "%.2f t", Round(sd_entry.sde_entry->truckmass / 1000.0f, 3));
        }

        if (m_show_details)
        {
            if (sd_entry.sde_entry->loadmass > 0)
            {
                ImGui::Text("%s", _L("Load Mass: ")); 
                ImGui::SameLine();
                ImGui::TextColored(theme.highlight_text_color, "%f.2 t", Round(sd_entry.sde_entry->loadmass / 1000.0f, 3));
            }
            this->DrawAttrInt(_L("Nodes: "), sd_entry.sde_entry->nodecount);
            this->DrawAttrInt(_L("Beams: "), sd_entry.sde_entry->beamcount);
            this->DrawAttrInt(_L("Shocks: "), sd_entry.sde_entry->shockcount);
            this->DrawAttrInt(_L("Hydros: "), sd_entry.sde_entry->hydroscount);
            this->DrawAttrInt(_L("SoundSources: "), sd_entry.sde_entry->soundsourcescount);
            this->DrawAttrInt(_L("Commands: "), sd_entry.sde_entry->commandscount);
            this->DrawAttrInt(_L("Rotators: "), sd_entry.sde_entry->rotatorscount);
            this->DrawAttrInt(_L("Exhausts: "), sd_entry.sde_entry->exhaustscount);
            this->DrawAttrInt(_L("Flares: "), sd_entry.sde_entry->flarescount);
            this->DrawAttrInt(_L("Flexbodies: "), sd_entry.sde_entry->flexbodiescount);
            this->DrawAttrInt(_L("Props: "), sd_entry.sde_entry->propscount);
            this->DrawAttrInt(_L("Wings: "), sd_entry.sde_entry->wingscount);
            if (sd_entry.sde_entry->hasSubmeshs)
            {
                ImGui::Text("%s", _L("Using Submeshs: ")); 
                ImGui::SameLine();
                ImGui::TextColored(theme.highlight_text_color, "%s", sd_entry.sde_entry->hasSubmeshs ?_L("Yes") : _L("No"));
            }
            this->DrawAttrFloat(_L("Torque: "), sd_entry.sde_entry->torque);
            this->DrawAttrInt(_L("Transmission Gear Count: "), sd_entry.sde_entry->numgears);
            if (sd_entry.sde_entry->minrpm > 0)
            {
                ImGui::Text("%s", _L("Engine RPM: ")); 
                ImGui::SameLine();
                ImGui::TextColored(theme.highlight_text_color, "%f - %f", sd_entry.sde_entry->minrpm, sd_entry.sde_entry->maxrpm);
            }
            this->DrawAttrStr(_L("Unique ID: "), sd_entry.sde_entry->uniqueid);
            this->DrawAttrStr(_L("GUID: "), sd_entry.sde_entry->guid);
            this->DrawAttrInt(_L("Times used: "), sd_entry.sde_entry->usagecounter);
            this->DrawAttrStr(_L("Date and Time modified: "), sd_entry.sde_filetime_str.ToCStr());
            this->DrawAttrStr(_L("Date and Time installed: "), sd_entry.sde_addtime_str.ToCStr());
            this->DrawAttrStr(_L("Vehicle Type: "), sd_entry.sde_driveable_str.ToCStr());

            this->DrawAttrSpecial(sd_entry.sde_entry->forwardcommands, _L("[forwards commands]"));
            this->DrawAttrSpecial(sd_entry.sde_entry->importcommands, _L("[imports commands]"));
            this->DrawAttrSpecial(sd_entry.sde_entry->rescuer, _L("[is rescuer]"));
            this->DrawAttrSpecial(sd_entry.sde_entry->custom_particles, _L("[uses custom particles]"));
            this->DrawAttrSpecial(sd_entry.sde_entry->fixescount > 0, _L("[has fixes]"));
            // Engine type 't' (truck) is the default, do not display it
            this->DrawAttrSpecial(sd_entry.sde_entry->enginetype == 'c', _L("[car engine]"));
            this->DrawAttrSpecial(sd_entry.sde_entry->resource_bundle_type == "Zip", _L("[zip archive]"));
            this->DrawAttrSpecial(sd_entry.sde_entry->resource_bundle_type == "FileSystem", _L("[unpacked in directory]"));

            if (!sd_entry.sde_entry->resource_bundle_path.empty())
            {
                ImGui::Text("%s", _L("Source: "));
                ImGui::SameLine();
                ImGui::TextColored(App::GetGuiManager()->GetTheme().value_blue_text_color,
                    "%s", sd_entry.sde_entry->resource_bundle_path.c_str());
            }
            if (!sd_entry.sde_entry->fname.empty())
            {
                ImGui::Text("%s", _L("Filename: "));
                ImGui::SameLine();
                ImGui::TextColored(App::GetGuiManager()->GetTheme().value_blue_text_color,
                    "%s", sd_entry.sde_entry->fname.c_str());
            }
        }
        ImGui::Checkbox(_L("Show details"), &m_show_details);
    }
    ImGui::EndChild();

    ImGui::BeginChild("buttons");
    if (m_selected_entry != -1)
    {
        DisplayEntry& sd_entry = m_display_entries[m_selected_entry];
        if (sd_entry.sde_entry->sectionconfigs.size() > 0)
        {
            ImGui::Combo(_L("Configuration"), &m_selected_sectionconfig,
                    &MainSelector::ScComboItemGetter, sd_entry.sde_entry,
                    static_cast<int>(sd_entry.sde_entry->sectionconfigs.size()));
            ImGui::SameLine();
        }
        if (ImGui::Button(_L("OK")) ||
            ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
        {
            this->Apply();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(_L("Cancel")))
    {
        this->Cancel();
    }
    ImGui::EndChild();

    ImGui::EndGroup();

    ImGui::End();
    if (!keep_open)
    {
        this->Cancel();
    }
};

void MainSelector::UpdateDisplayLists()
{
    int active_category_id = CacheSystem::CATEGORIES[0].ccg_id; // Fallback
    if (!m_display_categories.empty())
    {
        if (m_selected_category >= m_display_categories.size())
        {
            m_selected_category = 0;
        }
        active_category_id = m_display_categories[m_selected_category].sdc_category->ccg_id;
    }

    m_display_categories.clear();
    m_display_entries.clear();

    // Find all relevant entries
    CacheQuery query;
    query.cqy_filter_type = m_loader_type;
    query.cqy_filter_category_id = active_category_id;
    query.cqy_search_method = m_search_method;
    query.cqy_search_string = m_search_string;
    query.cqy_filter_guid = m_filter_guid;

    App::GetCacheSystem()->Query(query);
    if (active_category_id == CacheCategoryId::CID_All)
    {
        m_cache_file_freshness = query.cqy_res_last_update;
    }

    m_selected_entry = -1;
    for (CacheQueryResult const& res: query.cqy_results)
    {
        const bool is_fresh = this->IsEntryFresh(res.cqr_entry);
        if (is_fresh)
        {
            query.cqy_res_category_usage[CacheCategoryId::CID_Fresh]++;
        }

        if (active_category_id != CacheCategoryId::CID_Fresh || is_fresh)
        {
            m_display_entries.push_back(res.cqr_entry);
            m_selected_entry = 0;
        }
    }

    // Display used categories
    for (size_t i = 0; i < CacheSystem::NUM_CATEGORIES; ++i)
    {
        size_t usage = query.cqy_res_category_usage[CacheSystem::CATEGORIES[i].ccg_id];
        if (usage > 0 ||
            CacheSystem::CATEGORIES[i].ccg_id == CacheCategoryId::CID_Fresh) // HACK: Always include the "fresh" category
        {
            m_display_categories.emplace_back(&CacheSystem::CATEGORIES[i], usage);
        }
    }
}

bool MainSelector::IsEntryFresh(CacheEntry* entry)
{
    return entry->filetime >= m_cache_file_freshness - 86400;
}

void MainSelector::UpdateSearchParams()
{
    std::string input = m_search_input.ToCStr();

    if (input.find(":") == std::string::npos)
    {
        m_search_method = CacheSearchMethod::FULLTEXT;
        m_search_string = input;
    }
    else
    {
        Ogre::StringVector v = Ogre::StringUtil::split(input, ":");
        if (v.size() < 2)
        {
            m_search_method = CacheSearchMethod::NONE;
            m_search_string = "";
        }
        else if (v[0] == "guid")
        {
            m_search_method = CacheSearchMethod::GUID;
            m_search_string = v[1];
        }
        else if (v[0] == "author")
        {
            m_search_method = CacheSearchMethod::AUTHORS;
            m_search_string = v[1];
        }
        else if (v[0] == "wheels")
        {
            m_search_method = CacheSearchMethod::WHEELS;
            m_search_string = v[1];
        }
        else if (v[0] == "file")
        {
            m_search_method = CacheSearchMethod::FILENAME;
            m_search_string = v[1];
        }
        else
        {
            m_search_method = CacheSearchMethod::NONE;
            m_search_string = "";
        }
    }
}

// Static helper
bool MainSelector::ScComboItemGetter(void* data, int idx, const char** out_text)
{
    CacheEntry* entry = static_cast<CacheEntry*>(data);
    if (out_text)
        *out_text = entry->sectionconfigs.at(idx).c_str();
    return true;
}

void MainSelector::DrawAttrInt(const char* title, int val) const
{
    if (val > 0)
    {
        ImGui::Text("%s", title);
        ImGui::SameLine();
        ImGui::TextColored(App::GetGuiManager()->GetTheme().highlight_text_color, "%d", val);
    }
}

void MainSelector::DrawAttrFloat(const char* title, float val) const
{
    if (val > 0)
    {
        ImGui::Text("%s", title);
        ImGui::SameLine();
        ImGui::TextColored(App::GetGuiManager()->GetTheme().highlight_text_color, "%f", val);
    }
}

void MainSelector::DrawAttrSpecial(bool val, const char* label) const
{
    if (val)
    {
        ImGui::TextColored(App::GetGuiManager()->GetTheme().value_red_text_color, "%s", label);
    }
}

void MainSelector::DrawAttrStr(const char* desc, std::string const& str) const
{
    if (!str.empty())
    {
        ImGui::Text("%s", desc);
        ImGui::SameLine();
        ImGui::TextColored(App::GetGuiManager()->GetTheme().highlight_text_color, "%s", str.c_str());
    }
}

void MainSelector::Close()
{
    m_selected_entry = -1;
    m_selected_sectionconfig = 0;
    m_searchbox_was_active = false;
    m_loader_type = LT_None; // Hide window
}

void MainSelector::Cancel()
{
    this->Close();

    if (App::app_state.GetActive() == AppState::MAIN_MENU)
    {
        RoR::App::GetMainMenu()->LeaveMultiplayerServer(); // TODO: replace by message queue ~ 11/2019
        App::GetGuiManager()->SetVisible_GameMainMenu(true);
    }
    else if (App::app_state.GetActive() == AppState::SIMULATION)
    {
        App::GetSimController()->OnLoaderGuiCancel();
    }
}

void MainSelector::Apply()
{
    assert(m_selected_entry > -1); // Programmer error
    DisplayEntry& sd_entry = m_display_entries[m_selected_entry];

    if (m_loader_type == LT_Terrain &&
        App::app_state.GetActive() == AppState::MAIN_MENU)
    {
        App::sim_terrain_name.SetPending(sd_entry.sde_entry->fname.c_str());
        App::app_state.SetPending(AppState::SIMULATION);
        this->Close();
    }
    else if (App::app_state.GetActive() == AppState::SIMULATION)
    {
        LoaderType type = m_loader_type;
        std::string sectionconfig;
        if (sd_entry.sde_entry->sectionconfigs.size() > 0)
        {
            sectionconfig = sd_entry.sde_entry->sectionconfigs[m_selected_sectionconfig];
        }
        this->Close();

        App::GetSimController()->OnLoaderGuiApply(type, sd_entry.sde_entry, sectionconfig);
    }
}

// Static helper
bool MainSelector::CatComboItemGetter(void* data, int idx, const char** out_text)
{
    DisplayCategoryVec* dc_vec = static_cast<DisplayCategoryVec*>(data);
    if (out_text)
        *out_text = dc_vec->at(idx).sdc_title.ToCStr();
    return true;
}

MainSelector::DisplayEntry::DisplayEntry(CacheEntry* entry):
    sde_entry(entry)
{
    // Pre-format strings
    if (sde_entry->filetime > 0)
    {
        sde_filetime_str = asctime(gmtime(&sde_entry->filetime));
    }
    if (sde_entry->addtimestamp > 0)
    {
        sde_addtime_str = asctime(gmtime(&sde_entry->addtimestamp));
    }
    if (sde_entry->nodecount > 0)
    {
        static const char* str[] =
            {_L("Non-Driveable"), _L("Truck"), _L("Airplane"), _L("Boat"), _L("Machine")};
        sde_driveable_str = str[sde_entry->driveable];
    }
}

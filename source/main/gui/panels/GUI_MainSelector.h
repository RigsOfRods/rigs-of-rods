/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

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

/// @file
/// @author Moncef Ben Slimane (original MyGUI window, 11/2014)
/// @author Petr Ohlidal (remake to DearIMGUI, 11/2019)

#pragma once

#include "Application.h" // Str
#include "SimData.h" // ActorSpawnRequest
#include "CacheSystem.h" // CacheSearchMethod
#include "ForwardDeclarations.h"

#include <map>

namespace RoR {
namespace GUI {

class MainSelector 
{
public:
    const float LEFT_PANE_WIDTH = 250.f;
    const float PREVIEW_SIZE_RATIO = 0.7f;

    void Show(LoaderType type, std::string const& filter_guid = "", CacheEntry* advertised_entry = nullptr);
    bool IsVisible() { return m_loader_type != LT_None; };
    bool IsHovered() { return IsVisible() && m_is_hovered; }
    bool m_kb_focused = true;
    void Draw();
    void Close();

private:
    struct DisplayCategory
    {
        DisplayCategory(int id, std::string const& name, size_t usage);

        int        sdc_category_id;
        Str<200>   sdc_title;
    };

    struct DisplayEntry
    {
        DisplayEntry(CacheEntry* entry);

        CacheEntry* sde_entry = nullptr;
        Str<50>     sde_filetime_str;  // pre-formatted
        Str<50>     sde_addtime_str;   // pre-formatted
    };

    typedef std::vector<DisplayCategory> DisplayCategoryVec;
    typedef std::vector<DisplayEntry>    DisplayEntryVec;

    void UpdateDisplayLists();
    void UpdateSearchParams();
    void Apply();
    void Cancel();
    void DrawAttrInt(const char* desc, int val) const;
    void DrawAttrFloat(const char* desc, float val) const;
    void DrawAttrSpecial(bool val, const char* label) const;
    void DrawAttrStr(const char* desc, std::string const& str) const;
    void DrawCommonDetails(DisplayEntry& sd_entry);

    static bool ScComboItemGetter(void* data, int idx, const char** out_text);
    static bool CatComboItemGetter(void* data, int idx, const char** out_text);

    LoaderType         m_loader_type = LT_None;
    DisplayCategoryVec m_display_categories;
    DisplayEntryVec    m_display_entries;
    CacheSearchMethod  m_search_method = CacheSearchMethod::NONE;
    std::string        m_search_string;
    std::string        m_filter_guid;                //!< Used for skins
    Str<500>           m_search_input;
    bool               m_show_details = false;
    bool               m_searchbox_was_active = false;
    CacheEntry*        m_advertised_entry = nullptr; //!< Always shown on top, even if not existing in modcache (i.e. dummy default skin)
    bool               m_is_hovered = false;

    int                m_selected_category = 0;    //!< Combobox position (uses display list)
    int                m_selected_cid = 0;         //!< Category ID
    int                m_selected_entry = -1;
    int                m_selected_sectionconfig = 0;

    std::map<LoaderType, int> m_last_selected_category; //!< Last category-combobox position for each loader type
    std::map<LoaderType, int> m_last_selected_cid;      //!< Last selected category-ID for each loader type
    std::map<LoaderType, int> m_last_selected_entry;    //!< Stores the last manually selected entry index for each loader type
};

} // namespace GUI
} // namespace RoR

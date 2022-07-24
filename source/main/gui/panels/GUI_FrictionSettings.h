/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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
/// @author Thomas Fischer (thomas{AT}thomasfischer{DOT}biz)
/// @date   7th of September 2009

// Remade using DearIMGUI by Petr Ohlidal, 10/2019

#pragma once

#include "Application.h"

#include "SimData.h"

namespace RoR {
namespace GUI {

class FrictionSettings
{
public:

    struct Entry
    {
        Entry(const ground_model_t* const gm):
            backup_copy(*gm), working_copy(*gm), live_data(gm)
        {}

        ground_model_t backup_copy;
        ground_model_t working_copy;
        const ground_model_t* const live_data;
    };

    void SetVisible(bool visible) { m_is_visible = visible; }
    bool IsVisible() const { return m_is_visible; }
    bool IsHovered() const { return IsVisible() && m_is_hovered; }

    void AnalyzeTerrain();
    void setActiveCol(const ground_model_t* gm) { m_nearest_gm = gm; }
    void Draw();

private:
    static bool GmComboItemGetter(void* data, int idx, const char** out_text);
    void DrawTooltip(const char* title, const char* text);

    std::vector<Entry> m_gm_entries;
    int m_selected_gm = 0;
    const ground_model_t* m_nearest_gm = nullptr;
    bool m_is_visible = false;
    bool m_is_hovered = false;
};

} // namespace GUI
} // namespace RoR

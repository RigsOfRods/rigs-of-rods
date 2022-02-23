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

#pragma once

#include <string>

namespace RoR {

/// @addtogroup Terrain
/// @{

/// Minimalist editor mode; orig. code by Ulteq/2016
/// * Enter/Exit from/to simulation by Ctrl+Y (see EV_COMMON_TOGGLE_TERRAIN_EDITOR)
/// * Select object by middle mouse button or Enter key (closest to avatar)
/// * Rotate/move selected object with keys
/// Upon exit, file 'editor_out.cfg' is written to ROR_HOME/config (see RGN_CONFIG)
class TerrainEditor
{
public:
    void UpdateInputEvents(float dt);
    void WriteOutputFile();
    void ClearSelection();

private:
    bool                m_object_tracking = true;
    int                 m_rotation_axis = 1;        //!< 0=X, 1=Y, 2=Z
    std::string         m_last_object_name;
    int                 m_object_index = -1;
};

/// @} // addtogroup Terrain

} // namespace RoR

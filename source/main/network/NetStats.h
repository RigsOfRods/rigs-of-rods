/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2023 Petr Ohlidal

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

#ifdef USE_SOCKETW

#include "Application.h"
#include "OgreImGui.h"
#include "RoRnet.h"

namespace RoR {

/// @addtogroup Network
/// @{

typedef std::vector<uint32_t> NetGraphPlotline;

struct NetGraphData
{
    NetGraphPlotline plotline;
    int plotline_basemax = 0;
    int plotline_basemin = 0;

    explicit NetGraphData(size_t size, int basemin, int basemax);

    void         AddSample(uint32_t sample);
    static float ImPlotGetSample(void* data, int idx);
    void         ImPlotLines(const char* label, const char* overlay_text, ImVec2 size);
};

struct NetClientStats
{
    NetGraphData combined_ping = NetGraphData(50, 0, 100); //!< Sum of localclient<->server ping and server<->remoteclient ping (rorserver is a relay).
    NetGraphData sim_timeoffset = NetGraphData(50, -2000, 2000); //!< Difference between client timers; we monitor it's stability.
};

/// @}   //addtogroup Network

} // namespace RoR

#endif // USE_SOCKETW

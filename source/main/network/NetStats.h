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

typedef std::vector<RoRnet::NetTime32_t> NetGraphPlotline;

struct NetGraphData
{
    NetGraphPlotline plotline;
    RoRnet::NetTime32_t plotline_max = 0;
    RoRnet::NetTime32_t plotline_min = 0;
    float plotline_smoothmax = 0.f;
    float plotline_smoothmin = 0.f;

    NetGraphData(size_t size);

    void         AddSample(RoRnet::NetTime32_t sample);
    static float ImPlotGetSample(void* data, int idx);
    void         ImPlotLines(const char* label, const char* overlay_text, ImVec2 size);
};

struct NetClientStats
{
    NetGraphData remote_queue_delay = NetGraphData(20); //!< How long it takes remote client to transmit outgoing packet.
    NetGraphData local_queue_delay = NetGraphData(20);  //!< How long it takes us to process received packet.
    NetGraphData client_time_offset = NetGraphData(40); //!< Difference between client timers; we monitor it's stability.
};

/// @}   //addtogroup Network

} // namespace RoR

#endif // USE_SOCKETW

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

#ifdef USE_SOCKETW

#include "Network.h"

#include "Application.h"
#include "ChatSystem.h"
#include "Console.h"
#include "ErrorUtils.h"
#include "GameContext.h"
#include "GUIManager.h"
#include "GUI_TopMenubar.h"
#include "Language.h"
#include "RoRVersion.h"
#include "ScriptEngine.h"
#include "Utils.h"

#include <Ogre.h>
#include <SocketW.h>

#include <algorithm>
#include <chrono>
#include <cstring>

using namespace RoR;

NetGraphData::NetGraphData(size_t size)
{
    plotline.resize(size, RoRnet::NetTime32_t(0));
}

void NetGraphData::AddSample(RoRnet::NetTime32_t sample)
{
    plotline.erase(plotline.begin());
    plotline.push_back(sample);
    const auto minmax = std::minmax_element(plotline.begin(), plotline.end());
    plotline_min = *minmax.first;
    plotline_max = *minmax.second;

    // smooth scale
    const float SMOOTH_UP = 0.4f;
    const float SMOOTH_DOWN = 0.05f;

    const float fmin = static_cast<float>(*minmax.first);
    if (fmin < plotline_smoothmin)
        plotline_smoothmin = fmin * SMOOTH_UP + plotline_smoothmin * (1.f - SMOOTH_UP);
    else
        plotline_smoothmin = fmin * SMOOTH_DOWN + plotline_smoothmin * (1.f - SMOOTH_DOWN);

    const float fmax = static_cast<float>(*minmax.second);
    if (fmax > plotline_smoothmax)
        plotline_smoothmax = fmax * SMOOTH_UP + plotline_smoothmax * (1.f - SMOOTH_UP);
    else
        plotline_smoothmax = fmax * SMOOTH_DOWN + plotline_smoothmax * (1.f - SMOOTH_DOWN);
}

// static
float NetGraphData::ImPlotGetSample(void* data, int idx)
{
    NetGraphPlotline* plotline = static_cast<NetGraphPlotline*>(data);
    return static_cast<float>(plotline->at(idx));
}

void NetGraphData::ImPlotLines(const char* label, const char* overlay_text, ImVec2 size)
{
    ImGui::PlotLines(
        label,
        NetGraphData::ImPlotGetSample,
        static_cast<void*>(&plotline),
        static_cast<int>(plotline.size()),
        /*values_offset:*/0,
        overlay_text,
        plotline_smoothmin,
        plotline_smoothmax,
        size);
}

#endif // USE_SOCKETW

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

NetGraphData::NetGraphData(size_t size, int basemin, int basemax)
{
    plotline.resize(size, uint32_t(0));
    plotline_basemin = basemin;
    plotline_basemax = basemax;
}

void NetGraphData::AddSample(uint32_t sample)
{
    plotline.erase(plotline.begin());
    plotline.push_back(sample);
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
        (float)plotline_basemin,
        (float)plotline_basemax,
        size);
}

#endif // USE_SOCKETW

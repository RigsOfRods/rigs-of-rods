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

/// @file
/// @brief Generic driver's cab window for locomotives (actors spawned from `.train` files with an engine).

#pragma once

#include "ForwardDeclarations.h"
#include "InputEngine.h"

#include <imgui.h>
#include <set>
#include <vector>

namespace RoR {
namespace GUI {

class TrainPanel
{
public:
    void Draw(RoR::GfxActor* player);

    static bool IsTrain(const ActorPtr& actor); //!< `.train` file with an engine (wagons have none)

private:
    void Detach();                                   //!< Hand all simulated inputs back to the player
    void ApplyLevers(const ActorPtr& actor);
    void DrawStatus(const ActorPtr& actor);
    void DrawTacho(float kmh, float limit_kmh);
    void DrawBar(const char* id, const char* label, float& value, float shown, ImU32 fill, ImU32 grab_edge);
    void DrawSwitches(const ActorPtr& actor);
    void DrawCouplings(const ActorPtr& actor);
    void DrawCommands(const ActorPtr& actor);

    bool ColoredButton(const char* label, ImVec2 size, ImU32 col);
    void HoldButton(const ActorPtr& actor, const char* label, events ev, ImVec2 size, ImU32 col);
    void Pulse(const ActorPtr& actor, events ev);   //!< Toggle: event active for exactly one frame

    ActorPtr            m_actor;                     //!< Actor whose inputs we currently drive
    float               m_throttle = 0.f;            //!< 0..1, stays where the player left it
    float               m_train_brake = 0.f;         //!< 0..1, stays where the player left it
    std::vector<events> m_pulses;                    //!< Released at the start of the next frame
    std::set<int>       m_held;                      //!< Events held by buttons in the previous frame
    std::set<int>       m_held_now;                  //!< Events held by buttons in this frame
    int                 m_closed_for = -1;           //!< Instance ID of the actor the window was closed for
};

} // namespace GUI
} // namespace RoR

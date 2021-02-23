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

#pragma once

#include "Application.h"
#include "SimData.h"

namespace RoR {

class Screwprop : public ZeroedMemoryAllocator
{
public:

    Screwprop( Actor* actor, NodeIdx_t noderef, NodeIdx_t nodeback, NodeIdx_t nodeup, float power);

    void updateForces(int update);
    void setThrottle(float val);
    void setRudder(float val);
    float getThrottle();
    float getRudder();
    void reset();
    void toggleReverse();

private:

    DustPool *splashp, *ripplep;
    bool reverse;
    float fullpower; //!< in HP
    float rudder;
    float throtle;

    // Attachment
    Actor*    m_actor;
    NodeIdx_t nodeback;
    NodeIdx_t noderef;
    NodeIdx_t nodeup;
};

} // namespace RoR


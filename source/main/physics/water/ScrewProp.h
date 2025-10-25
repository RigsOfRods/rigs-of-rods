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
#include "RefCountingObject.h"
#include "RefCountingObjectPtr.h"
#include "SimData.h"

namespace RoR {

/// @addtogroup Gameplay
/// @{

/// @addtogroup Marine
/// @{

class Screwprop : public RefCountingObject<Screwprop>
{
public:

    Screwprop( ActorPtr actor, NodeNum_t noderef, NodeNum_t nodeback, NodeNum_t nodeup, float power);

    void updateForces(int update);
    void setThrottle(float val);
    void setRudder(float val);
    float getThrottle();
    float getRudder();
    float getMaxPower() { return fullpower; }
    bool getReverse() { return reverse; }
    void reset();
    void toggleReverse();

    NodeNum_t   GetRefNode()   const { return noderef; };
    NodeNum_t   GetBackNode()  const { return nodeback; };
    NodeNum_t   GetUpNode()    const { return nodeup; };

private:

    DustPool *splashp, *ripplep;
    bool reverse;
    float fullpower; //!< in HP
    float rudder;
    float throtle;

    // Attachment
    ActorPtr    m_actor;
    NodeNum_t nodeback;
    NodeNum_t noderef;
    NodeNum_t nodeup;
};

/// @} // addtogroup Marine
/// @} // addtogroup Gameplay

} // namespace RoR


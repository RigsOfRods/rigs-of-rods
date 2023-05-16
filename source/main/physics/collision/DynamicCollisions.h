/*
   This source file is part of Rigs of Rods
   Copyright 2005-2012 Pierre-Michel Ricordel
   Copyright 2007-2012 Thomas Fischer
   Copyright 2016 Fabian Killus

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

#include "ForwardDeclarations.h"
#include "SimData.h"

namespace RoR {

/// @addtogroup Physics
/// @{

/// @addtogroup Collisions
/// @{

void ResolveInterActorCollisions(const float dt, PointColDetector &interPointCD,
        const int free_collcab, int collcabs[], int cabs[],
        collcab_rate_t inter_collcabrate[], std::vector<node_t>& nodes,
        const float collrange,
        ground_model_t &submesh_ground_model);

void ResolveIntraActorCollisions(const float dt, PointColDetector &intraPointCD,
        const int free_collcab, int collcabs[], int cabs[],
        collcab_rate_t intra_collcabrate[], std::vector<node_t>& nodes,
        const float collrange,
        ground_model_t &submesh_ground_model);

/// @} // addtogroup Collisions
/// @} // addtogroup Physics

} // namespace RoRs

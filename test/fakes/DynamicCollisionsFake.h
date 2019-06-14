#pragma once

#include "../../source/main/physics/BeamData.h"
#include "../../source/main/physics/collision/Collisions.h"
#include "../../source/main/physics/collision/DynamicCollisions.h"

#ifdef ROR_FAKES_IMPL
    void ResolveIntraActorCollisions(const float dt, PointColDetector &intraPointCD,
        const int free_collcab, int collcabs[], int cabs[],
        collcab_rate_t intra_collcabrate[], node_t nodes[],
        const float collrange,
        ground_model_t &submesh_ground_model)
        {}
#endif // ROR_FAKES_IMPL

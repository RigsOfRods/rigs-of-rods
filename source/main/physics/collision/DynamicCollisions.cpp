/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2016      Fabian Killus

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

#include "DynamicCollisions.h"

#include "Application.h"
#include "Beam.h"
#include "BeamData.h"
#include "CartesianToTriangleTransform.h"
#include "Collisions.h"
#include "PointColDetector.h"
#include "RoRFrameListener.h"
#include "Triangle.h"
#include "node_t.h"

using namespace Ogre;

/// Determine on which side of a triangle an occuring collision takes place.
/**
 * The frontface of the triangle is the side towards which the surface normal is pointing.
 * The backface is the opposite side.
 *
 * \note This implementation is a heuristic, not an accurate calculation.
 *
 * @param distance      Signed shortest distance from collision point to triangle plane.
 * @param normal        Surface normal of the triangle.
 * @param surface_point Arbitrary point within the triangle plane.
 * @param neighbour_node_ids Indices of neighbouring nodes connected to the colliding node.
 * @param nodes         
 */
static bool BackfaceCollisionTest(const float distance,
        const Vector3 &normal,
        const node_t &surface_point,
        const std::vector<int> &neighbour_node_ids,
        const node_t nodes[])
{
    auto sign = [](float x){ return (x >= 0) ? 1 : -1; };

    // Summarize over the collision node and its connected neighbour nodes to infer on which side
    // of the collision plane most of these nodes are located.
    // Nodes in front contribute positively, nodes on the backface contribute negatively.

    // the contribution of the collision node itself has triple weight in this heuristic
    const int weight = 3;
    int face_indicator = weight * sign(distance);

    // calculate the contribution of neighbouring nodes (if it can still change the final outcome)
    if (neighbour_node_ids.size() > weight) {
        for (auto id : neighbour_node_ids) {
            const auto neighbour_distance = normal.dotProduct(nodes[id].AbsPosition - surface_point.AbsPosition);
            face_indicator += sign(neighbour_distance);
        }
    }

    // a negative sum indicates that the collision is occuring on the backface
    return (face_indicator < 0);
}


/// Test if a point given in triangle local coordinates lies within the triangle itself.
/**
 * A point (within in the triangle plane) is located inside the triangle if its barycentric coordinates
 * \f$\alpha, \beta, \gamma\f$ are all positive. The margin additionally defines the maximum distance
 * from the triangle plane within which a three-dimensional point is still considered to be close
 * enough to be inside the triangle.
 *
 * @param local Point in triangle local coordinates.
 * @param margin Range within which a point is considered to be close enough to the triangle plane.
 */
static bool InsideTriangleTest(const CartesianToTriangleTransform::TriangleCoord &local, const float margin)
{
    const auto coord    = local.barycentric;
    const auto distance = local.distance;
    return (coord.alpha >= 0) && (coord.beta >= 0) && (coord.gamma >= 0) && (std::abs(distance) <= margin);
}


/// Calculate collision forces and apply them to the collision node and the three vertex nodes of the collision triangle.
void ResolveCollisionForces(const float penetration_depth,
        node_t &hitnode, node_t &na, node_t &nb, node_t &no,
        const float alpha, const float beta, const float gamma,
        const Vector3 &normal,
        const float dt,
        ground_model_t &submesh_ground_model)
{

    //Find the point's velocity relative to the triangle
    const auto vecrelVel = (hitnode.Velocity - (na.Velocity * alpha + nb.Velocity * beta + no.Velocity * gamma));

    //Find the velocity perpendicular to the triangle
    //if it points away from the triangle the ignore it (set it to 0)
    const float velForce = std::max(0.0f, -vecrelVel.dotProduct(normal));

    //Velocity impulse
    const auto inv_dt = 1.0 / dt;
    const float vi = hitnode.mass * inv_dt * (velForce + inv_dt * penetration_depth) * 0.5f;

    //The force that the triangle puts on the point
    //(applied only when it is towards the point)
    const auto triangle_force = na.Forces * alpha + nb.Forces * beta + no.Forces * gamma;
    const float trfnormal = std::max(0.0f, triangle_force.dotProduct(normal));

    //The force that the point puts on the triangle
    //(applied only when it is towards the triangle)
    const float pfnormal = std::min(0.0f, hitnode.Forces.dotProduct(normal));

    const float fl = (vi + trfnormal - pfnormal) * 0.5f;

    auto forcevec = Vector3::ZERO;
    float nso;  // TODO unused

    //Calculate the collision forces
    primitiveCollision(&hitnode, forcevec, vecrelVel, normal, ((float) dt), &submesh_ground_model, &nso, penetration_depth, fl);

    // apply resulting collision force
    hitnode.Forces += forcevec;
    na.Forces -= alpha * forcevec;
    nb.Forces -= beta * forcevec;
    no.Forces -= gamma * forcevec;
}


void ResolveInterActorCollisions(const float dt, PointColDetector &interPointCD,
        const int free_collcab, int collcabs[], int cabs[],
        collcab_rate_t inter_collcabrate[], node_t nodes[],
        const float collrange,
        ground_model_t &submesh_ground_model)
{
    std::vector<Actor*> all_actors = RoR::App::GetSimController()->GetActors();

    for (int i=0; i<free_collcab; i++)
    {
        if (inter_collcabrate[i].rate > 0)
        {
            inter_collcabrate[i].distance++;
            inter_collcabrate[i].rate--;
            continue;
        }
        inter_collcabrate[i].rate = std::min(inter_collcabrate[i].distance, 12);
        inter_collcabrate[i].distance = 0;

        int tmpv = collcabs[i]*3;
        const auto no = &nodes[cabs[tmpv]];
        const auto na = &nodes[cabs[tmpv+1]];
        const auto nb = &nodes[cabs[tmpv+2]];

        interPointCD.query(no->AbsPosition
                , na->AbsPosition
                , nb->AbsPosition, collrange);

        if (interPointCD.hit_count > 0)
        {
            // setup transformation of points to triangle local coordinates
            const Triangle triangle(na->AbsPosition, nb->AbsPosition, no->AbsPosition);
            const CartesianToTriangleTransform transform(triangle);

            for (int h=0; h<interPointCD.hit_count; h++)
            {
                const auto hitnodeid = interPointCD.hit_list[h]->node_id;
                const auto hit_actor_id = interPointCD.hit_list[h]->actor_id;
                const auto hit_actor = all_actors[hit_actor_id];
                const auto hitnode = &hit_actor->ar_nodes[hitnodeid];

                // transform point to triangle local coordinates
                const auto local_point = transform(hitnode->AbsPosition);

                // collision test
                const bool is_colliding = InsideTriangleTest(local_point, collrange);
                if (is_colliding)
                {
                    inter_collcabrate[i].rate = 0;

                    const auto coord = local_point.barycentric;
                    auto distance   = local_point.distance;
                    auto normal     = triangle.normal();

                    // adapt in case the collision is occuring on the backface of the triangle
                    const auto neighbour_node_ids = hit_actor->ar_node_to_node_connections[hitnodeid];
                    const bool is_backface = BackfaceCollisionTest(distance, normal, *no, neighbour_node_ids, hit_actor->ar_nodes);
                    if (is_backface)
                    {
                        // flip surface normal and distance to triangle plane
                        normal   = -normal;
                        distance = -distance;
                    }

                    const auto penetration_depth = collrange - distance;

                    ResolveCollisionForces(penetration_depth, *hitnode, *na, *nb, *no, coord.alpha,
                            coord.beta, coord.gamma, normal, dt, submesh_ground_model);
                }
            }
        }
        else
        {
            inter_collcabrate[i].rate++;
        }
    }
}


void ResolveIntraActorCollisions(const float dt, PointColDetector &intraPointCD,
        const int free_collcab, int collcabs[], int cabs[],
        collcab_rate_t intra_collcabrate[], node_t nodes[],
        const float collrange,
        ground_model_t &submesh_ground_model)
{
    for (int i=0; i<free_collcab; i++)
    {
        if (intra_collcabrate[i].rate > 0)
        {
            intra_collcabrate[i].distance++;
            intra_collcabrate[i].rate--;
            continue;
        }
        if (intra_collcabrate[i].distance > 0)
        {
            intra_collcabrate[i].rate = std::min(intra_collcabrate[i].distance, 12);
            intra_collcabrate[i].distance = 0;
        }

        int tmpv = collcabs[i]*3;
        const auto no = &nodes[cabs[tmpv]];
        const auto na = &nodes[cabs[tmpv+1]];
        const auto nb = &nodes[cabs[tmpv+2]];

        intraPointCD.query(no->AbsPosition
                , na->AbsPosition
                , nb->AbsPosition, collrange);

        bool collision = false;

        if (intraPointCD.hit_count > 0)
        {
            // setup transformation of points to triangle local coordinates
            const Triangle triangle(na->AbsPosition, nb->AbsPosition, no->AbsPosition);
            const CartesianToTriangleTransform transform(triangle);

            for (int h=0; h<intraPointCD.hit_count; h++)
            {
                const auto hitnodeid = intraPointCD.hit_list[h]->node_id;
                const auto hitnode = &nodes[hitnodeid];

                //ignore wheel/chassis self contact
                if (hitnode->iswheel) continue;
                if (no == hitnode || na == hitnode || nb == hitnode) continue;

                // transform point to triangle local coordinates
                const auto local_point = transform(hitnode->AbsPosition);

                // collision test
                const bool is_colliding = InsideTriangleTest(local_point, collrange);
                if (is_colliding)
                {
                    collision = true;

                    const auto coord = local_point.barycentric;
                    auto distance = local_point.distance;
                    auto normal   = triangle.normal();

                    // adapt in case the collision is occuring on the backface of the triangle
                    if (distance < 0) 
                    {
                        // flip surface normal and distance to triangle plane
                        normal   = -normal;
                        distance = -distance;
                    }

                    const auto penetration_depth = collrange - distance;

                    ResolveCollisionForces(penetration_depth, *hitnode, *na, *nb, *no, coord.alpha,
                            coord.beta, coord.gamma, normal, dt, submesh_ground_model);
                }
            }
        }

        if (collision)
        {
            intra_collcabrate[i].rate = -20000;
        }
        else
        {
            intra_collcabrate[i].rate++;
        }
    }
}

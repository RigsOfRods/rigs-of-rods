#pragma once

#include "../../source/main/physics/collision/Collisions.h"

#include <string>

#ifdef ROR_FAKES_IMPL
    Collisions::Collisions() {}
    Collisions::~Collisions() {}

    bool               Collisions::isInside(Ogre::Vector3, std::string const&, std::string const&, float) { return false; }
    bool               Collisions::isInside(Ogre::Vector3, struct collision_box_t*, float) { return false; }
    ground_model_t*    Collisions::getGroundModelByString(std::string) { return nullptr; }
    Ogre::Vector3      Collisions::getPosition(std::string const &,std::string const &) {return Ogre::Vector3::ZERO;}
    Ogre::Quaternion   Collisions::getDirection(std::string const &,std::string const &) {return Ogre::Quaternion();}
    collision_box_t *  Collisions::getBox(std::string const &,std::string const &) {return nullptr;}
    float              Collisions::getSurfaceHeight(float,float) {return 0.f;}
    float              Collisions::getSurfaceHeightBelow(float,float,float) {return 0.f;}
    bool               Collisions::collisionCorrect(Ogre::Vector3 *,bool) {return false;}

    // Global func
    void ResolveInterActorCollisions(float, class PointColDetector&, int, int* const, int* const, struct collcab_rate_t* const, struct node_t* const, float, struct ground_model_t&) {}
    bool Collisions::groundCollision(struct node_t *,float) {return false;}
    bool Collisions::nodeCollision(struct node_t *,float,bool) {return false;}
#endif // ROR_FAKES_IMPL

#pragma once

#include "../../source/main/physics/SlideNode.h"

#ifdef ROR_FAKES_IMPL
    SlideNode::SlideNode(struct node_t *,struct RailGroup *){}

    void  SlideNode::ResetPositions(void){}
    float SlideNode::getLenTo(struct RailSegment const *)const  {return 0.f;}
    void SlideNode::UpdateForces(float) {}
    void SlideNode::UpdatePosition(void) {}
    float SlideNode::getLenTo(struct beam_t const *)const  {return 0.f;}
    RailSegment * RailGroup::FindClosestSegment(Ogre::Vector3 const &) {return nullptr;}
    Ogre::Vector3 const & SlideNode::GetSlideNodePosition(void)const  {return  Ogre::Vector3::ZERO;}
#endif // ROR_FAKES_IMPL

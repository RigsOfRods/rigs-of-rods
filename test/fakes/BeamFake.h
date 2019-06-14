#pragma once

#include "../../source/main/physics/Beam.h"

#ifdef ROR_FAKES_IMPL
    Actor::Actor(int,unsigned int,class std::shared_ptr<struct RigDef::File>,struct RoR::ActorSpawnRequest) {}
    Actor::~Actor() {}

    void  Actor::CalcNetwork(void)                               {}
    bool  Actor::AddTyrePressure(float)                          { return false; }
    float  Actor::getRotation(void)                              { return 0.f; }
    Ogre::Vector3  Actor::getPosition(void)                      { return Ogre::Vector3::ZERO; }
    void  Actor::ResetPosition(class Ogre::Vector<3,float>,bool) {}
    void  Actor::ResetPosition(float,float,bool,float)           {}
    bool  Actor::ReplayStep(void)                                { return false; }
    void  Actor::ForceFeedbackStep(int)                          {}
    void  Actor::HandleInputEvents(float)                        {}
    void  Actor::HandleAngelScriptEvents(float)                  {}
    void  Actor::ToggleLights(void)                              {}
    void  Actor::ToggleTies(int)                                 {}
    void  Actor::ToggleRopes(int)                                {}
    void  Actor::ToggleParkingBrake(void)                        {}
    void  Actor::updateSkidmarks(void)                           {}
    void  Actor::UpdateFlareStates(float)                        {}
    void  Actor::updateVisual(float)                             {}
    void  Actor::NotifyActorCameraChanged(void)                  {}
    void  Actor::StopAllSounds(void)                             {}
    void  Actor::UnmuteAllSounds(void)                           {}
    bool  Actor::getReverseLightVisible(void)                    { return false; }
    void  Actor::sendStreamData(void)                            {}
    void  Actor::updateDashBoards(float)                         {}
    void  Actor::UpdateBoundingBoxes(void)                       {}
    void  Actor::calculateAveragePosition(void)                  {}
    void  Actor::UpdatePhysicsOrigin(void)                       {}
    bool  Actor::CalcForcesEulerPrepare(bool)                    { return false; }
    void  Actor::CalcForcesEulerCompute(bool,int)                {}
    void  Actor::CalcBeamsInterActor(void)                       {}
    void  Actor::RecalculateNodeMasses(float)                    {}
    void  Actor::calcNodeConnectivityGraph(void)                 {}
    void  Actor::calculateLocalGForces(void)                     {}
    float Actor::GetTyrePressure(void) {return 0.f;}
    Ogre::Vector3 Actor::GetRotationCenter(void) { return Ogre::Vector3::ZERO; }
    float Actor::GetMinHeight(bool) {return 0.f;}
    float Actor::GetMaxHeight(bool) {return 0.f;}
    float Actor::GetHeightAboveGroundBelow(float,bool) {return 0.f;}
    void Actor::ToggleHooks(int,enum hook_states,int) {}
    void Actor::ToggleSlideNodeLock(void) {}
    void Actor::ToggleCustomParticles(void) {}
    void Actor::ToggleBeacons(void) {}
    void Actor::setReplayMode(bool) {}
    void Actor::resolveCollisions(float,bool) {}
    void Actor::prepareInside(bool) {}
    void Actor::toggleBlinkType(enum blinktype) {}
    void Actor::SoftReset(void) {}
    void Actor::SyncReset(bool) {}
    class Replay * Actor::getReplay(void) {return nullptr;}
    void Actor::resetSlideNodes(void) {}
    void Actor::updateSlideNodePositions(void) {}
#endif // ROR_FAKES_IMPL

#pragma once

#include "../../source/main/gfx/GfxActor.h"

#ifdef ROR_FAKES_IMPL
    RoR::GfxActor::GfxActor(class Actor *,class ActorSpawner *,std::string, std::vector<RoR::GfxActor::NodeGfx> &, std::vector<prop_t> &,int,class RoR::Renderdash *){}
    RoR::GfxActor::~GfxActor() {}

    RoR::GfxActor::VideoCamera::VideoCamera(void){}
    RoR::GfxActor::NodeGfx::NodeGfx(unsigned short){}

    void RoR::GfxActor::SetCabLightsActive(bool state_on) {}
    void RoR::GfxActor::ScaleActor(Ogre::Vector3 relpos, float ratio) {}
    void RoR::GfxActor::SetCastShadows(bool value) {}
    void RoR::GfxActor::ResetFlexbodies() {}
    void RoR::GfxActor::SetDebugView(DebugViewType dv) {}
    void RoR::GfxActor::UpdateSimDataBuffer(void)  {}
    void RoR::GfxActor::UpdateWheelVisuals(void)   {}
    void RoR::GfxActor::FinishWheelUpdates(void)   {}
    void RoR::GfxActor::UpdateFlexbodies(void)     {}
    void RoR::GfxActor::FinishFlexbodyTasks(void)  {}
    void RoR::GfxActor::UpdateCabMesh(void)        {}
    void RoR::GfxActor::UpdateWingMeshes(void)     {}
    void RoR::GfxActor::UpdateProps(float,bool)    {}
    void RoR::GfxActor::RegisterCabMaterial(class Ogre::SharedPtr<class Ogre::Material>,class Ogre::SharedPtr<class Ogre::Material>){}
    void RoR::GfxActor::AddMaterialFlare(int,class Ogre::SharedPtr<class Ogre::Material>){}
    void RoR::GfxActor::RegisterCabMesh(class Ogre::Entity *,class Ogre::SceneNode *,class FlexObj *){}
    void RoR::GfxActor::AddRod(int,int,int,char const *,bool,float){}
    void RoR::GfxActor::SetWheelVisuals(unsigned short,struct RoR::GfxActor::WheelGfx){}
    void RoR::GfxActor::RegisterAirbrakes(void){}
    void RoR::GfxActor::SortFlexbodies(void){}
    void RoR::GfxActor::SetVideoCamState(enum RoR::GfxActor::VideoCamState) {}
    void RoR::GfxActor::UpdateDebugView(void) {}
    void RoR::GfxActor::ToggleDebugView(void) {}
    void RoR::GfxActor::CycleDebugViews(void) {}
    void RoR::GfxActor::SetRenderdashActive(bool) {}
    RoR::GfxActor::SimBuffer::SimBuffer(void) {}
#endif // ROR_FAKES_IMPL

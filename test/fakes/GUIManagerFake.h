#pragma once

#include "../../source/main/gui/GUIManager.h"

#include <OgreString.h>
#include <OgreUTFString.h>

#ifdef ROR_FAKES_IMPL
    RoR::GUIManager::GUIManager(void) {}
    RoR::GUIManager::~GUIManager(void) {}

    RoR::GUIManager::GuiTheme::GuiTheme(void) {}


    RoR::Console* RoR::GUIManager::GetConsole() { return nullptr; }
    void RoR::GUIManager::PushNotification(Ogre::String Title, Ogre::UTFString text) {}
    void RoR::GUIManager::AddRigLoadingReport(std::string const & vehicle_name, std::string const & text, int num_errors, int num_warnings, int num_other) {}
    void RoR::GUIManager::windowResized(class Ogre::RenderWindow *)       {}
    bool RoR::GUIManager::frameStarted(struct Ogre::FrameEvent const &)   {return false;}
    bool RoR::GUIManager::frameEnded(struct Ogre::FrameEvent const &)     {return false;}
    void RoR::GUIManager::windowClosed(class Ogre::RenderWindow *)        {}

    void RoR::GUIManager::SetVisible_GameMainMenu        (bool v) {}
    void RoR::GUIManager::SetVisible_GameAbout           (bool v) {}
    void RoR::GUIManager::SetVisible_DebugOptions        (bool v) {}
    void RoR::GUIManager::SetVisible_MultiplayerSelector (bool v) {}
    void RoR::GUIManager::SetVisible_ChatBox             (bool v) {}
    void RoR::GUIManager::SetVisible_SpawnerReport       (bool v) {}
    void RoR::GUIManager::SetVisible_VehicleDescription  (bool v) {}
    void RoR::GUIManager::SetVisible_MpClientList        (bool v) {}
    void RoR::GUIManager::SetVisible_FrictionSettings    (bool v) {}
    void RoR::GUIManager::SetVisible_TextureToolWindow   (bool v) {}
    void RoR::GUIManager::SetVisible_LoadingWindow       (bool v) {}
    void RoR::GUIManager::SetVisible_Console             (bool v) {}
    void RoR::GUIManager::SetVisible_GameSettings        (bool v) {}
    void RoR::GUIManager::SetVisible_NodeBeamUtils       (bool v) {}

    bool RoR::GUIManager::IsVisible_GameMainMenu         () { return false; }
    bool RoR::GUIManager::IsVisible_GameAbout            () { return false; }
    bool RoR::GUIManager::IsVisible_DebugOptions         () { return false; }
    bool RoR::GUIManager::IsVisible_MultiplayerSelector  () { return false; }
    bool RoR::GUIManager::IsVisible_MainSelector         () { return false; }
    bool RoR::GUIManager::IsVisible_ChatBox              () { return false; }
    bool RoR::GUIManager::IsVisible_SpawnerReport        () { return false; }
    bool RoR::GUIManager::IsVisible_VehicleDescription   () { return false; }
    bool RoR::GUIManager::IsVisible_MpClientList         () { return false; }
    bool RoR::GUIManager::IsVisible_FrictionSettings     () { return false; }
    bool RoR::GUIManager::IsVisible_TextureToolWindow    () { return false; }
    bool RoR::GUIManager::IsVisible_LoadingWindow        () { return false; }
    bool RoR::GUIManager::IsVisible_Console              () { return false; }
    bool RoR::GUIManager::IsVisible_GameSettings         () { return false; }
    bool RoR::GUIManager::IsVisible_TopMenubar           () { return false; }
    bool RoR::GUIManager::IsVisible_NodeBeamUtils        () { return false; }
    class RoR::GUI::MainSelector * RoR::GUIManager::GetMainSelector(void) {return nullptr;}
    class RoR::GUI::LoadingWindow * RoR::GUIManager::GetLoadingWindow(void) {return nullptr;}
    class RoR::GUI::MpClientList * RoR::GUIManager::GetMpClientList(void) {return nullptr;}
    class RoR::GUI::FrictionSettings * RoR::GUIManager::GetFrictionSettings(void) {return nullptr;}
    class RoR::GUI::SimUtils * RoR::GUIManager::GetSimUtils(void) {return nullptr;}
    void RoR::GUIManager::ShowMessageBox(char const *,char const *,bool,char const *,char const *) {}
    void RoR::GUIManager::HideNotification(void) {}
    void RoR::GUIManager::UpdateSimUtils(float,class Actor *) {}
    void RoR::GUIManager::NewImGuiFrame(float) {}
    void RoR::GUIManager::DrawSimulationGui(float) {}
    void RoR::GUIManager::hideGUI(bool) {}
    void RoR::GUIManager::FrictionSettingsUpdateCollisions(void) {}
    void RoR::GUIManager::SetMouseCursorVisibility(enum RoR::GUIManager::MouseCursorVisibility) {}
#endif // ROR_FAKES_IMPL

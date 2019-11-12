/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

/// @file
/// @author Moncef Ben Slimane
/// @date   12/2014

//  Notice:
//  This GUI is a little bit different from the others, so don't take as example.
//  This GUI class is a set of small independent info boxes. The main widget is invisible and un-clickable.

#include "GUI_SimUtils.h"

#include <MyGUI.h>
#include <OgreRenderTarget.h>
#include <OgreRenderWindow.h>
#include <OgreRoot.h>

#include "RoRPrerequisites.h"

#include "Utils.h"
#include "RoRVersion.h"
#include "Language.h"
#include "GUIManager.h"
#include "Application.h"
#include "OgreImGui.h"
#include "OgreSubsystem.h"
#include "Beam.h"

#include "AeroEngine.h"
#include "BeamEngine.h"
#include "ScrewProp.h"

using namespace RoR;
using namespace GUI;

#define CLASS        SimUtils
#define MAIN_WIDGET  ((MyGUI::Window*)mMainWidget)

CLASS::CLASS()
{
    //Usefull
    MainThemeColor = U("#FF7D02");
    WhiteColor = U("#FFFFFF");
    RedColor = U("#DF2121");
    BlueColor = U("#3399DD");

    MAIN_WIDGET->setUserString("interactive", "0");
    MAIN_WIDGET->setPosition(0, 0);

    MyGUI::IntSize screenSize = MyGUI::RenderManager::getInstance().getViewSize();
    MAIN_WIDGET->setSize(screenSize);

    m_actor_stats_str = "";

    m_fps_box_visible = false;

    m_last_notifi_push_time = 0.0f;
    m_notifi_box_alpha = 1.0f;

    m_notifications_disabled = false;
}

CLASS::~CLASS()
{
    this->SetBaseVisible(false);
}

void CLASS::SetBaseVisible(bool v)
{
    if (!v)
    {
        this->SetFPSBoxVisible(false);
        this->HideNotificationBox();
    }
    MAIN_WIDGET->setVisible(v);
}

bool CLASS::IsBaseVisible()
{
    return MAIN_WIDGET->getVisible();
}

void CLASS::SetFPSBoxVisible(bool v)
{
    m_fps_box_visible = v;
    m_fpscounter_box->setVisible(v);
}

void CLASS::HideNotificationBox()
{
    m_notification->setVisible(false);
}

void CLASS::DisableNotifications(bool disabled)
{
    m_notifications_disabled = disabled;
}

void CLASS::FrameStepSimGui(float dt)
{
    if (!MAIN_WIDGET->getVisible())
        return;

    unsigned long ot = Ogre::Root::getSingleton().getTimer()->getMilliseconds();
    if (m_notification->getVisible())
    {
        unsigned long endTime = m_last_notifi_push_time + 5000;
        unsigned long startTime = endTime - (long)1000.0f;
        if (ot < startTime)
        {
            m_notifi_box_alpha = 1.0f;
        }
        else
        {
            m_notifi_box_alpha = 1 - ((ot - startTime) / 1000.0f);
        }

        m_notification->setAlpha(m_notifi_box_alpha);

        if (m_notifi_box_alpha <= 0.1)
            m_notification->setVisible(false);
    }
}

void CLASS::UpdateStats(float dt, Actor* actor)
{
    if (!MAIN_WIDGET->getVisible())
        return;

    if (m_fps_box_visible)
    {
        const Ogre::RenderTarget::FrameStats& stats = App::GetOgreSubsystem()->GetRenderWindow()->getStatistics();
        m_cur_fps->setCaptionWithReplacing(_L("Current FPS: ") + TOUTFSTRING(stats.lastFPS));
        m_avg_fps->setCaptionWithReplacing(_L("Average FPS: ") + TOUTFSTRING(stats.avgFPS));
        m_worst_fps->setCaptionWithReplacing(_L("Worst FPS: ") + TOUTFSTRING(stats.worstFPS));
        m_best_fps->setCaptionWithReplacing(_L("Best FPS: ") + TOUTFSTRING(stats.bestFPS));
        m_triangle_count->setCaptionWithReplacing(_L("Triangle count: ") + TOUTFSTRING(stats.triangleCount));
        m_batch_count->setCaptionWithReplacing(_L("Batch count: ") + TOUTFSTRING(stats.batchCount));
    }
    else
        m_fpscounter_box->setVisible(false);
}

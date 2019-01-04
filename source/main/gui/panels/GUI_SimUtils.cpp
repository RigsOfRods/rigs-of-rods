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
    m_actor_info_visible = false;

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
        this->SetActorInfoBoxVisible(false);
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

void CLASS::SetActorInfoBoxVisible(bool v)
{
    m_actor_info_visible = v;
    m_truckinfo_box->setVisible(v);
}

void CLASS::PushNotification(Ogre::String Title, Ogre::String text)
{
    if (!MAIN_WIDGET->getVisible())
        return;
    if (m_notifications_disabled)
        return;

    m_not_title->setCaption(Title);
    m_not_text->setCaption(text);
    m_notification->setVisible(true);
    m_last_notifi_push_time = Ogre::Root::getSingleton().getTimer()->getMilliseconds();
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

    if (m_actor_info_visible && actor != nullptr)
    {
        if (!m_truckinfo_box->getVisible())
            m_truckinfo_box->setVisible(true);

        m_truck_name->setMaxTextLength(28);
        m_truck_name->setCaptionWithReplacing(actor->GetActorDesignName());
        m_actor_stats_str = "\n"; //always reset on each frame + space

        //taken from TruckHUD.cpp (now removed), TODO: needs cleanup
        beam_t* beam = actor->ar_beams;
        float average_deformation = 0.0f;
        float beamstress = 0.0f;
        float current_deformation = 0.0f;
        float mass = actor->getTotalMass();
        int beambroken = 0;
        int beamdeformed = 0;

        for (int i = 0; i < actor->ar_num_beams; i++ , beam++)
        {
            if (beam->bm_broken != 0)
            {
                beambroken++;
            }
            beamstress += beam->stress;
            current_deformation = fabs(beam->L - beam->refL);
            if (fabs(current_deformation) > 0.0001f && beam->bm_type != BEAM_HYDRO)
            {
                beamdeformed++;
            }
            average_deformation += current_deformation;
        }

        float health = ((float)beambroken / (float)actor->ar_num_beams) * 10.0f + ((float)beamdeformed / (float)actor->ar_num_beams);
        if (health < 1.0f)
        {
            m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("Vehicle health: ") + WhiteColor + TOUTFSTRING(Round((1.0f - health) * 100.0f, 2)) + U("%") + "\n";
        }
        else if (health >= 1.0f)
        {
            //When this condition is true, it means that health is at 0% which means 100% of destruction.
            m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("Vehicle destruction: ") + WhiteColor + U("100%") + "\n";
        }

        m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("Beam count: ") + WhiteColor + TOUTFSTRING(actor->ar_num_beams) + "\n";
        m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("Broken beams count: ") + WhiteColor + TOUTFSTRING(beambroken) + U(" (") + TOUTFSTRING(Round((float)beambroken / (float)actor->ar_num_beams, 2) * 100.0f) + U("%)") + "\n";
        m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("Deformed beams count: ") + WhiteColor + TOUTFSTRING(beamdeformed) + U(" (") + TOUTFSTRING(Round((float)beamdeformed / (float)actor->ar_num_beams, 2) * 100.0f) + U("%)") + "\n";
        m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("Average deformation: ") + WhiteColor + TOUTFSTRING(Round((float)average_deformation / (float)actor->ar_num_beams, 4) * 100.0f) + "\n";

        //Taken from TruckHUD.cpp ..
        wchar_t beamstressstr[256];
        swprintf(beamstressstr, 256, L"%+08.0f", 1 - (float)beamstress / (float)actor->ar_num_beams);
        m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("Average stress: ") + WhiteColor + Ogre::UTFString(beamstressstr) + "\n";

        m_actor_stats_str = m_actor_stats_str + "\n"; //Some space

        int wcount = actor->getWheelNodeCount();
        wchar_t nodecountstr[256];
        swprintf(nodecountstr, 256, L"%d (wheels: %d)", actor->ar_num_nodes, wcount);
        m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("Node count: ") + WhiteColor + Ogre::UTFString(nodecountstr) + "\n";

        wchar_t truckmassstr[256];
        swprintf(truckmassstr, 256, L"%8.2f kg (%.2f tons)", mass, mass / 1000.0f);
        m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("Total mass: ") + WhiteColor + Ogre::UTFString(truckmassstr) + "\n";

        m_actor_stats_str = m_actor_stats_str + "\n"; //Some space

        if (actor->ar_driveable == TRUCK && actor->ar_engine)
        {
            if (actor->ar_engine->GetEngineRpm() > actor->ar_engine->getMaxRPM())
                m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("Engine RPM: ") + RedColor + TOUTFSTRING(Round(actor->ar_engine->GetEngineRpm())) + U(" / ") + TOUTFSTRING(Round(actor->ar_engine->getMaxRPM())) + "\n";
            else
                m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("Engine RPM: ") + WhiteColor + TOUTFSTRING(Round(actor->ar_engine->GetEngineRpm())) + U(" / ") + TOUTFSTRING(Round(actor->ar_engine->getMaxRPM())) + "\n";

            m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("Input shaft RPM: ") + WhiteColor + TOUTFSTRING(Round(std::max(0.0f, actor->ar_engine->GetInputShaftRpm()))) + "\n\n";

            float currentTorque = actor->ar_engine->GetEngineTorque();

            m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("Current torque: ") + WhiteColor + TOUTFSTRING(Round(currentTorque)) + U(" Nm") + "\n";

            float currentKw = currentTorque * Ogre::Math::TWO_PI * (actor->ar_engine->GetEngineRpm() / 60.0f) / 1000.0f;

            m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("Current power: ") + WhiteColor + TOUTFSTRING(Round(currentKw / 0.73549875f)) + U(" hp / ") + TOUTFSTRING(Round(currentKw)) + U(" Kw") + "\n\n";

            m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("Current gear: ") + WhiteColor + TOUTFSTRING(Round(actor->ar_engine->GetGear())) + "\n";

            m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("Drive ratio: ") + WhiteColor + TOUTFSTRING(Round(actor->ar_engine->GetDriveRatio())) + ":1\n\n";

            float velocityKMH = actor->ar_wheel_speed * 3.6f;
            float velocityMPH = actor->ar_wheel_speed * 2.23693629f;
            float carSpeedKPH = actor->ar_nodes[0].Velocity.length() * 3.6f;
            float carSpeedMPH = actor->ar_nodes[0].Velocity.length() * 2.23693629f;

            // apply a deadzone ==> no flickering +/-
            if (fabs(velocityMPH) < 0.5f)
            {
                velocityKMH = velocityMPH = 0.0f;
            }
            if (fabs(carSpeedMPH) < 0.5f)
            {
                carSpeedKPH = carSpeedMPH = 0.0f;
            }

            Ogre::UTFString wsmsg = _L("Wheel speed: ");
            //Some kind of wheel skidding detection? lol
            if (velocityKMH > carSpeedKPH * 1.25f)
                m_actor_stats_str = m_actor_stats_str + MainThemeColor + wsmsg + RedColor + TOUTFSTRING(Round(velocityKMH)) + U(" km/h (") + TOUTFSTRING(Round(velocityMPH)) + U(" mph)") + "\n";
            else if (velocityKMH < carSpeedKPH / 1.25f)
                m_actor_stats_str = m_actor_stats_str + MainThemeColor + wsmsg + BlueColor + TOUTFSTRING(Round(velocityKMH)) + U(" km/h (") + TOUTFSTRING(Round(velocityMPH)) + U(" mph)") + "\n";
            else
                m_actor_stats_str = m_actor_stats_str + MainThemeColor + wsmsg + WhiteColor + TOUTFSTRING(Round(velocityKMH)) + U(" km/h (") + TOUTFSTRING(Round(velocityMPH)) + U(" mph)") + "\n";

            m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("Vehicle speed: ") + WhiteColor + TOUTFSTRING(Round(carSpeedKPH)) + U(" km/h (") + TOUTFSTRING(Round(carSpeedMPH)) + U(" mph)") + "\n";
        }
        else
        {
            float speedKN = actor->ar_nodes[0].Velocity.length() * 1.94384449f;
            m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("Current speed: ") + WhiteColor + TOUTFSTRING(Round(speedKN)) + U(" kn (") + TOUTFSTRING(Round(speedKN * 1.852)) + U(" km/h) (") + TOUTFSTRING(Round(speedKN * 1.151)) + U(" mph)") + "\n";

            Ogre::UTFString engmsg = _L("Engine ");
            if (actor->ar_driveable == AIRPLANE)
            {
                float altitude = actor->ar_nodes[0].AbsPosition.y / 30.48 * 100;
                m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("Altitude: ") + WhiteColor + TOUTFSTRING(Round(altitude)) + U(" feet (") + TOUTFSTRING(Round(altitude * 0.30480)) + U(" meters)") + "\n";
                for (int i = 0; i < 8; i++)
                {
                    if (actor->ar_aeroengines[i] && actor->ar_aeroengines[i]->getType() == AeroEngine::AEROENGINE_TYPE_TURBOJET)
                        m_actor_stats_str = m_actor_stats_str + MainThemeColor + engmsg + TOUTFSTRING(i + 1 /*not to start with 0, players wont like it i guess*/) + " : " + WhiteColor + TOUTFSTRING(Round(actor->ar_aeroengines[i]->getRPM())) + "%" + "\n";
                    else if (actor->ar_aeroengines[i] && actor->ar_aeroengines[i]->getType() == AeroEngine::AEROENGINE_TYPE_TURBOPROP)
                        m_actor_stats_str = m_actor_stats_str + MainThemeColor + engmsg + TOUTFSTRING(i + 1 /*not to start with 0, players wont like it i guess*/) + " : " + WhiteColor + TOUTFSTRING(Round(actor->ar_aeroengines[i]->getRPM())) + " RPM" + "\n";
                }
            }
            else if (actor->ar_driveable == BOAT)
            {
                for (int i = 0; i < 8; i++)
                {
                    if (actor->ar_screwprops[i])
                        m_actor_stats_str = m_actor_stats_str + MainThemeColor + engmsg + TOUTFSTRING(i + 1 /*not to start with 0, players wont like it i guess*/) + " : " + WhiteColor + TOUTFSTRING(Round(actor->ar_screwprops[i]->getThrottle() *100 )) + "%" + "\n";
                }
            }
        }

        m_actor_stats_str = m_actor_stats_str + "\n"; //Some space

        float speedKPH = actor->ar_top_speed * 3.6f;
        float speedMPH = actor->ar_top_speed * 2.23693629f;
        m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("Top speed: ") + WhiteColor + TOUTFSTRING(Round(speedKPH)) + U(" km/h (") + TOUTFSTRING(Round(speedMPH)) + U(" mph)") + "\n";

        //Is this really usefull? people really use it?
        m_actor_stats_str = m_actor_stats_str + "\n"; //Some space

        wchar_t geesstr[256];
        Ogre::Vector3 gees = actor->getGForces();
        Ogre::UTFString tmp = _L("Vertical: % 1.2fg\nSagittal: % 1.2fg\nLateral:  % 1.2fg");
        swprintf(geesstr, 256, tmp.asWStr_c_str(), gees.x, gees.y, gees.z);
        m_actor_stats_str = m_actor_stats_str + MainThemeColor + _L("G-Forces:\n") + WhiteColor + Ogre::UTFString(geesstr) + "\n";

        m_truck_stats->setCaptionWithReplacing(m_actor_stats_str);
    }
    else
        m_truckinfo_box->setVisible(false);
}

/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

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

/** 
	@file   GUI_SimUtils.cpp
	@author Moncef Ben Slimane
	@date   12/2014
*/

/*
	Notice:
	This GUI is a little bit different from the others, so don't take as example.
*/
#include "GUI_SimUtils.h"

#include "RoRPrerequisites.h"
#include "Utils.h"
#include "RoRVersion.h"
#include "Language.h"
#include "GUIManager.h"
#include "Application.h"
#include "OgreSubsystem.h"
#include "OgreRenderWindow.h"
#include "Beam.h"

#include "AeroEngine.h"
#include "BeamEngine.h"
#include "ScrewProp.h"

#include <MyGUI.h>
#include <OgreRenderTarget.h>
#include <OgreRoot.h>

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

	truckstats = "";

	b_fpsbox = false;
	b_truckinfo = false;

	alpha = 1.0f;

	ShowMain(); //It's invisible and unclickable, so no worrys
}

CLASS::~CLASS()
{
	HideMain();
	//delete(MAIN_WIDGET);
}

void CLASS::ShowMain()
{
	//Kinda of initialization
	MAIN_WIDGET->setVisible(true);
}

void CLASS::HideMain()
{
	MAIN_WIDGET->setVisible(false);
}

void CLASS::ToggleFPSBox()
{
	b_fpsbox = !b_fpsbox;
	m_fpscounter_box->setVisible(b_fpsbox);
}

void CLASS::ToggleTruckInfoBox()
{
	b_truckinfo = !b_truckinfo;
	m_truckinfo_box->setVisible(b_truckinfo);
}

void CLASS::PushNotification(Ogre::String Title, Ogre::String text)
{
	if (!MAIN_WIDGET->getVisible()) return;

	m_not_title->setCaption(Title);
	m_not_text->setCaption(text);
	m_notification->setVisible(true);
	pushTime = Ogre::Root::getSingleton().getTimer()->getMilliseconds();
}

void CLASS::framestep(float dt)
{
	if (!MAIN_WIDGET->getVisible()) return;

	unsigned long ot = Ogre::Root::getSingleton().getTimer()->getMilliseconds();
	if (m_notification->getVisible())
	{
		unsigned long endTime = pushTime + 5000;
		unsigned long startTime = endTime - (long)1000.0f;
		if (ot < startTime)
		{
			alpha = 1.0f;
		}
		else
		{
			alpha = 1 - ((ot - startTime) / 1000.0f);
		}

		m_notification->setAlpha(alpha);

		if (alpha <= 0.1)
			m_notification->setVisible(false);
	}
}

void CLASS::UpdateStats(float dt, Beam *truck)
{
	if (!MAIN_WIDGET->getVisible()) return;

	if (b_fpsbox)
	{
		const Ogre::RenderTarget::FrameStats& stats = Application::GetOgreSubsystem()->GetRenderWindow()->getStatistics();
		m_cur_fps->setCaptionWithReplacing("Current FPS: " + Ogre::StringConverter::toString(stats.lastFPS));
		m_avg_fps->setCaptionWithReplacing("Average FPS: " + Ogre::StringConverter::toString(stats.avgFPS));
		m_worst_fps->setCaptionWithReplacing("Worst FPS: " + Ogre::StringConverter::toString(stats.worstFPS));
		m_best_fps->setCaptionWithReplacing("Best FPS: " + Ogre::StringConverter::toString(stats.bestFPS));
		m_triangle_count->setCaptionWithReplacing("Triangle count: " + Ogre::StringConverter::toString(stats.triangleCount));
		m_batch_count->setCaptionWithReplacing("Batch count: " + Ogre::StringConverter::toString(stats.batchCount));
	}
	else
		m_fpscounter_box->setVisible(false);

	if (b_truckinfo && truck != nullptr)
	{
		m_truck_name->setCaptionWithReplacing(truck->getTruckName());
		truckstats = "\n"; //always reset on each frame + space

		//taken from TruckHUD.cpp, needs cleanup
		beam_t *beam = truck->getBeams();
		float average_deformation = 0.0f;
		float beamstress = 0.0f;
		float current_deformation = 0.0f;
		float mass = truck->getTotalMass();
		int beamCount = truck->getBeamCount();
		int beambroken = 0;
		int beamdeformed = 0;

		for (int i = 0; i < beamCount; i++, beam++)
		{
			if (beam->broken != 0)
			{
				beambroken++;
			}
			beamstress += beam->stress;
			current_deformation = fabs(beam->L - beam->refL);
			if (fabs(current_deformation) > 0.0001f && beam->type != BEAM_HYDRO && beam->type != BEAM_INVISIBLE_HYDRO)
			{
				beamdeformed++;
			}
			average_deformation += current_deformation;
		}


		float health = ((float)beambroken / (float)beamCount) * 10.0f + ((float)beamdeformed / (float)beamCount);
		if (health < 1.0f)
		{
			truckstats = truckstats + MainThemeColor + "Vehicle's health: " + WhiteColor + TOUTFSTRING(Round((1.0f - health) * 100.0f, 2)) + U("%") + "\n";
		}
		else if (health >= 1.0f)
		{
			//When this condition is true, it means that health is at 0% which means 100% of destruction.
			truckstats = truckstats + MainThemeColor + "Vehicle's destruction: " + WhiteColor + U("100%") + "\n";
		}

		truckstats = truckstats + MainThemeColor + "Beam count: " + WhiteColor + TOUTFSTRING(beamCount) + "\n";
		truckstats = truckstats + MainThemeColor + "Broken Beams count: " + WhiteColor + TOUTFSTRING(beambroken) + U(" (") + TOUTFSTRING(Round((float)beambroken / (float)beamCount, 2) * 100.0f) + U("%)") + "\n";
		truckstats = truckstats + MainThemeColor + "Deformed Beams count: " + WhiteColor + TOUTFSTRING(beamdeformed) + U(" (") + TOUTFSTRING(Round((float)beamdeformed / (float)beamCount, 2) * 100.0f) + U("%)") + "\n";
		truckstats = truckstats + MainThemeColor + "Average Deformation: " + WhiteColor + TOUTFSTRING(Round((float)average_deformation / (float)beamCount, 4) * 100.0f) + "\n";

		//Taken from TruckHUD.cpp ..
		wchar_t beamstressstr[256];
		swprintf(beamstressstr, 256, L"%+08.0f", 1 - (float)beamstress / (float)beamCount);
		truckstats = truckstats + MainThemeColor + "Average Stress: " + WhiteColor + Ogre::UTFString(beamstressstr) + "\n";

		truckstats = truckstats + "\n"; //Some space

		int ncount = truck->getNodeCount();
		int wcount = truck->getWheelNodeCount();
		wchar_t nodecountstr[256];
		swprintf(nodecountstr, 256, L"%d (wheels: %d)", ncount, wcount);
		truckstats = truckstats + MainThemeColor + "Node count: " + WhiteColor + Ogre::UTFString(nodecountstr) + "\n";

		wchar_t truckmassstr[256];
		Ogre::UTFString massstr;
		swprintf(truckmassstr, 256, L"%ls %8.2f kg (%.2f tons)", massstr.asWStr_c_str(), mass, mass / 1000.0f);
		truckstats = truckstats + MainThemeColor + "Total mass: " + WhiteColor + Ogre::UTFString(truckmassstr) + "\n";

		truckstats = truckstats + "\n"; //Some space

		if (truck->driveable == TRUCK && truck->engine)
		{
			if (truck->engine->getRPM() > truck->engine->getMaxRPM())
				truckstats = truckstats + MainThemeColor + "Engine RPM: " + RedColor + TOUTFSTRING(Round(truck->engine->getRPM())) + U(" / ") + TOUTFSTRING(Round(truck->engine->getMaxRPM())) + "\n";
			else
				truckstats = truckstats + MainThemeColor + "Engine RPM: " + WhiteColor + TOUTFSTRING(Round(truck->engine->getRPM())) + U(" / ") + TOUTFSTRING(Round(truck->engine->getMaxRPM())) + "\n";

			float currentKw = (((truck->engine->getRPM() * truck->engine->getEngineTorque() *(3.14159265358979323846 /* pi.. */ / 30)) / 1000));

			truckstats = truckstats + MainThemeColor + "Current Power: " + WhiteColor + TOUTFSTRING(Round(currentKw *1.34102209)) + U(" hp / ") + TOUTFSTRING(Round(currentKw)) + U(" Kw") + "\n";

			float velocityKMH = truck->WheelSpeed* 3.6f;
			float velocityMPH = truck->WheelSpeed * 2.23693629f;
			float carSpeedKPH = truck->nodes[0].Velocity.length() * 3.6f;
			float carSpeedMPH = truck->nodes[0].Velocity.length() * 2.23693629f;

			// apply a deadzone ==> no flickering +/-
			if (fabs(truck->WheelSpeed) < 1.0f)
			{
				velocityKMH = velocityMPH = 0.0f;
			}
			if (fabs(truck->nodes[0].Velocity.length()) < 1.0f)
			{
				carSpeedKPH = carSpeedMPH = 0.0f;
			}

			//Some kind of wheel skidding detection? lol
			if (Round(velocityKMH, 0.1) > Round(carSpeedKPH, 0.1) + 2)
				truckstats = truckstats + MainThemeColor + "Wheel speed: " + RedColor + TOUTFSTRING(Round(velocityKMH)) + U(" km/h (") + TOUTFSTRING(Round(velocityMPH)) + U(" mph)") + "\n";
			else if (Round(velocityKMH, 0.1) < Round(carSpeedKPH, 0.1) - 2)
				truckstats = truckstats + MainThemeColor + "Wheel speed: " + BlueColor + TOUTFSTRING(Round(velocityKMH)) + U(" km/h (") + TOUTFSTRING(Round(velocityMPH)) + U(" mph)") + "\n";
			else
				truckstats = truckstats + MainThemeColor + "Wheel speed: " + WhiteColor + TOUTFSTRING(Round(velocityKMH)) + U(" km/h (") + TOUTFSTRING(Round(velocityMPH)) + U(" mph)") + "\n";

			truckstats = truckstats + MainThemeColor + "Car speed: " + WhiteColor + TOUTFSTRING(Round(carSpeedKPH)) + U(" km/h (") + TOUTFSTRING(Round(carSpeedMPH)) + U(" mph)") + "\n";
		}
		else 
		{
			float speedKN = truck->nodes[0].Velocity.length() * 1.94384449f;
			truckstats = truckstats + MainThemeColor + "Current Speed: " + WhiteColor + TOUTFSTRING(Round(speedKN)) + U(" kn (") + TOUTFSTRING(Round(speedKN * 1.852)) + U(" km/h) (") + TOUTFSTRING(Round(speedKN * 1.151)) + U(" mph)") + "\n";
			float altitude = truck->nodes[0].AbsPosition.y / 30.48 * 100;

			truckstats = truckstats + MainThemeColor + "Altitude: " + WhiteColor + TOUTFSTRING(Round(altitude)) + U(" feet (") + TOUTFSTRING(Round(altitude * 0.30480)) + U(" meters)") + "\n";

			if (truck->driveable == AIRPLANE)
			{
				for (int i = 0; i < 8; i++)
				{
					if (truck->aeroengines[i] && truck->aeroengines[i]->getType() == AeroEngine::AEROENGINE_TYPE_TURBOJET)
						truckstats = truckstats + MainThemeColor + "Engine " + TOUTFSTRING(i + 1 /*not to start with 0, players wont like it i guess*/) + " : " + WhiteColor + TOUTFSTRING(Round(truck->aeroengines[i]->getRPM())) + "%" + "\n";
					else if (truck->aeroengines[i] && truck->aeroengines[i]->getType() == AeroEngine::AEROENGINE_TYPE_TURBOPROP)
						truckstats = truckstats + MainThemeColor + "Engine " + TOUTFSTRING(i + 1 /*not to start with 0, players wont like it i guess*/) + " : " + WhiteColor + TOUTFSTRING(Round(truck->aeroengines[i]->getRPM())) + " RPM" + "\n";
				}
			}
			else if(truck->driveable == BOAT)
			{
				for (int i = 0; i < 8; i++)
				{
					if (truck->screwprops[i])
						truckstats = truckstats + MainThemeColor + "Engine " + TOUTFSTRING(i + 1 /*not to start with 0, players wont like it i guess*/) + " : " + WhiteColor + TOUTFSTRING(Round(truck->screwprops[i]->getThrottle() *100 )) + "%" + "\n";
				}
			}
				
		}

		/*
		truckstats = truckstats + MainThemeColor + "maxG: " + WhiteColor + Ogre::UTFString(geesstr) + "\n";
		*/

		//Is this really usefull? people really use it?
		truckstats = truckstats + "\n"; //Some space

		wchar_t geesstr[256];
		Ogre::Vector3 gees = truck->getGForces();
		// apply deadzones ==> no flickering +/-
		if (fabs(gees.y) < 0.01) gees.y = 0.0f;
		if (fabs(gees.z) < 0.01) gees.z = 0.0f;
		Ogre::UTFString tmp = _L("Vertical: %1.2fg | Saggital: %1.2fg | Lateral: %1.2fg");
		swprintf(geesstr, 256, tmp.asWStr_c_str(), gees.x, gees.y, gees.z);
		truckstats = truckstats + MainThemeColor + "Gees: " + WhiteColor + Ogre::UTFString(geesstr) + "\n";

		if (truck->driveable == TRUCK || truck->driveable == AIRPLANE || truck->driveable == BOAT)
		{
			if (gees.x > maxPosVerG[truck->driveable])
				maxPosVerG[truck->driveable] = gees.x;
			if (gees.x < maxNegVerG[truck->driveable])
				maxNegVerG[truck->driveable] = gees.x;

			if (gees.y > maxPosSagG[truck->driveable])
				maxPosSagG[truck->driveable] = gees.y;
			if (gees.y < maxNegSagG[truck->driveable])
				maxNegSagG[truck->driveable] = gees.y;

			if (gees.z > maxPosLatG[truck->driveable])
				maxPosLatG[truck->driveable] = gees.z;
			if (gees.z < maxNegLatG[truck->driveable])
				maxNegLatG[truck->driveable] = gees.z;

			tmp = _L("V %1.2fg %1.2fg // S %1.2fg %1.2fg // L %1.2fg %1.2fg");
			swprintf(geesstr, 256, tmp.asWStr_c_str(),
				maxPosVerG[truck->driveable],
				maxNegVerG[truck->driveable],
				maxPosSagG[truck->driveable],
				maxNegSagG[truck->driveable],
				maxPosLatG[truck->driveable],
				maxNegLatG[truck->driveable]
				);
			truckstats = truckstats + MainThemeColor + "maxG: " + WhiteColor + Ogre::UTFString(geesstr) + "\n";
		}

		m_truck_stats->setCaptionWithReplacing(truckstats);
	}
	else 
		m_truckinfo_box->setVisible(false);

}
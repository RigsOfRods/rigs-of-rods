/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "TruckHUD.h"

#include "AeroEngine.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "InputEngine.h"
#include "Language.h"
#include "Utils.h"

using namespace Ogre;

TruckHUD::TruckHUD() :
	  border(0)	
	, updatetime(0.0f)
	, width(450)
{
	// init counters
	for (int i=NOT_DRIVEABLE; i < AI; i++)
	{
		maxVelos[i] = maxPosVerG[i] = maxPosSagG[i] = maxPosLatG[i] = -9999;
		minVelos[i] = maxNegVerG[i] = maxNegSagG[i] = maxNegLatG[i] = 9999;
		avVelos[i] = 0;
	}

	truckHUD = OverlayManager::getSingleton().getByName("tracks/TruckInfoBox");
}

void TruckHUD::show(bool value)
{
	if ( value ) {
		truckHUD->show();
	} else {
		truckHUD->hide();
	}
}

bool TruckHUD::isVisible()
{
	return truckHUD->isVisible();
}

void TruckHUD::checkOverflow(OverlayElement* e)
{
	int newval = e->getLeft() + e->getWidth() + border;

	if (newval > this->width)
	{
		this->width = newval;
		OverlayElement *panel = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/MainPanel");
		panel->setWidth(width);
	}
}

bool TruckHUD::update(float dt, Beam *truck, bool visible)
{
	OverlayElement *overlayElement = 0;

	// only update every 300 ms
	if (visible)
	{
		updatetime -= dt;
		if (updatetime <= 0.0f)
		{
			// update now, reset timer
			updatetime = 0.3f;
		} else
		{
			// don't update visuals, only count stats
			visible = false;
		}
	}

	if (visible)
	{
		overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/Truckname");
		overlayElement->setCaption(truck->getTruckName());
		checkOverflow(overlayElement);

		overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/Truckauthor");
		overlayElement->setCaption(_L("(no author information available)"));
		std::vector<authorinfo_t> file_authors = truck->getAuthors();
		if (!file_authors.empty())
		{
			String authors = "";
			for (std::vector<authorinfo_t>::iterator it = file_authors.begin(); it != file_authors.end(); ++it)
			{
				authors += (*it).name + " ";
			}
			overlayElement->setCaption(_L("Authors: ") + authors);
		}
		checkOverflow(overlayElement);

		std::vector<std::string> description = truck->getDescription();
		for (unsigned int i=1; i < 3; i++)
		{
			overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/DescriptionLine" + TOSTRING(i+1));
			overlayElement->setCaption("");
			if (i < description.size())
			{
				overlayElement->setCaption(ANSI_TO_UTF(description[i]));
			}
			checkOverflow(overlayElement);
		}

		beam_t *beam = truck->getBeams();
		float average_deformation = 0.0f;
		float beamstress = 0.0f;
		float current_deformation = 0.0f;
		float mass = truck->getTotalMass();
		int beamCount = truck->getBeamCount();
		int beambroken = 0;
		int beamdeformed = 0;

		for (int i=0; i < beamCount; i++, beam++)
		{
			if (beam->broken != 0)
			{
				beambroken++;
			}
			beamstress += beam->stress;
			current_deformation = fabs(beam->L-beam->refL);
			if (fabs(current_deformation) > 0.0001f && beam->type != BEAM_HYDRO && beam->type != BEAM_INVISIBLE_HYDRO)
			{
				beamdeformed++;
			}
			average_deformation += current_deformation;
		}

		overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/BeamTotal");
		overlayElement->setCaption(_L("beam count: ") + TOUTFSTRING(beamCount));
		checkOverflow(overlayElement);

		overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/BeamBroken");
		overlayElement->setCaption(_L("broken: ") + TOUTFSTRING(beambroken) + U(" (") + TOUTFSTRING(Round((float)beambroken / (float)beamCount, 2) * 100.0f) + U("%)"));
		checkOverflow(overlayElement);

		overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/BeamHealth");
		float health = ((float)beambroken / (float)beamCount) * 10.0f + ((float)beamdeformed / (float)beamCount);
		if (health < 1.0f)
		{
			overlayElement->setCaption(_L("health: ") + TOUTFSTRING(Round((1.0f - health) * 100.0f, 2)) + U("%"));
			overlayElement->setColour(ColourValue(0.6f, 0.8f, 0.4f, 1.0f));
		} else if (health >= 1.0f)
		{
			health = ((float)beambroken / (float)beamCount) * 3.0f;
			health = std::min(health, 1.0f);
			overlayElement->setCaption(_L("destruction: ") + TOUTFSTRING(Round(health * 100.0f, 2)) + U("%"));
			overlayElement->setColour(ColourValue(0.8f, 0.4f, 0.4f, 1.0f));
		}
		checkOverflow(overlayElement);


		overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/BeamDeformed");
		overlayElement->setCaption(_L("deformed: ") + TOUTFSTRING(beamdeformed) + U(" (") + TOUTFSTRING(Round((float)beamdeformed / (float)beamCount, 2) * 100.0f) + U("%)"));
		checkOverflow(overlayElement);

		overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/AverageBeamDeformation");
		overlayElement->setCaption(_L("average deformation: ") + TOUTFSTRING(Round((float)average_deformation / (float)beamCount, 4) * 100.0f));
		checkOverflow(overlayElement);
		
		overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/AverageBeamStress");
		wchar_t beamstressstr[256];
		swprintf(beamstressstr, 256, L"%+08.0f", 1-(float)beamstress/(float)beamCount);
		overlayElement->setCaption(_L("average stress: ") + UTFString(beamstressstr));
		checkOverflow(overlayElement);

		overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/NodeCount");
		int ncount = truck->getNodeCount();
		int wcount =  truck->getWheelNodeCount();
		wchar_t nodecountstr[256];
		swprintf(nodecountstr, 256, L"%d (wheels: %d)", ncount, wcount);
		overlayElement->setCaption(_L("node count: ") + UTFString(nodecountstr));
		checkOverflow(overlayElement);

		overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/TruckWeight");
		wchar_t truckmassstr[256];
		UTFString massstr = _L("current mass:");
		swprintf(truckmassstr, 256, L"%ls %8.2f kg (%.2f tons)", massstr.asWStr_c_str(), mass, mass / 1000.0f);
		overlayElement->setCaption(UTFString(truckmassstr));
		checkOverflow(overlayElement);

		overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/Gees");
		wchar_t geesstr[256];
		Vector3 gees = truck->getGForces();
		// apply deadzones ==> no flickering +/-
		if (fabs(gees.y) < 0.01) gees.y = 0.0f;
		if (fabs(gees.z) < 0.01) gees.z = 0.0f;
		UTFString tmp = _L("Gees: Vertical %1.2fg // Saggital %1.2fg // Lateral %1.2fg");
		swprintf(geesstr, 256, tmp.asWStr_c_str(), gees.x, gees.y, gees.z);
		overlayElement->setCaption(UTFString(geesstr));
		checkOverflow(overlayElement);

		// max g-forces
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

			tmp = _L("maxG: V %1.2fg %1.2fg // S %1.2fg %1.2fg // L %1.2fg %1.2fg");
			swprintf(geesstr, 256, tmp.asWStr_c_str(),
					maxPosVerG[truck->driveable],
					maxNegVerG[truck->driveable],
					maxPosSagG[truck->driveable],
					maxNegSagG[truck->driveable],
					maxPosLatG[truck->driveable],
					maxNegLatG[truck->driveable]
				);

			overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/Gees2");
			overlayElement->setCaption(UTFString(geesstr));
			checkOverflow(overlayElement);
		}
	}

	Vector3 hdir = Vector3::ZERO;
	if (truck->cameranodepos[0] >= 0 && truck->cameranodepos[0] < MAX_NODES)
	{
		hdir = (truck->nodes[truck->cameranodepos[0]].RelPosition - truck->nodes[truck->cameranodedir[0]].RelPosition).normalisedCopy();
	}
	float g_along_hdir = hdir.dotProduct(truck->ffforce / 10000.0f);

	// always update these statistics, also if not visible!
	overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CurrentRPM");
	overlayElement->setCaption("");
	UTFString rpmsstr = _L("current RPM:");
	if (truck->driveable == TRUCK && truck->engine)
	{
		overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CurrentRPM");
		overlayElement->setCaption(rpmsstr + U(" ") + TOUTFSTRING(Round(truck->engine->getRPM())) + U(" / ") + TOUTFSTRING(Round(truck->engine->getMaxRPM())));
	} else if (truck->driveable == AIRPLANE)
	{
		for (int i=0; i < 8; i++)
		{
			if (truck->aeroengines[i])
			{
				rpmsstr = rpmsstr + U(" / ") + TOUTFSTRING(Round(truck->aeroengines[i]->getRPM()));
			}
		}
		overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CurrentRPM");
		overlayElement->setCaption(UTFString(rpmsstr));
	}
	checkOverflow(overlayElement);

	UTFString cspeedstr   = _L("current Speed:") + U(" ");
	UTFString mspeedstr   = _L("max Speed:")     + U(" ");
	UTFString altitudestr = _L("Altitude:")      + U(" ");

	if (truck->driveable == TRUCK)
	{
		maxVelos[truck->driveable] = std::max(maxVelos[truck->driveable], truck->WheelSpeed);
		minVelos[truck->driveable] = std::min(truck->WheelSpeed, minVelos[truck->driveable]);

		overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CurrentVelocity");
		
		float velocityKMH = truck->WheelSpeed* 3.6f;
		float velocityMPH = truck->WheelSpeed * 2.23693629f;
		// apply a deadzone ==> no flickering +/-
		if (fabs(truck->WheelSpeed) < 1.0f)
		{
			velocityKMH = velocityMPH = 0.0f;
		}
		overlayElement->setCaption(cspeedstr + TOUTFSTRING(Round(velocityKMH)) + U(" km/h (") + TOUTFSTRING(Round(velocityMPH)) + U(" mph)"));
		checkOverflow(overlayElement);

		overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/MaxVelocity");
		float velocityMaxKMH = maxVelos[truck->driveable] * 3.6f;
		float velocityMinKMH = minVelos[truck->driveable] * 3.6f;
		float velocityMaxMPH = maxVelos[truck->driveable] * 2.23693629f;
		float velocityMinMPH = minVelos[truck->driveable] * 2.23693629f;
		overlayElement->setCaption(mspeedstr + TOUTFSTRING(Round(velocityMaxKMH)) + U("km/h (") + TOUTFSTRING(Round(velocityMaxMPH)) + U("mph)") + U(", min: ") + TOUTFSTRING(Round(velocityMinKMH)) + U("km/h (") + TOUTFSTRING(Round(velocityMinMPH)) + U("mph)"));
		checkOverflow(overlayElement);

		overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/AverageVelocity");
		overlayElement->setCaption("");
	} else if (truck->driveable == AIRPLANE || truck->driveable == BOAT)
	{
		float velocity = truck->nodes[0].Velocity.length();
		
		if (truck->cameranodepos[0] >= 0 && truck->cameranodedir[0] && truck->driveable == BOAT)
		{
			hdir = (truck->nodes[truck->cameranodepos[0]].RelPosition - truck->nodes[truck->cameranodedir[0]].RelPosition).normalisedCopy();
			velocity = hdir.dotProduct(truck->nodes[truck->cameranodepos[0]].Velocity);
		}
		
		maxVelos[truck->driveable] = std::max(maxVelos[truck->driveable], velocity);
		minVelos[truck->driveable] = std::min(velocity, minVelos[truck->driveable]);

		overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CurrentVelocity");
		float velocityKN = velocity * 1.94384449f;
		// apply a deadzone ==> no flickering +/-
		if (fabs(velocity) < 1.0f)
		{
			velocityKN = 0.0f;
		}
		overlayElement->setCaption(cspeedstr + TOUTFSTRING(Round(velocityKN)) + U(" kn"));
		checkOverflow(overlayElement);

		overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/MaxVelocity");
		float velocityMaxKN = maxVelos[truck->driveable] * 1.94384449f;
		float velocityMinKN = minVelos[truck->driveable] * 1.94384449f;
		overlayElement->setCaption(mspeedstr + TOUTFSTRING(Round(maxVelos[truck->driveable])) + U(" kn, min: ") + TOUTFSTRING(Round(minVelos[truck->driveable])) + U(" kn"));
		checkOverflow(overlayElement);

		overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/AverageVelocity");
		overlayElement->setCaption("");
		if (truck->driveable == AIRPLANE)
		{
			float altitude = truck->nodes[0].AbsPosition.y * 1.1811f;
			overlayElement->setCaption(altitudestr + TOUTFSTRING(Round(altitude)) + U(" m"));
			checkOverflow(overlayElement);
		}
	} else if (truck->driveable == MACHINE)
	{
		OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/MaxVelocity")->setCaption("");
		OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CurrentVelocity")->setCaption("");
		OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/AverageVelocity")->setCaption("");
	}

	OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/TimeStats")->setCaption("");

	if (visible)
	{
		// update commands

		// clear them first
		for (int i=1; i <= COMMANDS_VISIBLE; i++)
		{
			overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/Command" + TOSTRING(i));
			overlayElement->setCaption("");
		}

		int filledCommands = 0;
		for (int i=1; i < MAX_COMMANDS && filledCommands < COMMANDS_VISIBLE; i += 2)
		{
			if(truck->commandkey[i].beams.empty() || truck->commandkey[i].description == "hide") continue;

			filledCommands++;
			char commandID[256] = {};
			String keyStr = "";

			sprintf(commandID, "COMMANDS_%02d", i);
			int eventID = INPUTENGINE.resolveEventName(String(commandID));
			String keya = INPUTENGINE.getEventCommand(eventID);
			sprintf(commandID, "COMMANDS_%02d", i+1);
			eventID = INPUTENGINE.resolveEventName(String(commandID));
			String keyb = INPUTENGINE.getEventCommand(eventID);

			// cut off expl
			if (keya.size() > 6 && keya.substr(0,5) == "EXPL+") keya = keya.substr(5);
			if (keyb.size() > 6 && keyb.substr(0,5) == "EXPL+") keyb = keyb.substr(5);

			keyStr = keya + "/" + keyb;

			overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/Command" + TOSTRING(filledCommands));
			
			if (truck->commandkey[i].description.empty())
			{
				overlayElement->setCaption(keyStr + ": " + _L("unknown function"));
			} else
			{
				overlayElement->setCaption(keyStr + ": " + truck->commandkey[i].description);
			}
			checkOverflow(overlayElement);
		}

		// hide command section title if no commands
		overlayElement = overlayElement = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CommandsTitleLabel");
		overlayElement->setCaption("");
		if (filledCommands > 0)
		{
			// TODO: Fix the position of this overlay element
			//overlayElement->setCaption(_L("Commands:"));
			checkOverflow(overlayElement);
		}
	}
	return true;
}

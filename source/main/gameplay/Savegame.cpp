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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 4th of September 2011

#include "Savegame.h"

#include "Beam.h"
#include "BeamFactory.h"
#include "CameraManager.h"
#include "Character.h"
#include "Console.h"
#include "Language.h"
#include "RoRFrameListener.h"
#include "RoRVersion.h"

using namespace Ogre;

const char *Savegame::current_version = "ROR_SAVEGAME_v2";

#define WRITEVAR(x)    fwrite(&x, sizeof(x), 1, f)
#define WRITEARR(x, y) for (int n = 0; n < y; n++) { WRITEVAR(x); }

#define READVAR(x)     fread(&x, sizeof(x), 1, f)
#define READARR(x,y)   for (int n = 0; n < y; n++) { READVAR(x); }

int Savegame::save(Ogre::String &filename)
{
	LOG("trying to save savegame as " + filename + " ...");
	FILE *f = fopen(filename.c_str(), "wb");

	// wait for engine sync?

	// TODO: show error
	if (!f)
	{
		LOG("error opening savegame");
#ifdef USE_MYGUI
		Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, _L("error opening savegame"), "error.png");
#endif // USE_MYGUI
		return 1;
	}

	int current_truck = BeamFactory::getSingleton().getCurrentTruckNumber();
	int free_truck    = BeamFactory::getSingleton().getTruckCount();
	Beam **trucks     = BeamFactory::getSingleton().getTrucks();

	// now construct the file, header first
	{
		struct savegame_header h;
		memset(&h, 0, sizeof(h));
		strcpy(h.ror_version, ROR_VERSION_STRING);
		strcpy(h.savegame_version, current_version);
		h.entries       = free_truck;
		h.current_truck = current_truck;

		{
			// and generic things like character and camera
			if (gEnv->player)
			{
				Vector3 pos = gEnv->player->getPosition();
				// WARNING: breaks if Real == double!
				memcpy(&h.player_pos, pos.ptr(), sizeof(float) * 3);
			}
			if (gEnv->mainCamera)
			{
				Vector3 pos = gEnv->mainCamera->getPosition();
				// WARNING: breaks if Real == double!
				memcpy(&h.cam_pos, pos.ptr(), sizeof(float) * 3);
			}
			
			// TODO: FIX savegame camera integration
			//h.camRotX = gEnv->frameListener->camRotX.valueRadians();
			//h.camRotY = gEnv->frameListener->camRotY.valueRadians();
			//h.camDist = gEnv->frameListener->camDist;


			//memcpy(&h.cam_ideal_pos, gEnv->frameListener->camIdealPosition.ptr(), sizeof(float) * 3);

			//h.pushcamRotX = gEnv->frameListener->pushcamRotX.valueRadians();
			//h.pushcamRotY = gEnv->frameListener->pushcamRotY.valueRadians();
			//h.mMoveScale  = gEnv->frameListener->mMoveScale;
			//h.mRotScale   = gEnv->frameListener->mRotScale.valueRadians();

			//memcpy(&h.lastPosition, gEnv->frameListener->lastPosition.ptr(), sizeof(float) * 3);

			//h.cameramode     = gEnv->frameListener->cameramode;
			//h.lastcameramode = gEnv->frameListener->lastcameramode;
		}

		// write header to file
		fwrite(&h, sizeof(h), 1, f);
	}

	// iterate the trucks
	for (int i = 0; i < free_truck; i++)
	{
		Beam *t = trucks[i];
		if (!t) continue;

		// construct entry header
		{
			struct savegame_entry_header dh;
			memset(&dh, 0, sizeof(dh));
			dh.magic = entry_magic; // conversion from 'const int' to 'unsigned int', signed/unsigned mismatch!
			dh.free_nodes = t->free_node;
			dh.free_beams = t->free_beam;
			dh.free_shock = t->free_shock;
			dh.free_wheel = t->free_wheel;
			//dh.free_hooks = t->free_hooks;
			dh.free_rotator = t->free_rotator;
			strcpy(dh.filename, t->realtruckfilename.c_str());
			memcpy(&dh.origin, t->origin.ptr(), sizeof(float) * 3);

			if (t->engine) dh.engine = 1;

			dh.collisionBoundingBoxeses = t->collisionBoundingBoxes.size();

			// write entry header to file
			fwrite(&dh, sizeof(dh), 1, f);
		}

		// and put the data
		{
			WRITEARR(t->nodes[n], t->free_node);
			WRITEARR(t->beams[n], t->free_beam);
			WRITEARR(t->shocks[n], t->free_shock);
			WRITEARR(t->wheels[n], t->free_wheel);
			//WRITEARR(t->hooks[n], t->hooks.size());
			//WRITEARR(t->ropes[n], t->ropes.size());
			//WRITEARR(t->ropables[n], t->ropables.size());
			//WRITEARR(t->ties[n], t->ties.size());
			//WRITEARR(t->rigidifiers[n], t->free_rigidifier);
			//WRITEARR(t->wings[n], t->free_wing);
			WRITEARR(t->rotators[n], t->free_rotator);
			//WRITEARR(t->flares[n], t->free_flare);
			//WRITEARR(t->cparticles[n], t->free_cparticle);
			//WRITEARR(t->pressure_beams[n], t->free_pressure_beam);
			//WRITEARR(t->aeroengines[n], t->free_aeroengine);
			//WRITEARR(t->screwprops[n], t->free_screwprop);
			// WRITEARR(t->hydro[n], t->free_hydro); // saved in beam
			//WRITEARR(t->airbrakes[n], t->free_airbrake);

			// now some single variables
			WRITEVAR(t->slopeBrake);
			WRITEVAR(t->slopeBrakeFactor);
			WRITEVAR(t->slopeBrakeAttAngle);
			WRITEVAR(t->previousCrank);
			WRITEVAR(t->alb_ratio);
			WRITEVAR(t->alb_minspeed);
			WRITEVAR(t->alb_mode);
			WRITEVAR(t->alb_pulse);
			WRITEVAR(t->alb_pulse_state);
			WRITEVAR(t->alb_present);
			WRITEVAR(t->alb_notoggle);
			WRITEVAR(t->tc_ratio);
			WRITEVAR(t->tc_wheelslip);
			WRITEVAR(t->tc_fade);
			WRITEVAR(t->tc_mode);
			WRITEVAR(t->tc_pulse);
			WRITEVAR(t->tc_pulse_state);
			WRITEVAR(t->tc_present);
			WRITEVAR(t->tc_notoggle);
			WRITEVAR(t->tcalb_timer);
			WRITEVAR(t->antilockbrake);
			WRITEVAR(t->tractioncontrol);
			WRITEVAR(t->animTimer);
			WRITEVAR(t->beam_creak);
			WRITEVAR(t->enable_wheel2);
			WRITEVAR(t->truckmass);
			WRITEVAR(t->loadmass);
			WRITEVAR(t->flaresMode);
			WRITEVAR(t->state);
			WRITEVAR(t->cparticle_enabled);
			WRITEVAR(t->origin);
			WRITEVAR(t->boundingBox.getMinimum().x);
			WRITEVAR(t->boundingBox.getMinimum().y);
			WRITEVAR(t->boundingBox.getMinimum().z);
			WRITEVAR(t->boundingBox.getMaximum().x);
			WRITEVAR(t->boundingBox.getMaximum().y);
			WRITEVAR(t->boundingBox.getMaximum().z);

			// bounding boxes
			for(unsigned int i=0; i < t->collisionBoundingBoxes.size(); i++)
			{
				WRITEVAR(t->collisionBoundingBoxes[i].getMinimum().x);
				WRITEVAR(t->collisionBoundingBoxes[i].getMinimum().y);
				WRITEVAR(t->collisionBoundingBoxes[i].getMinimum().z);

				WRITEVAR(t->collisionBoundingBoxes[i].getMaximum().x);
				WRITEVAR(t->collisionBoundingBoxes[i].getMaximum().y);
				WRITEVAR(t->collisionBoundingBoxes[i].getMaximum().z);
			}

			// TODO: commandkey, Skin, engine, slidenodes, flexbodies (damage texture)
		}
	}
	// and we are done :)
	LOG("saving done");
#ifdef USE_MYGUI
	Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("saving done"), "information.png");
#endif // USE_MYGUI

	fclose(f);
	return 0;
}

int Savegame::load(Ogre::String &filename)
{
	LOG("trying to load savegame " + filename + " ...");
	FILE *f = fopen(filename.c_str(), "rb");

	// wait for engine sync
	//BEAMLOCK();

	if (!f)
	{
		LOG("error opening savegame");
#ifdef USE_MYGUI
		Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, _L("error opening savegame"), "error.png");
#endif // USE_MYGUI
		return 1;
	}

	//int current_truck = BeamFactory::getSingleton().getCurrentTruckNumber();
	int free_truck    = BeamFactory::getSingleton().getTruckCount();
	Beam **trucks     = BeamFactory::getSingleton().getTrucks();

	// now read the file, header first
	struct savegame_header h;
	{
		fread(&h, sizeof(h), 1, f);
		if (strcmp(h.savegame_version, current_version))
		{
			String errstr = _L("unknown savegame version: ") + String(h.savegame_version) + _L(" supported version: ") + String(current_version);
			LOG(errstr);
#ifdef USE_MYGUI
			Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, errstr, "error.png");
#endif // USE_MYGUI
			fclose(f);
			return 1;
		}
	}

	// restore generic things: characer and camera
	{
		// and generic things like character and camera
		if (gEnv->player)
		{
			gEnv->player->setPosition(Vector3(h.player_pos));
		}

		// TODO: FIX savegame camera integration
		/*
		if (gEnv->frameListener->getCamera())
		{
			gEnv->frameListener->getCamera()->setPosition(Vector3(h.cam_pos));
		}

		gEnv->frameListener->camRotX = Radian(h.camRotX);
		gEnv->frameListener->camRotY = Radian(h.camRotY);
		gEnv->frameListener->camDist = h.camDist;

		gEnv->frameListener->camIdealPosition = Vector3(h.cam_ideal_pos);

		gEnv->frameListener->pushcamRotX = Radian(h.pushcamRotX);
		gEnv->frameListener->pushcamRotY = Radian(h.pushcamRotY);
		gEnv->frameListener->mMoveScale  = h.mMoveScale;
		gEnv->frameListener->mRotScale   = Radian(h.mRotScale);

		gEnv->frameListener->lastPosition = Vector3(h.lastPosition);

		gEnv->frameListener->cameramode = h.cameramode;
		gEnv->frameListener->lastcameramode = h.lastcameramode;
		*/
	}

	// iterate the trucks
	for (unsigned int i = 0; i < h.entries; i++)
	{
		struct savegame_entry_header dh;
		fread(&dh, sizeof(dh), 1, f);

		// check magic first, to prevent reading corrupt data
		if (dh.magic != entry_magic)
		{
			LOG(_L("savegame corrupted: ") + filename);
#ifdef USE_MYGUI
			Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, _L("savegame corrupted: ") + filename, "error.png");
#endif // USE_MYGUI
			fclose(f);
			return 1;
		}

		// restore its origin
		Vector3 origin = Vector3(dh.origin);

		// now iterate all existing trucks and look if we can reuse one
		Beam *t = NULL;
		for (int tn = 0; tn < free_truck; tn++)
		{
			if (!trucks[tn]) continue;
			if (trucks[tn]->realtruckfilename == String(dh.filename))
			{
				// found, set it!
				t = trucks[tn];
				break;
			}
		}

		// no spawned truck found, create a new one
		if (!t)
		{
			t = BeamFactory::getSingleton().createLocal(origin, Quaternion::ZERO, dh.filename);
		}

		// still a problem?
		// ignore this vehicle then
		if (!t)
		{
			LOG("error loading vehicle from savegame: " + String(dh.filename));
			continue;
		}

		// now load the data from the file into the vehicle

		// and the nodes and beams
		if (t->free_node != dh.free_nodes)
		{
			LOG("wrong node number for truck " + String(dh.filename) + " expected " + TOSTRING(dh.free_nodes) + " nodes, but vehicle has " + TOSTRING(t->free_node) + " nodes. Will not load that vehicle.");
			continue;
		}
		for (int n = 0; n < t->free_node; n++)
		{
			// save some pointers
			SceneNode *tmp = t->nodes[n].mSceneNode;

			// load from file into memory
			fread(&t->nodes[n], sizeof(node_t), 1, f);

			// and restore the pointers
			t->nodes[n].mSceneNode = tmp;
		}

		if (t->free_beam != dh.free_beams)
		{
			LOG("wrong beam number for truck " + String(dh.filename) + " expected " + TOSTRING(dh.free_nodes) + " beams, but vehicle has " + TOSTRING(t->free_node) + " beams. Will not load that vehicle.");
			continue;
		}
		for (int n = 0; n < t->free_beam; n++)
		{
			// save some pointers
			beam_t tmp;
			tmp.p1         = t->beams[n].p1;
			tmp.p2         = t->beams[n].p2;
			tmp.p2truck    = t->beams[n].p2truck;
			tmp.shock      = t->beams[n].shock;
			tmp.mSceneNode = t->beams[n].mSceneNode;
			tmp.mEntity    = t->beams[n].mEntity;

			// load from file into memory
			fread(&t->beams[n], sizeof(beam_t), 1, f);

			// and restore the pointers
			t->beams[n].p1         = tmp.p1;
			t->beams[n].p2         = tmp.p2;
			t->beams[n].p2truck    = tmp.p2truck;
			t->beams[n].shock      = tmp.shock;
			t->beams[n].mSceneNode = tmp.mSceneNode;
			t->beams[n].mEntity    = tmp.mEntity;
		}

		if (t->free_shock != dh.free_shock)
		{
			LOG("wrong shock number for truck " + String(dh.filename) + " expected " + TOSTRING(dh.free_shock) + " shocks, but vehicle has " + TOSTRING(t->free_shock) + " shocks. Will not load that vehicle.");
			continue;
		}
		READARR(t->shocks[n], t->free_shock);

		if (t->free_wheel != dh.free_wheel)
		{
			LOG("wrong wheel number for truck " + String(dh.filename) + " expected " + TOSTRING(dh.free_wheel) + " wheels, but vehicle has " + TOSTRING(t->free_wheel) + " wheels. Will not load that vehicle.");
			continue;
		}
		for (int n = 0; n < t->free_wheel; n++)
		{
			wheel_t tmp;
			fread(&tmp, sizeof(tmp), 1, f);
			// copy only some
			t->wheels[n].speed = tmp.speed;
			t->wheels[n].delta_rotation = tmp.delta_rotation;
		}
		

		/*
		if (t->hooks.size() != dh.free_hooks)
		{
			LOG("wrong hook number for truck " + String(dh.filename) + " expected " + TOSTRING(dh.free_hooks) + " hooks, but vehicle has " + TOSTRING(t->hooks.size()) + " hooks. Will not load that vehicle.");
			continue;
		}
		for (int n = 0; n < t->hooks.size(); n++)
		{
			hook_t tmp;
			fread(&tmp, sizeof(tmp), 1, f);
			//TODO
		}
		*/

		if (t->free_rotator != dh.free_rotator)
		{
			LOG("wrong rotator number for truck " + String(dh.filename) + " expected " + TOSTRING(dh.free_rotator) + " rotators, but vehicle has " + TOSTRING(t->free_rotator) + " rotators. Will not load that vehicle.");
			continue;
		}
		for (int n = 0; n < t->free_rotator; n++)
		{
			rotator_t tmp;
			fread(&tmp, sizeof(tmp), 1, f);
			t->rotators[n].angle = tmp.angle;
			t->rotators[n].rate  = tmp.rate;
			t->rotators[n].force = tmp.force;
			t->rotators[n].tolerance = tmp.tolerance;
		}

		// now the vars
		READVAR(t->slopeBrake);
		READVAR(t->slopeBrakeFactor);
		READVAR(t->slopeBrakeAttAngle);
		READVAR(t->previousCrank);
		READVAR(t->alb_ratio);
		READVAR(t->alb_minspeed);
		READVAR(t->alb_mode);
		READVAR(t->alb_pulse);
		READVAR(t->alb_pulse_state);
		READVAR(t->alb_present);
		READVAR(t->alb_notoggle);
		READVAR(t->tc_ratio);
		READVAR(t->tc_wheelslip);
		READVAR(t->tc_fade);
		READVAR(t->tc_mode);
		READVAR(t->tc_pulse);
		READVAR(t->tc_pulse_state);
		READVAR(t->tc_present);
		READVAR(t->tc_notoggle);
		READVAR(t->tcalb_timer);
		READVAR(t->antilockbrake);
		READVAR(t->tractioncontrol);
		READVAR(t->animTimer);
		READVAR(t->beam_creak);
		READVAR(t->enable_wheel2);
		READVAR(t->truckmass);
		READVAR(t->loadmass);
		READVAR(t->flaresMode);
		READVAR(t->state);
		READVAR(t->cparticle_enabled);
		READVAR(t->origin);
		READVAR(t->boundingBox.getMinimum().x);
		READVAR(t->boundingBox.getMinimum().y);
		READVAR(t->boundingBox.getMinimum().z);
		READVAR(t->boundingBox.getMaximum().x);
		READVAR(t->boundingBox.getMaximum().y);
		READVAR(t->boundingBox.getMaximum().z);

		// bounding boxes
		t->collisionBoundingBoxes.clear();
		t->collisionBoundingBoxes.resize(dh.collisionBoundingBoxeses);
		for(unsigned int i=0; i < dh.collisionBoundingBoxeses; i++)
		{
			READVAR(t->collisionBoundingBoxes[i].getMinimum().x);
			READVAR(t->collisionBoundingBoxes[i].getMinimum().y);
			READVAR(t->collisionBoundingBoxes[i].getMinimum().z);

			READVAR(t->collisionBoundingBoxes[i].getMaximum().x);
			READVAR(t->collisionBoundingBoxes[i].getMaximum().y);
			READVAR(t->collisionBoundingBoxes[i].getMaximum().z);
		}

		// important: active all vehicles upon loading!
		// they will go sleeping automatically
		if (t->state > DESACTIVATED)
			t->state = DESACTIVATED; // desactivated = active but not leading
	}

	// try to set the current truck
	BeamFactory::getSingleton().setCurrentTruck(h.current_truck);

	// and we are done :)
	LOG("loading done.");
#ifdef USE_MYGUI
	Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("loading done."), "error.png");
#endif // USE_MYGUI
	fclose(f);
	return 0;
}

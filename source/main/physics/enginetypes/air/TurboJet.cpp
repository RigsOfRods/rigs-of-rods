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
#include "TurboJet.h"

#include "BeamData.h"
#include "MaterialFunctionMapper.h"
#include "MaterialReplacer.h"
#include "Ogre.h"
#include "Skin.h"
#include "SoundScriptManager.h"

using namespace Ogre;

Turbojet::Turbojet(char* propname, int tnumber, int trucknum, node_t *nd, int tnodefront, int tnodeback, int tnoderef, float tmaxdrythrust, bool treversable, bool tafterburnable, float tafterburnthrust, float diskdiam, float nozdiam, float nozlength, bool disable_smoke, bool _heathaze, MaterialFunctionMapper *mfm, Skin *usedSkin, MaterialReplacer *mr) : mr(mr)
{
	heathaze=_heathaze;
	nodes=nd;
	number=tnumber;
	this->trucknum=trucknum;
#ifdef USE_OPENAL
	switch (number)
	{
	case 0: mod_id=SS_MOD_AEROENGINE1;src_id=SS_TRIG_AEROENGINE1;thr_id=SS_MOD_THROTTLE1;ab_id=SS_TRIG_AFTERBURNER1;break;
	case 1: mod_id=SS_MOD_AEROENGINE2;src_id=SS_TRIG_AEROENGINE2;thr_id=SS_MOD_THROTTLE2;ab_id=SS_TRIG_AFTERBURNER2;break;
	case 2: mod_id=SS_MOD_AEROENGINE3;src_id=SS_TRIG_AEROENGINE3;thr_id=SS_MOD_THROTTLE3;ab_id=SS_TRIG_AFTERBURNER3;break;
	case 3: mod_id=SS_MOD_AEROENGINE4;src_id=SS_TRIG_AEROENGINE4;thr_id=SS_MOD_THROTTLE4;ab_id=SS_TRIG_AFTERBURNER4;break;
	case 4: mod_id=SS_MOD_AEROENGINE5;src_id=SS_TRIG_AEROENGINE5;thr_id=SS_MOD_THROTTLE5;ab_id=SS_TRIG_AFTERBURNER5;break;
	case 5: mod_id=SS_MOD_AEROENGINE6;src_id=SS_TRIG_AEROENGINE6;thr_id=SS_MOD_THROTTLE6;ab_id=SS_TRIG_AFTERBURNER6;break;
	case 6: mod_id=SS_MOD_AEROENGINE7;src_id=SS_TRIG_AEROENGINE7;thr_id=SS_MOD_THROTTLE7;ab_id=SS_TRIG_AFTERBURNER7;break;
	case 7: mod_id=SS_MOD_AEROENGINE8;src_id=SS_TRIG_AEROENGINE8;thr_id=SS_MOD_THROTTLE8;ab_id=SS_TRIG_AFTERBURNER8;break;
	default: mod_id=SS_MOD_NONE;src_id=SS_TRIG_NONE;thr_id=SS_MOD_NONE;ab_id=SS_TRIG_NONE;
	}
#endif
	nodeback=tnodeback; nodes[nodeback].iIsSkin=true;
	nodefront=tnodefront; nodes[nodefront].iIsSkin=true;
	noderef=tnoderef;
	afterburnable=tafterburnable;
	reversable=treversable;
	maxdrythrust=tmaxdrythrust;
	afterburnthrust=tafterburnthrust;
	afterburner=false;
	timer=0;
	warmuptime=15.0;
	lastflip=0;
	radius=nozdiam/2.0;
	area=2*3.14159*radius*0.6*radius*0.6;
	exhaust_velocity=0;
	axis=nodes[nodefront].RelPosition-nodes[nodeback].RelPosition;
	reflen=axis.length();
	axis=axis/reflen;
	reset();
	//setup visuals
	char paname[256];
	sprintf(paname, "%s-nozzle", propname);
	Entity *te = gEnv->sceneManager->createEntity(paname, "nozzle.mesh");
	MaterialFunctionMapper::replaceSimpleMeshMaterials(te, ColourValue(1, 0.5, 0.5));
	if (mfm) mfm->replaceMeshMaterials(te);
	if (mr) mr->replaceMeshMaterials(te);
	if (usedSkin) usedSkin->replaceMeshMaterials(te);
	nzsnode=gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
	nzsnode->attachObject(te);
	nzsnode->setScale(nozlength, nozdiam, nozdiam);
	if (afterburnable)
	{
		sprintf(paname, "%s-abflame", propname);
		te = gEnv->sceneManager->createEntity(paname, "abflame.mesh");
		MaterialFunctionMapper::replaceSimpleMeshMaterials(te, ColourValue(1, 1, 0));
		if (mfm) mfm->replaceMeshMaterials(te);
		if (mr) mr->replaceMeshMaterials(te);
		if (usedSkin) usedSkin->replaceMeshMaterials(te);
		absnode=gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
		absnode->attachObject(te);
		absnode->setScale(1.0, nozdiam, nozdiam);
		absnode->setVisible(false);
	}
	//smoke visual
	if (disable_smoke)
	{
		smokeNode=0;
		smokePS=0;
	}
	else
	{
		sprintf(paname,"%s-smoke", propname);
		smokeNode=gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
		smokePS=gEnv->sceneManager->createParticleSystem(paname, "tracks/TurbopropSmoke");
		if (smokePS)
		{
			smokePS->setVisibilityFlags(DEPTHMAP_DISABLED); // disable particles in depthmap
			smokeNode->attachObject(smokePS);
			smokePS->setCastShadows(false);
		}

		heathazePS=0;
		if (heathaze)
		{
			sprintf(paname,"%s-smoke-heat", propname);
			heathazePS=gEnv->sceneManager->createParticleSystem(paname, "tracks/JetHeatHaze");
			smokeNode->attachObject(heathazePS);
			heathazePS->setCastShadows(false);
			heathazePS->setVisibilityFlags(DEPTHMAP_DISABLED); // disable particles in depthmap
		}
	}
}

void Turbojet::updateVisuals()
{
	//nozzle
	nzsnode->setPosition(nodes[nodeback].smoothpos);
	//build a local system
	Vector3 laxis=nodes[nodefront].RelPosition-nodes[nodeback].RelPosition;
	laxis.normalise();
	Vector3 paxis=Plane(laxis, 0).projectVector(nodes[noderef].RelPosition-nodes[nodeback].RelPosition);
	paxis.normalise();
	Vector3 taxis=laxis.crossProduct(paxis);
	Quaternion dir=Quaternion(laxis, paxis, taxis);
	nzsnode->setOrientation(dir);
	//afterburner
	if (afterburner)
	{
		absnode->setVisible(true);
		float flamelength=(afterburnthrust/15.0)*(rpm/100.0);
		flamelength=flamelength*(1.0+(((Real)rand()/(Real)RAND_MAX)-0.5)/10.0);
		absnode->setScale(flamelength, radius*2.0, radius*2.0);
		absnode->setPosition(nodes[nodeback].smoothpos+dir*Vector3(-0.2, 0.0, 0.0));
		absnode->setOrientation(dir);
	}
	else absnode->setVisible(false);
	//smoke
	if (smokeNode)
	{
		smokeNode->setPosition(nodes[nodeback].smoothpos);
		ParticleEmitter *emit=smokePS->getEmitter(0);
		ParticleEmitter *hemit=0;
		if (heathazePS)
			hemit=heathazePS->getEmitter(0);
		emit->setDirection(-axis);
		emit->setParticleVelocity(exhaust_velocity);
		if (hemit)
		{
			hemit->setDirection(-axis);
			hemit->setParticleVelocity(exhaust_velocity);
		}
		if (!failed)
		{
			if (ignition)
			{
				emit->setEnabled(true);
				emit->setColour(ColourValue(0.0,0.0,0.0,0.02+throtle*0.03));
				emit->setTimeToLive((0.02+throtle*0.03)/0.1);
				if (hemit)
				{
					hemit->setEnabled(true);
					hemit->setTimeToLive((0.02+throtle*0.03)/0.1);
				}
			}
			else
			{
				emit->setEnabled(false);
				if (hemit)
					hemit->setEnabled(false);
			}
		}
		else
		{
			emit->setDirection(Vector3(0,1,0));
			emit->setParticleVelocity(7.0);
			emit->setEnabled(true);
			emit->setColour(ColourValue(0.0,0.0,0.0,0.1));
			emit->setTimeToLive(0.1/0.1);
			
			if (hemit)
			{
				hemit->setDirection(Vector3(0,1,0));
				hemit->setParticleVelocity(7.0);
				hemit->setEnabled(true);
				hemit->setTimeToLive(0.1/0.1);
			}
		}
	}
}

void Turbojet::updateForces(float dt, int doUpdate)
{
	if (doUpdate)
	{
#ifdef USE_OPENAL
		//sound update
		SoundScriptManager::getSingleton().modulate(trucknum, mod_id, rpm);
#endif //OPENAL
	}
	timer+=dt;
	axis=nodes[nodefront].RelPosition-nodes[nodeback].RelPosition;
	float axlen=axis.length();
	axis=axis/axlen; //normalize
	if (fabs(reflen-axlen)>0.1) {rpm=0;failed=true;}; //check for broken

	float warmupfactor=1.0;
	if (warmup)
	{
		warmupfactor=(timer-warmupstart)/warmuptime;
		if (warmupfactor>=1.0) warmup=false;
	}

	//turbine RPM
	//braking (compression)
	if (rpm<0) rpm=0;
	float torque=-rpm/100.0;
	//powering with limiter
	if (rpm<100.0 && !failed && ignition) torque+=((0.2+throtle*0.8)*warmupfactor);
	//integration
	rpm+=(double)dt*torque*30.0;

	float enginethrust=0;
	if (!failed && ignition)
	{
		enginethrust=maxdrythrust*rpm/100.0;
		afterburner=(afterburnable && throtle>0.95 && rpm>80);
		if (afterburner) enginethrust+=(afterburnthrust-maxdrythrust);
	} else afterburner=false;
#ifdef USE_OPENAL
	if (afterburner)
		SoundScriptManager::getSingleton().trigStart(trucknum, ab_id);
	else
		SoundScriptManager::getSingleton().trigStop(trucknum, ab_id);
#endif //OPENAL

	nodes[nodeback].Forces+=(enginethrust*1000.0)*axis;
	exhaust_velocity=enginethrust*5.6/area;
}

void Turbojet::setThrottle(float val)
{
	if (val>1.0) val=1.0;
	if (val<0.0) val=0.0;
	throtle=val;
#ifdef USE_OPENAL
	//sound update
	SoundScriptManager::getSingleton().modulate(trucknum, thr_id, val);
#endif //OPENAL
}

float Turbojet::getThrottle()
{
	return throtle;
}

void Turbojet::setRPM(float _rpm)
{
	rpm = _rpm;
}

void Turbojet::reset()
{
	rpm=0;
	throtle=0;
	propwash=0;
	failed=false;
	ignition=false;
	reverse=false;
}

void Turbojet::toggleReverse()
{
	if (!reversable) return;
	throtle=0;
	reverse=!reverse;
}

void Turbojet::flipStart()
{
	if (timer-lastflip<0.3) return;
	ignition=!ignition;
	if (ignition && !failed)
	{
		warmup=true;
		warmupstart=timer;
#ifdef USE_OPENAL
		SoundScriptManager::getSingleton().trigStart(trucknum, src_id);
#endif //OPENAL
	}
	else
	{
#ifdef USE_OPENAL
		SoundScriptManager::getSingleton().trigStop(trucknum, src_id);
#endif //OPENAL
	}

	lastflip=timer;
}


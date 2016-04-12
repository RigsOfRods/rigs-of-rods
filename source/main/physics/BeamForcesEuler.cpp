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
#include "RoRPrerequisites.h"

#include "AeroEngine.h"
#include "AirBrake.h"
#include "Airfoil.h"
#include "Application.h"
#include "ApproxMath.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "BeamFactory.h"
#include "BeamStats.h"
#include "Buoyance.h"
#include "CmdKeyInertia.h"
#include "Collisions.h"
#include "Differentials.h"
#include "DustPool.h"
#include "FlexAirfoil.h"
#include "InputEngine.h"
#include "Replay.h"
#include "ScrewProp.h"
#include "SoundScriptManager.h"
#include "Water.h"
#include "TerrainManager.h"

using namespace Ogre;

void Beam::calcForcesEulerCompute(int doUpdate, Real dt, int step, int maxsteps)
{
	IWater *water = 0;
	if (gEnv->terrainManager)
		water = gEnv->terrainManager->getWater();

	increased_accuracy = false;

	//engine callback
	if (engine)
	{
		BES_START(BES_CORE_TruckEngine);
		engine->update(dt, doUpdate);
		BES_STOP(BES_CORE_TruckEngine);
	}
	//if (doUpdate) mWindow->setDebugText(engine->status);

	calcBeams(doUpdate, dt, step, maxsteps);

	if (doUpdate)
	{
		//just call this once per frame to avoid performance impact
		hookToggle(-2, HOOK_LOCK, -1);
	}

	//auto locks (scan just once per frame, need to use a timer(truck-based) to get
	for (std::vector <hook_t>::iterator it = hooks.begin(); it!=hooks.end(); it++)
	{
		//we need to do this here to avoid countdown speedup by triggers
		it->timer = std::max(0.0f, it->timer - dt);
	}

	BES_START(BES_CORE_AnimatedProps);

	//animate props
	// TODO: only calculate animated props every frame and not in the core routine
	for (int propi=0; propi<free_prop; propi++)
	{
		int animnum=0;
		float rx = 0.0f;
		float ry = 0.0f;
		float rz = 0.0f;

		while (props[propi].animFlags[animnum])
		{
			if (props[propi].animFlags[animnum])
			{
				float cstate = 0.0f;
				int div = 0.0f;
				int flagstate = props[propi].animFlags[animnum];
				float animOpt1 = props[propi].animOpt1[animnum];
				float animOpt2 = props[propi].animOpt2[animnum];
				float animOpt3 = props[propi].animOpt3[animnum];

				calcAnimators(flagstate, cstate, div, dt, animOpt1, animOpt2, animOpt3);

				// key triggered animations
				if ((props[propi].animFlags[animnum] & ANIM_FLAG_EVENT) && props[propi].animKey[animnum] != -1)
				{
					if (RoR::Application::GetInputEngine()->getEventValue(props[propi].animKey[animnum]))
					{
						// keystatelock is disabled then set cstate
						if (props[propi].animKeyState[animnum] == -1.0f)
						{
							cstate += RoR::Application::GetInputEngine()->getEventValue(props[propi].animKey[animnum]);
						} else if (!props[propi].animKeyState[animnum])
						{
							// a key was pressed and a toggle was done already, so bypass
							//toggle now
							if (!props[propi].lastanimKS[animnum])
							{
								props[propi].lastanimKS[animnum] = 1.0f;
								// use animkey as bool to determine keypress / release state of inputengine
								props[propi].animKeyState[animnum] = 1.0f;
							}
							else
							{
								props[propi].lastanimKS[animnum] = 0.0f;
								// use animkey as bool to determine keypress / release state of inputengine
								props[propi].animKeyState[animnum] = 1.0f;
							}
						} else
						{
							// bypas mode, get the last set position and set it
							cstate +=props[propi].lastanimKS[animnum];
						}
					} else
					{
						// keyevent exists and keylock is enabled but the key isnt pressed right now = get lastanimkeystatus for cstate and reset keypressed bool animkey
						if (props[propi].animKeyState[animnum] != -1.0f)
						{
							cstate +=props[propi].lastanimKS[animnum];
							props[propi].animKeyState[animnum] = 0.0f;
						}
					}
				}

				//propanimation placed here to avoid interference with existing hydros(cstate) and permanent prop animation
				//truck steering
				if (props[propi].animFlags[animnum] & ANIM_FLAG_STEERING) cstate += hydrodirstate;
				//aileron
				if (props[propi].animFlags[animnum] & ANIM_FLAG_AILERONS) cstate += hydroaileronstate;
				//elevator
				if (props[propi].animFlags[animnum] & ANIM_FLAG_ELEVATORS) cstate += hydroelevatorstate;
				//rudder
				if (props[propi].animFlags[animnum] & ANIM_FLAG_ARUDDER) cstate += hydrorudderstate;
				//permanent
				if (props[propi].animFlags[animnum] & ANIM_FLAG_PERMANENT) cstate += 1.0f;

				cstate *= props[propi].animratio[animnum];

				// autoanimate noflip_bouncer
				if (props[propi].animOpt5[animnum]) cstate *= (props[propi].animOpt5[animnum]);

				//rotate prop
				if ((props[propi].animMode[animnum] & ANIM_MODE_ROTA_X) || (props[propi].animMode[animnum] & ANIM_MODE_ROTA_Y) || (props[propi].animMode[animnum] & ANIM_MODE_ROTA_Z))
				{
					float limiter = 0.0f;
					if (props[propi].animMode[animnum] & ANIM_MODE_AUTOANIMATE)
					{
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_X)
						{
							props[propi].rot = props[propi].rot * (Quaternion(Degree(0), Vector3::UNIT_Z) * Quaternion(Degree(0), Vector3::UNIT_Y) * Quaternion(Degree(cstate), Vector3::UNIT_X));
							props[propi].rotaX += cstate;
							limiter = props[propi].rotaX;
						}
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_Y)
						{
							props[propi].rot = props[propi].rot * (Quaternion(Degree(0), Vector3::UNIT_Z) * Quaternion(Degree(cstate), Vector3::UNIT_Y) * Quaternion(Degree(0), Vector3::UNIT_X));
							props[propi].rotaY += cstate;
							limiter = props[propi].rotaY;
						}
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_Z)
						{
							props[propi].rot = props[propi].rot * (Quaternion(Degree(cstate), Vector3::UNIT_Z) * Quaternion(Degree(0), Vector3::UNIT_Y) * Quaternion(Degree(0), Vector3::UNIT_X));
							props[propi].rotaZ += cstate;
							limiter = props[propi].rotaZ;
						}
					} else
					{
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_X) rx += cstate;
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_Y) ry += cstate;
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_Z) rz += cstate;
					}

					bool limiterchanged = false;
					// check if a positive custom limit is set to evaluate/calc flip back
					if (props[propi].animOpt2[animnum] - props[propi].animOpt4[animnum])
					{
						if (limiter > props[propi].animOpt2[animnum])
						{
							if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
							{
								limiter = props[propi].animOpt2[animnum];				// stop at limit
								props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if bounce is set
								limiterchanged = true;
							} else
							{
								limiter = props[propi].animOpt1[animnum];				// flip to other side at limit
								limiterchanged = true;
							}
						}
					} else
					{																	// no custom limit set, use 360�
						while (limiter > 180.0f)
						{
							if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
							{
								limiter = 180.0f;										// stop at limit
								props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if bounce is set
								limiterchanged = true;
							} else
							{
								limiter -= 360.0f;										// flip to other side at limit
								limiterchanged = true;
							}
						}
					}

					// check if a negative custom limit is set to evaluate/calc flip back
					if (props[propi].animOpt1[animnum] - props[propi].animOpt4[animnum])
					{
						if (limiter < (props[propi].animOpt1[animnum]))
						{
							if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
							{
								limiter = props[propi].animOpt1[animnum];				// stop at limit
								props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if active
								limiterchanged = true;
							} else
							{
								limiter = props[propi].animOpt2[animnum];				// flip to other side at limit
								limiterchanged = true;
							}
						}
					} else																// no custom limit set, use 360�
					{
						while (limiter < -180.0f)
						{
							if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
							{
								limiter = -180.0f;										// stop at limit
								props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if active
								limiterchanged = true;
							} else
							{
								limiter += 360.0f;										// flip to other side at limit including overflow
								limiterchanged = true;
							}
						}
					}

					if (limiterchanged)
					{
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_X) props[propi].rotaX = limiter;
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_Y) props[propi].rotaY = limiter;
						if (props[propi].animMode[animnum] & ANIM_MODE_ROTA_Z) props[propi].rotaZ = limiter;
					}
				}

				//offset prop

				// TODO Unused Varaible
				//float ox = props[propi].orgoffsetX;

				// TODO Unused Varaible
				//float oy = props[propi].orgoffsetY;

				// TODO Unused Varaible
				//float oz = props[propi].orgoffsetZ;

				if ((props[propi].animMode[animnum] & ANIM_MODE_OFFSET_X) || (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Y) || (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Z))
				{
					float offset = 0.0f;
					float autooffset = 0.0f;

					if (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_X) offset = props[propi].orgoffsetX;
					if (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Y) offset = props[propi].orgoffsetY;
					if (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Z) offset = props[propi].orgoffsetZ;

					offset += cstate;
					if (props[propi].animMode[animnum] & ANIM_MODE_AUTOANIMATE)
					{
						autooffset = offset;
						// check if a positive custom limit is set to evaluate/calc flip back
						if (props[propi].animOpt2[animnum] - props[propi].animOpt4[animnum])
						{
							if (autooffset > props[propi].animOpt2[animnum])
							{
								if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
								{
									autooffset = props[propi].animOpt2[animnum];			// stop at limit
									props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if active
								} else
									autooffset = props[propi].animOpt1[animnum];			// flip to other side at limit
							}
						} else																// no custom limit set, use 10x as default
						{
							while (autooffset > 10.0f)
							{
								if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
								{
									autooffset = 10.0f;										// stop at limit
									props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if bounce is set
								} else
									autooffset -= 20.0f;									// flip to other side at limit including overflow
							}
						}
						// check if a negative custom limit is set to evaluate/calc flip back
						if (props[propi].animOpt1[animnum] - props[propi].animOpt4[animnum])
						{
							if (autooffset < (props[propi].animOpt1[animnum]))
							{
								if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
								{
									autooffset = props[propi].animOpt1[animnum];			// stop at limit
									props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if active
								} else
									autooffset = props[propi].animOpt2[animnum];			// flip to other side at limit
							}
						} else																// no custom limit set, use -10x�
						{
							while (autooffset < -10.0f)
							{
								if (props[propi].animMode[animnum] & ANIM_MODE_NOFLIP)
								{
									autooffset = -10.0f;									// stop at limit
									props[propi].animOpt5[animnum] *= -1.0f;				// change cstate multiplier if bounce is set
								} else
									autooffset += 20.0f;									// flip to other side at limit including overflow
							}
						}
					}

					if (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_X)
					{
						props[propi].offsetx = offset;
						if (props[propi].animMode[animnum] & ANIM_MODE_AUTOANIMATE)
							props[propi].orgoffsetX = autooffset;
					}
					if (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Y)
					{
						props[propi].offsety = offset;
						if (props[propi].animMode[animnum] & ANIM_MODE_AUTOANIMATE)
							props[propi].orgoffsetY = autooffset;
					}
					if (props[propi].animMode[animnum] & ANIM_MODE_OFFSET_Z)
					{
						props[propi].offsetz = offset;
						if (props[propi].animMode[animnum] & ANIM_MODE_AUTOANIMATE)
							props[propi].orgoffsetZ = autooffset;
					}
				}
			}
			animnum++;
		}
		//recalc the quaternions with final stacked rotation values ( rx, ry, rz )
		rx += props[propi].rotaX;
		ry += props[propi].rotaY;
		rz += props[propi].rotaZ;
		props[propi].rot = Quaternion(Degree(rz), Vector3::UNIT_Z) * Quaternion(Degree(ry), Vector3::UNIT_Y) * Quaternion(Degree(rx), Vector3::UNIT_X);
	}

	BES_STOP(BES_CORE_AnimatedProps);

	if (state==ACTIVATED) //force feedback sensors
	{
		if (doUpdate)
		{
			affforce = 0;
			affhydro = 0;
		}
		if (currentcamera != -1)
		{
			affforce += nodes[cameranodepos[currentcamera]].Forces;
		}
		for (int i=0; i<free_hydro; i++)
		{
			if ((beams[hydro[i]].hydroFlags & (HYDRO_FLAG_DIR|HYDRO_FLAG_SPEED)) && !beams[hydro[i]].broken)
				affhydro += beams[hydro[i]].hydroRatio * beams[hydro[i]].refL * beams[hydro[i]].stress;
		}
	}

	//mouse stuff
	if (mousenode != -1)
	{
		Vector3 dir = mousepos - nodes[mousenode].AbsPosition;
		nodes[mousenode].Forces += mousemoveforce * dir;
	}

	// START Slidenode section /////////////////////////////////////////////////
	// these must be done before the integrator, or else the forces are not calculated properly
	BES_START(BES_CORE_SlideNodes);
	updateSlideNodeForces(dt);
	BES_STOP(BES_CORE_SlideNodes);
	// END Slidenode section   /////////////////////////////////////////////////

	BES_START(BES_CORE_Nodes);

	watercontact = false;

	calcNodes(doUpdate, dt, step, maxsteps);

	AxisAlignedBox tBoundingBox(nodes[0].AbsPosition, nodes[0].AbsPosition);

	for (unsigned int i = 0; i < collisionBoundingBoxes.size(); i++)
	{
		collisionBoundingBoxes[i].scale(Ogre::Vector3(0.0));
	}

	for (int i=0; i<free_node; i++)
	{
		tBoundingBox.merge(nodes[i].AbsPosition);
		if (nodes[i].collisionBoundingBoxID >= 0 && (unsigned int) nodes[i].collisionBoundingBoxID < collisionBoundingBoxes.size())
		{
			AxisAlignedBox &bb = collisionBoundingBoxes[nodes[i].collisionBoundingBoxID];
			if (bb.getSize().length() == 0.0 && bb.getMinimum().length() == 0.0)
			{
				bb.setExtents(nodes[i].AbsPosition, nodes[i].AbsPosition);
			} else
			{
				bb.merge(nodes[i].AbsPosition);
			}
		}
	}

	for (unsigned int i = 0; i < collisionBoundingBoxes.size(); i++)
	{
		collisionBoundingBoxes[i].setMinimum(collisionBoundingBoxes[i].getMinimum() - Vector3(0.05f, 0.05f, 0.05f));
		collisionBoundingBoxes[i].setMaximum(collisionBoundingBoxes[i].getMaximum() + Vector3(0.05f, 0.05f, 0.05f));

		predictedCollisionBoundingBoxes[i].setExtents(collisionBoundingBoxes[i].getMinimum(), collisionBoundingBoxes[i].getMaximum());
		predictedCollisionBoundingBoxes[i].merge(collisionBoundingBoxes[i].getMinimum() + nodes[0].Velocity);
		predictedCollisionBoundingBoxes[i].merge(collisionBoundingBoxes[i].getMaximum() + nodes[0].Velocity);
	}

	// anti-explosion guard
	// rationale behind 1e9 number:
	// - while 1e6 is reachable by a fast vehicle, it will be badly deformed and shaking due to loss of precision in calculations
	// - at 1e7 any typical RoR vehicle falls apart and stops functioning
	// - 1e9 may be reachable only by a vehicle that is 1000 times bigger than a typical RoR vehicle, and it will be a loooong trip
	// to be able to travel such long distances will require switching physics calculations to higher precision numbers
	// or taking a different approach to the simulation (truck-local coordinate system?)
	if (!inRange(tBoundingBox.getMinimum().x + tBoundingBox.getMaximum().x +
		tBoundingBox.getMinimum().y + tBoundingBox.getMaximum().y +
		tBoundingBox.getMinimum().z + tBoundingBox.getMaximum().z, -1e9, 1e9))
	{
		m_reset_request = REQUEST_RESET_ON_INIT_POS; // truck exploded, schedule reset
		return; // return early to avoid propagating invalid values
	}

	boundingBox.setMinimum(tBoundingBox.getMinimum() - Vector3(0.05f, 0.05f, 0.05f));
	boundingBox.setMaximum(tBoundingBox.getMaximum() + Vector3(0.05f, 0.05f, 0.05f));

	predictedBoundingBox.setExtents(boundingBox.getMinimum(), boundingBox.getMaximum());
	predictedBoundingBox.merge(boundingBox.getMinimum() + nodes[0].Velocity);
	predictedBoundingBox.merge(boundingBox.getMaximum() + nodes[0].Velocity);

	BES_STOP(BES_CORE_Nodes);
		
	BES_START(BES_CORE_Turboprop);

	//turboprop forces
	for (int i=0; i<free_aeroengine; i++)
		if (aeroengines[i]) aeroengines[i]->updateForces(dt, doUpdate);

	BES_STOP(BES_CORE_Turboprop);
	BES_START(BES_CORE_Screwprop);

	//screwprop forces
	for (int i=0; i<free_screwprop; i++)
		if (screwprops[i]) screwprops[i]->updateForces(doUpdate);

	BES_STOP(BES_CORE_Screwprop);
	BES_START(BES_CORE_Wing);

	//wing forces
	for (int i=0; i<free_wing; i++)
		if (wings[i].fa) wings[i].fa->updateForces();

	BES_STOP(BES_CORE_Wing);
	BES_START(BES_CORE_FuseDrag);

	//compute fuse drag
	if (fuseAirfoil)
	{
		Vector3 wind=-fuseFront->Velocity;
		float wspeed=wind.length();
		Vector3 axis=fuseFront->RelPosition-fuseBack->RelPosition;
		float s=axis.length()*fuseWidth;
		float cz, cx, cm;
		float v=axis.getRotationTo(wind).w;
		float aoa=0;
		if (v<1.0 && v>-1.0) aoa=2.0*acos(v); //quaternion fun
		fuseAirfoil->getparams(aoa, 1.0, 0.0, &cz, &cx, &cm);

		//tropospheric model valid up to 11.000m (33.000ft)
		float altitude=fuseFront->AbsPosition.y;

		// TODO Unused Varaible
		//float sea_level_temperature=273.15f+15.0f; //in Kelvin
		float sea_level_pressure=101325; //in Pa

		// TODO Unused Varaible
		//float airtemperature=sea_level_temperature-altitude*0.0065f; //in Kelvin
		float airpressure=sea_level_pressure*approx_pow(1.0-0.0065*altitude/288.1, 5.24947); //in Pa
		float airdensity=airpressure*0.0000120896f;//1.225 at sea level

		//fuselage as an airfoil + parasitic drag (half fuselage front surface almost as a flat plane!)
		fusedrag=((cx*s+fuseWidth*fuseWidth*0.5)*0.5*airdensity*wspeed/free_node)*wind; //free_node is never null
	}

	BES_STOP(BES_CORE_FuseDrag);
	BES_START(BES_CORE_Airbrakes);

	//airbrakes
	for (int i=0; i<free_airbrake; i++)
	{
		airbrakes[i]->applyForce();
	}

	BES_STOP(BES_CORE_Airbrakes);
	BES_START(BES_CORE_Buoyance);

	//water buoyance
	if (free_buoycab && water)
	{
		for (int i=0; i<free_buoycab; i++)
		{
			int tmpv = buoycabs[i] * 3;
			buoyance->computeNodeForce(&nodes[cabs[tmpv]], &nodes[cabs[tmpv+1]], &nodes[cabs[tmpv+2]], doUpdate == 1, buoycabtypes[i]);
		}
	}

	BES_STOP(BES_CORE_Buoyance);
	BES_START(BES_CORE_Axles);

	//wheel speed
	Real wspeed = 0.0;
	//wheel stuff

	float engine_torque = 0.0;

	// calculate torque per wheel
	if (engine && proped_wheels != 0)
		engine_torque = engine->getTorque() / proped_wheels;

	int propcounter = 0;
	float newspeeds[MAX_WHEELS];

	float intertorque[MAX_WHEELS] = {}; //bad initialization
	//old-style viscous code
	if (free_axle == 0)
	{
		//first, evaluate torque from inter-differential locking
		for (int i=0; i<proped_wheels/2-1; i++)
		{
			float speed1=(wheels[proppairs[i*2]].speed+wheels[proppairs[i*2+1]].speed)*0.5f;
			float speed2=(wheels[proppairs[i*2+2]].speed+wheels[proppairs[i*2+3]].speed)*0.5f;
			float torque=(speed1-speed2)*10000.0f;
			intertorque[i*2]-=torque*0.5f;
			intertorque[i*2+1]-=torque*0.5f;
			intertorque[i*2+2]+=torque*0.5f;
			intertorque[i*2+3]+=torque*0.5f;
		}
	}

	// new-style Axles
	// loop through all axles for inter axle torque, this is the torsion to keep
	// the axles aligned with each other as if they connected by a shaft
	for (int i=1; i<free_axle; i++)
	{
		if (!axles[i]) continue;
		Ogre::Real axle_torques[2] = {0.0f, 0.0f};
		differential_data_t diff_data =
		{
			{
				(wheels[axles[i-1]->wheel_1].speed + wheels[axles[i-1]->wheel_2].speed) * 0.5f,
					(wheels[axles[i]->wheel_1].speed + wheels[axles[i]->wheel_2].speed) * 0.5f
			},
			axles[i-1]->delta_rotation,
				{ axle_torques[0], axle_torques[1] },
				0, // no input torque, just calculate forces from different axle positions
				dt
		};

#if 0
		// use an open diff just for fun :)
		Axle::calcOpenDiff( diff_data );
#else
		// use the locked diff, most vehicles are setup this way...
		Axle::calcLockedDiff( diff_data );
#endif

		axles[i-1]->delta_rotation = diff_data.delta_rotation;
		axles[i]->delta_rotation = -diff_data.delta_rotation;

		intertorque[axles[i-1]->wheel_1] = diff_data.out_torque[0];
		intertorque[axles[i-1]->wheel_2] = diff_data.out_torque[0];
		intertorque[axles[i]->wheel_1] = diff_data.out_torque[1];
		intertorque[axles[i]->wheel_2] = diff_data.out_torque[1];
	}

	// loop through all the wheels (new style again)
	for (int i=0; i<free_axle; i++)
	{
		if (!axles[i]) continue;
		Ogre::Real axle_torques[2] = {0.0f, 0.0f};
		wheel_t *axle_wheels[2] = { &wheels[axles[i]->wheel_1], &wheels[axles[i]->wheel_2] };

		differential_data_t diff_data =
		{
			{ axle_wheels[0]->speed, axle_wheels[1]->speed },
			axle_wheels[0]->delta_rotation,
			{ axle_torques[0], axle_torques[1] },
			// twice the torque since this is for two wheels, plus extra torque from
			// inter-axle torsion
			2.0f * engine_torque + intertorque[axles[i]->wheel_1],
			dt
		};

		axles[i]->calcTorque( diff_data );

		axle_wheels[0]->delta_rotation = diff_data.delta_rotation;
		axle_wheels[1]->delta_rotation = -diff_data.delta_rotation;

		intertorque[axles[i]->wheel_1] = diff_data.out_torque[0];
		intertorque[axles[i]->wheel_2] = diff_data.out_torque[1];
	}

	BES_STOP(BES_CORE_Axles);
	BES_START(BES_CORE_Wheels);

	// driving aids traction control & anti-lock brake pulse
	tc_timer  += dt;
	alb_timer += dt;

	if (alb_timer >= alb_pulse_time)
	{
		alb_timer = 0.0f;
		alb_pulse_state = !alb_pulse_state;
	}
	if (tc_timer >= tc_pulse_time)
	{
		tc_timer = 0.0f;
		tc_pulse_state = !tc_pulse_state;
	}

	bool tc_active = false;
	bool alb_active = false;

	// get current speed
	float curspeed = nodes[0].Velocity.length();

	// fix for airplanes crashing when getAcc() is used
	float currentAcc = 0.0f;
	if (driveable == TRUCK && engine)
	{
		currentAcc = engine->getAcc();
	}

	for (int i=0; i<free_wheel; i++)
	{
		Real speedacc = 0.0;

		// total torque estimation
		Real total_torque = 0.0;
		if (wheels[i].propulsed > 0)
		{
			total_torque = (free_axle == 0) ? engine_torque : intertorque[i];
		}

		if (wheels[i].braked)
		{
			// handbrake
			float hbrake = 0.0f;

			if (parkingbrake && wheels[i].braked != 4)
			{
				hbrake = hbrakeforce;
			}

			// directional braking
			float dbrake = 0.0f;

			if (WheelSpeed < 20.0f && ((wheels[i].braked == 2 && hydrodirstate > 0.0f)
				|| (wheels[i].braked == 3 && hydrodirstate < 0.0f)))
			{
				dbrake = brakeforce * abs(hydrodirstate);
			}

			if ((brake != 0.0 || dbrake != 0.0 || hbrake != 0.0) && braked_wheels != 0 && fabs(wheels[i].speed) > 0.0f)
			{
				float brake_coef = 1.0f;
				float antilock_coef = 1.0f;
				// anti-lock braking
				if (alb_mode && alb_pulse_state && (brake > 0.0f || dbrake > 0.0f) && curspeed > fabs(wheels[i].speed) && curspeed > alb_minspeed)
				{
					antilock_coef = fabs(wheels[i].speed) / curspeed;
					antilock_coef = pow(antilock_coef, alb_ratio);
					alb_active = (antilock_coef < 0.9);
				}
				if (fabs(wheels[i].speed) < 1.0f)
				{
					if (wheels[i].firstLock)
					{
						wheels[i].avgSpeed = 0.0f;
						wheels[i].firstLock = false;
					}
					// anti-jitter
					if (fabs(wheels[i].avgSpeed) < 2.0f)
					{
						brake_coef = pow(fabs(wheels[i].speed), 2.0f);
					} else 
					{
						brake_coef = pow(fabs(wheels[i].speed), 0.5f);
					}
					// anti-skidding
					wheels[i].avgSpeed += wheels[i].speed;
					wheels[i].avgSpeed = std::max(-10.0f, std::min(wheels[i].avgSpeed, 10.0f));
					float speed_diff = wheels[i].speed - wheels[i].lastSpeed;
					float speed_prediction = wheels[i].speed + 0.5f * speed_diff;
					if (speed_prediction * wheels[i].avgSpeed < 0.0f)
					{
						brake_coef = 0.0f;
					}
				} else {
					wheels[i].firstLock = true;
				}

				if (wheels[i].speed > 0)
					total_torque -= ((brake + dbrake) * antilock_coef + hbrake) * brake_coef;
				else
					total_torque += ((brake + dbrake) * antilock_coef + hbrake) * brake_coef;
			}
		} else
		{
			wheels[i].firstLock = true;
		}

		// traction control
		if (tc_mode && tc_pulse_state && wheels[i].propulsed > 0 && currentAcc > 0.0f && fabs(wheels[i].speed) > curspeed)
		{
			curspeed = std::max(0.5f, curspeed);

			// tc_wheelslip = allowed amount of slip in percent
			float wheelslip = 1.0f + tc_wheelslip;
			// wheelslip allowed doubles up to tc_fade, a tribute to RoRs wheelspeed calculation and friction
			wheelslip += tc_wheelslip * (curspeed / tc_fade);

			if (fabs(wheels[i].speed) > curspeed * wheelslip)
			{
				float torque_coef = (curspeed * wheelslip) / fabs(wheels[i].speed);
				torque_coef = pow(torque_coef, tc_ratio);
				total_torque *= torque_coef;
				tc_active = (torque_coef < 0.9f);
			}
		}

		// old-style
		if (free_axle == 0 && wheels[i].propulsed > 0)
		{
			// differential locking
			if (i%2)
				total_torque -= (wheels[i].speed - wheels[i-1].speed) * 10000.0;
			else
				total_torque -= (wheels[i].speed - wheels[i+1].speed) * 10000.0;
			// inter differential locking
			total_torque += intertorque[propcounter];
			propcounter++;
		}

		// application to wheel
		Vector3 axis = wheels[i].refnode1->RelPosition - wheels[i].refnode0->RelPosition;
		float axis_precalc = total_torque/(Real)(wheels[i].nbnodes);
		axis = fast_normalise(axis);

		for (int j=0; j<wheels[i].nbnodes; j++)
		{
			Vector3 radius(Vector3::ZERO);

			if (j%2)
				radius = wheels[i].nodes[j]->RelPosition - wheels[i].refnode1->RelPosition;
			else
				radius = wheels[i].nodes[j]->RelPosition - wheels[i].refnode0->RelPosition;

			float inverted_rlen = fast_invSqrt(radius.squaredLength());

			if (wheels[i].propulsed==2)
			{
				radius = -radius;
			}

			Vector3 dir = axis.crossProduct(radius);
			wheels[i].nodes[j]->Forces += dir * (axis_precalc*inverted_rlen*inverted_rlen);
			//wheel speed
			if (j%2)
				speedacc += (wheels[i].nodes[j]->Velocity-wheels[i].refnode1->Velocity).dotProduct(dir) * inverted_rlen;
			else
				speedacc += (wheels[i].nodes[j]->Velocity-wheels[i].refnode0->Velocity).dotProduct(dir) * inverted_rlen;
		}
		// wheel speed
		newspeeds[i] = speedacc / wheels[i].nbnodes;
		if (wheels[i].propulsed==1)
		{
			wspeed += newspeeds[i];
		}
		// for network
		wheels[i].rp += (newspeeds[i] / wheels[i].radius) * dt;
		// reaction torque
		Vector3 rradius = wheels[i].arm->RelPosition - wheels[i].near_attach->RelPosition;
		Vector3 radius = Plane(axis, wheels[i].near_attach->RelPosition).projectVector(rradius);
		Real rlen = radius.length(); // length of the projected arm
		float offset = (rradius-radius).length(); // length of the error arm
		axis *= total_torque;
		if (rlen > 0.01)
		{
			radius /= (2.0f * rlen * rlen);
			Vector3 cforce = axis.crossProduct(radius);
			// modulate the force according to induced torque error
			if (offset * 2.0f > rlen)
				cforce = Vector3::ZERO; // too much error!
			else
				cforce *= (1.0f - ((offset * 2.0f) / rlen)); // linear modulation
			wheels[i].arm->Forces -= cforce;
			wheels[i].near_attach->Forces += cforce;
		}
	}

	// dashboard overlays for tc+alb
	if (doUpdate)
	{
		antilockbrake = false;
		tractioncontrol = false;
	}

	antilockbrake = std::max(antilockbrake, (int)alb_active);
	tractioncontrol = std::max(tractioncontrol, (int)tc_active);

	if (step == maxsteps)
	{
		if (!antilockbrake)
		{
#ifdef USE_OPENAL
			SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_ALB_ACTIVE);
#endif //USE_OPENAL
		} else
		{
#ifdef USE_OPENAL
			SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_ALB_ACTIVE);
#endif //USE_OPENAL
		}

		if (!tractioncontrol)
		{
#ifdef USE_OPENAL
			SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_TC_ACTIVE);
#endif //USE_OPENAL
		} else
		{
#ifdef USE_OPENAL
			SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_TC_ACTIVE);
#endif //USE_OPENAL
		}
	}

	for (int i=0; i<free_wheel; i++)
	{
		wheels[i].lastSpeed = wheels[i].speed;
		wheels[i].speed = newspeeds[i];
	}
	if (proped_wheels)
	{
		wspeed /= (float)proped_wheels;
	}

	// wheel speed  in m/s !
	WheelSpeed = wspeed;

	if (engine && free_wheel && wheels[0].radius > 0.0f)
	{
		engine->setSpin(wspeed / wheels[0].radius * RAD_PER_SEC_TO_RPM);
	}

	// calculate driven distance
	float distance_driven = fabs(wspeed * dt);
	odometerTotal += distance_driven;
	odometerUser  += distance_driven;

	BES_STOP(BES_CORE_Wheels);
	BES_START(BES_CORE_Shocks);

	//update position
	//		if (free_node != 0)
	//			aposition/=(Real)(free_node);
	//variable shocks for stabilization
	if (free_active_shock && stabcommand)
	{
		if ((stabcommand==1 && stabratio<0.1) || (stabcommand==-1 && stabratio>-0.1))
			stabratio=stabratio+(float)stabcommand*dt*STAB_RATE;
		for (int i=0; i<free_shock; i++)
		{
			// active shocks now
			if (shocks[i].flags & SHOCK_FLAG_RACTIVE)
				beams[shocks[i].beamid].L=beams[shocks[i].beamid].refL*(1.0+stabratio);
			else if (shocks[i].flags & SHOCK_FLAG_LACTIVE)
				beams[shocks[i].beamid].L=beams[shocks[i].beamid].refL*(1.0-stabratio);
		}
	}
	//auto shock adjust
	if (free_active_shock && doUpdate)
	{
		Vector3 dir = nodes[cameranodepos[0]].RelPosition-nodes[cameranoderoll[0]].RelPosition;
		dir.normalise();
		float roll = asin(dir.dotProduct(Vector3::UNIT_Y));
		//mWindow->setDebugText("Roll:"+ TOSTRING(roll));
		if (fabs(roll) > 0.2)
		{
			stabsleep = -1.0; //emergency timeout stop
		}
		if (fabs(roll) > 0.01 && stabsleep < 0.0)
		{
			if (roll>0.0 && stabcommand != -1)
			{
				stabcommand = 1;
			} else if (roll<0.0 && stabcommand != 1)
			{
				stabcommand=-1; 
			} else
			{
				stabcommand = 0;
				stabsleep = 3.0;
			}
		} else 
		{
			stabcommand = 0;
		}

#ifdef USE_OPENAL
		if (stabcommand && fabs(stabratio) < 0.1)
			SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_AIR);
		else
			SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_AIR);
#endif //OPENAL
	}

	BES_STOP(BES_CORE_Shocks);
	BES_START(BES_CORE_Hydros);

	//direction
	if (hydrodirstate!=0 || hydrodircommand!=0)
	{
		float rate=1;
		if (hydroSpeedCoupling)
		{
			//new rate value (30 instead of 40) to deal with the changed cmdKeyInertia settings
			rate=30.0/(10.0+fabs(wspeed/2.0));
			// minimum rate: 20% --> enables to steer high velocity trucks
			if (rate<1.2) rate = 1.2;
		}
		//need a maximum rate for analog devices, otherwise hydro beams break
		if (!hydroSpeedCoupling)
		{
			float hydrodirstateOld = hydrodirstate;
			hydrodirstate = hydrodircommand;
			if (abs(hydrodirstate-hydrodirstateOld) > 0.02)
			{
				hydrodirstate = (hydrodirstate - hydrodirstateOld) * 0.02 + hydrodirstateOld;
			}
		}
		if ( hydrodircommand!=0 && hydroSpeedCoupling)
		{
			if (hydrodirstate > hydrodircommand)
				hydrodirstate -= dt * rate;
			else
				hydrodirstate += dt * rate;
		}
		if (hydroSpeedCoupling)
		{
			float dirdelta=dt;
			if      (hydrodirstate >  dirdelta) hydrodirstate -= dirdelta;
			else if (hydrodirstate < -dirdelta) hydrodirstate += dirdelta;
			else hydrodirstate = 0;
		}
	}
	//aileron
	if (hydroaileronstate!=0 || hydroaileroncommand!=0)
	{
		if (hydroaileroncommand!=0)
		{
			if (hydroaileronstate > hydroaileroncommand)
				hydroaileronstate -= dt*4.0;
			else
				hydroaileronstate += dt*4.0;
		}
		float delta = dt;
		if      (hydroaileronstate >  delta) hydroaileronstate -= delta;
		else if (hydroaileronstate < -delta) hydroaileronstate += delta;
		else hydroaileronstate = 0;
	}
	//rudder
	if (hydrorudderstate!=0 || hydroruddercommand!=0)
	{
		if (hydroruddercommand!=0)
		{
			if (hydrorudderstate > hydroruddercommand)
				hydrorudderstate -= dt*4.0;
			else
				hydrorudderstate += dt*4.0;
		}

		float delta = dt;
		if      (hydrorudderstate >  delta) hydrorudderstate -= delta;
		else if (hydrorudderstate < -delta) hydrorudderstate += delta;
		else hydrorudderstate = 0;
	}
	//elevator
	if (hydroelevatorstate!=0 || hydroelevatorcommand!=0)
	{
		if (hydroelevatorcommand!=0)
		{
			if (hydroelevatorstate > hydroelevatorcommand)
				hydroelevatorstate -= dt*4.0;
			else
				hydroelevatorstate += dt*4.0;
		}
		float delta = dt;
		if      (hydroelevatorstate >  delta) hydroelevatorstate -= delta;
		else if (hydroelevatorstate < -delta) hydroelevatorstate += delta;
		else hydroelevatorstate = 0;
	}
	//update length, dirstate between -1.0 and 1.0
	for (int i=0; i<free_hydro; i++)
	{
		//compound hydro
		float cstate = 0.0f;
		int div = 0;
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_SPEED)
		{
			//special treatment for SPEED
			if (WheelSpeed < 12.0f)
				cstate += hydrodirstate * (12.0f - WheelSpeed) / 12.0f;
			div++;
		}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_DIR) {cstate+=hydrodirstate;div++;}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_AILERON) {cstate+=hydroaileronstate;div++;}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_RUDDER) {cstate+=hydrorudderstate;div++;}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_ELEVATOR) {cstate+=hydroelevatorstate;div++;}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_REV_AILERON) {cstate-=hydroaileronstate;div++;}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_REV_RUDDER) {cstate-=hydrorudderstate;div++;}
		if (beams[hydro[i]].hydroFlags & HYDRO_FLAG_REV_ELEVATOR) {cstate-=hydroelevatorstate;div++;}

		if (cstate >  1.0) cstate =  1.0;
		if (cstate < -1.0) cstate = -1.0;
		// Animators following, if no animator, skip all the tests...
		int flagstate = beams[hydro[i]].animFlags;
		if (flagstate)
		{
			float animoption = beams[hydro[i]].animOption;
			calcAnimators(flagstate, cstate, div, dt, 0.0f, 0.0f, animoption);
		}

		if (div)
		{
			cstate /= (float)div;

			if (hydroInertia)
				cstate=hydroInertia->calcCmdKeyDelay(cstate,i,dt);

			if (!(beams[hydro[i]].hydroFlags & HYDRO_FLAG_SPEED))
				hydrodirwheeldisplay=cstate;

			float factor = 1.0-cstate*beams[hydro[i]].hydroRatio;

			// check and apply animators limits if set
			if (flagstate)
			{
				if (factor < 1.0f - beams[hydro[i]].shortbound)
					factor = 1.0f - beams[hydro[i]].shortbound;
				if (factor > 1.0f + beams[hydro[i]].longbound)
					factor = 1.0f + beams[hydro[i]].longbound;
			}

			beams[hydro[i]].L=beams[hydro[i]].Lhydro * factor;

		}
	}

	BES_STOP(BES_CORE_Hydros);
	BES_START(BES_CORE_Commands);

	// commands
	if (hascommands)
	{
		int active = 0;
		int requested = 0;
		float work = 0.0;

		// canwork
		if (engine)
			canwork = engine->getRPM() > engine->getIdleRPM() * 0.95f;
		else
			canwork = true;

		// crankfactor
		float crankfactor = 1.0f;
		if (engine)
			crankfactor = engine->getCrankFactor();

		// speed up machines
		if (driveable==MACHINE)
			crankfactor = 2;

		for (int i=0; i<=MAX_COMMANDS; i++)
		{
			for (int j=0; j < (int)commandkey[i].beams.size(); j++)
			{
				int k = abs(commandkey[i].beams[j]);
				if (k >= 0 && k < free_beam)
				{
					beams[k].autoMoveLock = false;
				}
			}
		}

		for (int i=0; i<=MAX_COMMANDS; i++)
		{
			float oldValue = commandkey[i].commandValue;

			commandkey[i].commandValue = std::max(commandkey[i].playerInputValue, commandkey[i].triggerInputValue);

			commandkey[i].triggerInputValue = 0.0f;

			if (commandkey[i].commandValue > 0.01f && oldValue < 0.01f)
			{
				// just started
				commandkey[i].commandValueState = 1;
			} else if (commandkey[i].commandValue < 0.01f && oldValue > 0.01f)
			{
				// just stopped
				commandkey[i].commandValueState = -1;
			}

			for (int j=0; j < (int)commandkey[i].beams.size(); j++)
			{
				if (commandkey[i].commandValue >= 0.5)
				{
					int k = abs(commandkey[i].beams[j]);
					if (k >= 0 && k < free_beam)
					{
						beams[k].autoMoveLock = true;
					}
				}
			}
		}

		// now process normal commands
		for (int i=0; i<=MAX_COMMANDS; i++)
		{
			bool requestpower = false;
			for (int j=0; j < (int)commandkey[i].beams.size(); j++)
			{
				int bbeam_dir = (commandkey[i].beams[j] > 0) ? 1 : -1;
				int bbeam = std::abs(commandkey[i].beams[j]);

				if (bbeam > free_beam) continue;

				// restrict forces
				if (beams[bbeam].isForceRestricted)
					crankfactor = std::min(crankfactor, 1.0f);

				float v  = commandkey[i].commandValue;
				int &vst = commandkey[i].commandValueState;

				// self centering
				if (beams[bbeam].isCentering && !beams[bbeam].autoMoveLock)
				{
					// check for some error
					if (beams[bbeam].refL == 0 || beams[bbeam].L == 0)
						continue;

					float current = (beams[bbeam].L / beams[bbeam].refL);

					if (fabs(current-beams[bbeam].centerLength) < 0.0001)
					{
						// hold condition
						beams[bbeam].autoMovingMode = 0;
					} else
					{
						// determine direction
						if (current > beams[bbeam].centerLength)
							beams[bbeam].autoMovingMode = -1;
						else
							beams[bbeam].autoMovingMode = 1;
					}
				}

				if (beams[bbeam].refL != 0 && beams[bbeam].L != 0)
				{
					float clen = beams[bbeam].L / beams[bbeam].refL;
					if ((bbeam_dir > 0 && clen < beams[bbeam].commandLong) || (bbeam_dir < 0 && clen > beams[bbeam].commandShort))
					{
						float dl=beams[bbeam].L;

						if (beams[bbeam].isOnePressMode==2)
						{
							// one press + centering
							if (bbeam_dir*beams[bbeam].autoMovingMode > 0 && bbeam_dir*clen > bbeam_dir*beams[bbeam].centerLength && !beams[bbeam].pressedCenterMode)
							{
								beams[bbeam].pressedCenterMode = true;
								beams[bbeam].autoMovingMode = 0;
							} else if (bbeam_dir*beams[bbeam].autoMovingMode < 0 && bbeam_dir*clen > bbeam_dir*beams[bbeam].centerLength && beams[bbeam].pressedCenterMode)
							{
								beams[bbeam].pressedCenterMode = false;
							}
						}
						if (beams[bbeam].isOnePressMode > 0)
						{
							bool key = (v > 0.5);
							if (bbeam_dir*beams[bbeam].autoMovingMode <= 0 && key)
							{
								beams[bbeam].autoMovingMode = bbeam_dir*1;
							} else if (beams[bbeam].autoMovingMode == bbeam_dir*1 && !key)
							{
								beams[bbeam].autoMovingMode = bbeam_dir*2;
							} else if (beams[bbeam].autoMovingMode == bbeam_dir*2 && key)
							{
								beams[bbeam].autoMovingMode = bbeam_dir*3;
							} else if (beams[bbeam].autoMovingMode == bbeam_dir*3 && !key)
							{
								beams[bbeam].autoMovingMode = 0;
							}
						}

						if (cmdInertia)
							v = cmdInertia->calcCmdKeyDelay(v, i, dt);

						if (bbeam_dir*beams[bbeam].autoMovingMode > 0)
							v = 1;

						if (beams[bbeam].commandNeedsEngine && ((engine && !engine->running) || !canwork)) continue;

						if (v > 0.0f && beams[bbeam].commandEngineCoupling > 0.0f)
							requestpower = true;

#ifdef USE_OPENAL
						// command sounds
						if (vst == 1)
						{
							// just started
							SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_LINKED_COMMAND, SL_COMMAND, -i);
							SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_LINKED_COMMAND, SL_COMMAND, i);
							vst = 0;
						} else if (vst == -1)
						{
							// just stopped
							SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_LINKED_COMMAND, SL_COMMAND, i);
							vst = 0;
						} else if (vst == 0)
						{
							// already running, modulate
							SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_LINKED_COMMANDRATE, v, SL_COMMAND, i);
						}
#endif //USE_OPENAL
						float cf = 1.0f;

						if (beams[bbeam].commandEngineCoupling > 0)
							cf = crankfactor;

						if (bbeam_dir > 0)
							beams[bbeam].L *= (1.0 + beams[bbeam].commandRatioLong * v * cf * dt / beams[bbeam].L);
						else
							beams[bbeam].L *= (1.0 - beams[bbeam].commandRatioShort * v * cf * dt / beams[bbeam].L);

						dl = fabs(dl - beams[bbeam].L);
						if (requestpower)
						{
							active++;
							work += fabs(beams[bbeam].stress) * dl * beams[bbeam].commandEngineCoupling;
						}
					} else if (beams[bbeam].isOnePressMode > 0 && bbeam_dir*beams[bbeam].autoMovingMode > 0)
					{
						// beyond length
						beams[bbeam].autoMovingMode = 0;
					}
				}
			}
			// also for rotators
			for (int j=0; j < (int)commandkey[i].rotators.size(); j++)
			{
				float v = 0.0f;
				int rota = std::abs(commandkey[i].rotators[j]) - 1;

				if (rotators[rota].rotatorNeedsEngine && ((engine && !engine->running) || !canwork)) continue;

				if (rotaInertia)
				{
					v = rotaInertia->calcCmdKeyDelay(commandkey[i].commandValue, i, dt);

					if (v > 0.0f && rotators[rota].rotatorEngineCoupling > 0.0f)
						requestpower = true;
				}

				float cf = 1.0f;

				if (rotators[rota].rotatorEngineCoupling > 0.0f)
					cf = crankfactor;

				if (commandkey[i].rotators[j] > 0)
					rotators[rota].angle += rotators[rota].rate * v * cf * dt;
				else
					rotators[rota].angle -= rotators[rota].rate * v * cf * dt;
			}
			if (requestpower)
				requested++;
		}

		if (engine)
		{
			engine->hydropump = work;
			engine->prime     = requested;
		}
		if (doUpdate && state==ACTIVATED)
		{
#ifdef USE_OPENAL
			if (active > 0)
			{
				SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_PUMP);
				float pump_rpm = 660.0f * (1.0f - (work / (float)active) / 100.0f);
				SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_PUMP, pump_rpm);
			} else
			{
				SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_PUMP);
			}
#endif //USE_OPENAL
		}
		// rotators
		for (int i=0; i<free_rotator; i++)
		{
			// compute rotation axis
			Vector3 axis=nodes[rotators[i].axis1].RelPosition-nodes[rotators[i].axis2].RelPosition;
			//axis.normalise();
			axis=fast_normalise(axis);
			// find the reference plane
			Plane pl=Plane(axis, 0);
			// for each pair
			for (int k=0; k<2; k++)
			{
				// find the reference vectors
				Vector3 ref1=pl.projectVector(nodes[rotators[i].axis2].RelPosition-nodes[rotators[i].nodes1[k]].RelPosition);
				Vector3 ref2=pl.projectVector(nodes[rotators[i].axis2].RelPosition-nodes[rotators[i].nodes2[k]].RelPosition);
				// theory vector
				Vector3 th1=Quaternion(Radian(rotators[i].angle+3.14159/2.0), axis)*ref1;
				// find the angle error
				float aerror=asin((th1.normalisedCopy()).dotProduct(ref2.normalisedCopy()));
				//mWindow->setDebugText("Error:"+ TOSTRING(aerror));
				// exert forces
				float rigidity=rotators[i].force;
				Vector3 dir1=ref1.crossProduct(axis);
				//dir1.normalise();
				dir1=fast_normalise(dir1);
				Vector3 dir2=ref2.crossProduct(axis);
				//dir2.normalise();
				dir2=fast_normalise(dir2);
				float ref1len=ref1.length();
				float ref2len=ref2.length();

				// simple jitter fix
				if (ref1len <= rotators[i].tolerance) ref1len = 0.0f;
				if (ref2len <= rotators[i].tolerance) ref2len = 0.0f;

				nodes[rotators[i].nodes1[k]].Forces+=(aerror*ref1len*rigidity)*dir1;
				nodes[rotators[i].nodes2[k]].Forces-=(aerror*ref2len*rigidity)*dir2;
				// symmetric
				nodes[rotators[i].nodes1[k+2]].Forces-=(aerror*ref1len*rigidity)*dir1;
				nodes[rotators[i].nodes2[k+2]].Forces+=(aerror*ref2len*rigidity)*dir2;
			}
		}
	}

	BES_STOP(BES_CORE_Commands);

	// go through all ties and process them
	for (std::vector<tie_t>::iterator it=ties.begin(); it!=ties.end(); it++)
	{
		// only process tying ties
		if (!it->tying) continue;

		// division through zero guard
		if (it->beam->refL == 0 || it->beam->L == 0) continue;

		float clen = it->beam->L / it->beam->refL;
		if (clen > it->beam->commandShort)
		{
			float dl = it->beam->L;
			it->beam->L *= (1.0 - it->beam->commandRatioShort * dt / it->beam->L);
		} else
		{
			// tying finished, end reached
			it->tying = false;
		}

		// check if we hit a certain force limit, then abort the tying process
		if (fabs(it->beam->stress) > it->beam->maxtiestress)
			it->tying = false;
	}

	BES_START(BES_CORE_Replay);

	// we also store a new replay frame
	if (replay && replay->isValid())
	{
		replayTimer += dt;
		if (replayTimer > replayPrecision)
		{
			// store nodes
			node_simple_t *nbuff = (node_simple_t *)replay->getWriteBuffer(0);
			if (nbuff)
			{
				for (int i=0; i<free_node; i++)
					nbuff[i].pos = nodes[i].AbsPosition;

				// store beams
				beam_simple_t *bbuff = (beam_simple_t *)replay->getWriteBuffer(1);
				for (int i=0; i<free_beam; i++)
				{
					bbuff[i].broken = beams[i].broken;
					bbuff[i].disabled = beams[i].disabled;
				}

				replay->writeDone();
				replayTimer = 0.0f;
			}
		}
	}

	BES_STOP(BES_CORE_Replay);
}

bool Beam::calcForcesEulerPrepare(int doUpdate, Ogre::Real dt, int step, int maxsteps)
{
	if (dt==0.0) return false;
	if (state >= SLEEPING) return false;
	if (deleting) return false;
	if (m_reset_request) return false;

	BES_START(BES_CORE_WholeTruckCalc);

	forwardCommands();
	calcBeamsInterTruck(doUpdate, dt, step, maxsteps);

	return true;
}

void Beam::calcForcesEulerFinal(int doUpdate, Ogre::Real dt, int step, int maxsteps)
{
	calcHooks();
	calcRopes();

	BES_STOP(BES_CORE_WholeTruckCalc);
}

void Beam::calcBeams(int doUpdate, Ogre::Real dt, int step, int maxsteps)
{
	BES_START(BES_CORE_Beams);
	// Springs
	for (int i=0; i<free_beam; i++)
	{
		if (!beams[i].disabled && !beams[i].p2truck)
		{
			// Calculate beam length
			Vector3 dis = beams[i].p1->RelPosition - beams[i].p2->RelPosition;

			Real dislen = dis.squaredLength();
			Real inverted_dislen = fast_invSqrt(dislen);
			
			dislen *= inverted_dislen;

			// Calculate beam's deviation from normal
			Real difftoBeamL = dislen - beams[i].L;

			Real k = beams[i].k;
			Real d = beams[i].d;

			switch (beams[i].bounded)
			{
			case SHOCK1:
				{
					float interp_ratio;

					// Following code interpolates between defined beam parameters and default beam parameters
					if (difftoBeamL > beams[i].longbound * beams[i].L)
						interp_ratio =  difftoBeamL - beams[i].longbound  * beams[i].L;
					else if (difftoBeamL < -beams[i].shortbound * beams[i].L)
						interp_ratio = -difftoBeamL - beams[i].shortbound * beams[i].L;
					else
						break;

					// Hard (normal) shock bump
					float tspring = DEFAULT_SPRING;
					float tdamp   = DEFAULT_DAMP;

					// Skip camera, wheels or any other shocks which are not generated in a shocks or shocks2 section
					if (beams[i].type == BEAM_HYDRO || beams[i].type == BEAM_INVISIBLE_HYDRO)
					{
						tspring = beams[i].shock->sbd_spring;
						tdamp   = beams[i].shock->sbd_damp;
					}

					k += (tspring - k) * interp_ratio;
					d += (tdamp   - d) * interp_ratio;
				}
				break;

			case SHOCK2:
				calcShocks2(i, difftoBeamL, k, d, dt, doUpdate);
				break;

			case SUPPORTBEAM:
				if (difftoBeamL > 0.0f)
				{
					k  = 0.0f;
					d *= 0.1f;
					float break_limit = SUPPORT_BEAM_LIMIT_DEFAULT;
					if (beams[i].longbound > 0.0f)
					{
						// This is a supportbeam with a user set break limit, get the user set limit
						break_limit = beams[i].longbound;
					}

					// If support beam is extended the originallength * break_limit, break and disable it
					if (difftoBeamL > beams[i].L * break_limit)
					{
						beams[i].broken = true;
						beams[i].disabled = true;
						if (beambreakdebug)
						{
							LOG(" XXX Support-Beam " + TOSTRING(i) + " limit extended and broke. Length: " + TOSTRING(difftoBeamL) + 
									" / max. Length: " + TOSTRING(beams[i].L*break_limit) + ". It was between nodes " + TOSTRING(beams[i].p1->id) + " and " + TOSTRING(beams[i].p2->id) + ".");
						}
					}
				}
				break;

			case ROPE:
				if (difftoBeamL < 0.0f)
				{
					k  = 0.0f;
					d *= 0.1f;
				}
				break;
			}

			// Calculate beam's rate of change
			Vector3 v = beams[i].p1->Velocity - beams[i].p2->Velocity;

			float slen = -k * (difftoBeamL) - d * v.dotProduct(dis) * inverted_dislen;
			beams[i].stress = slen;

			// Fast test for deformation
			float len = std::abs(slen);
			if (len > beams[i].minmaxposnegstress)
			{
				if ((beams[i].type == BEAM_NORMAL || beams[i].type == BEAM_INVISIBLE) && beams[i].bounded != SHOCK1 && k != 0.0f)
				{
					// Actual deformation tests
					if (slen > beams[i].maxposstress && difftoBeamL < 0.0f) // compression
					{
						increased_accuracy = true;
						Real yield_length = beams[i].maxposstress / k;
						Real deform = difftoBeamL + yield_length * (1.0f - beams[i].plastic_coef);
						Real Lold = beams[i].L;
						beams[i].L += deform;
						beams[i].L = std::max(MIN_BEAM_LENGTH, beams[i].L);
						slen = slen - (slen - beams[i].maxposstress) * 0.5f;
						len = slen;
						if (beams[i].L > 0.0f && Lold > beams[i].L)
						{
							beams[i].maxposstress *= Lold / beams[i].L;
							beams[i].minmaxposnegstress = std::min(beams[i].maxposstress, -beams[i].maxnegstress);
							beams[i].minmaxposnegstress = std::min(beams[i].minmaxposnegstress, beams[i].strength);
						}
						// For the compression case we do not remove any of the beam's
						// strength for structure stability reasons
						//beams[i].strength += deform * k * 0.5f;
						if (beamdeformdebug)
						{
							LOG(" YYY Beam " + TOSTRING(i) + " just deformed with extension force " + TOSTRING(len) + 
									" / " + TOSTRING(beams[i].strength) + ". It was between nodes " + TOSTRING(beams[i].p1->id) + " and " + TOSTRING(beams[i].p2->id) + ".");
						}
					} else if (slen < beams[i].maxnegstress && difftoBeamL > 0.0f) // expansion
					{
						increased_accuracy = true;
						Real yield_length = beams[i].maxnegstress / k;
						Real deform = difftoBeamL + yield_length * (1.0f - beams[i].plastic_coef);
						Real Lold = beams[i].L;
						beams[i].L += deform;
						slen = slen - (slen - beams[i].maxnegstress) * 0.5f;
						len = -slen;
						if (Lold > 0.0f && beams[i].L > Lold)
						{
							beams[i].maxnegstress *= beams[i].L / Lold;
							beams[i].minmaxposnegstress = std::min(beams[i].maxposstress, -beams[i].maxnegstress);
							beams[i].minmaxposnegstress = std::min(beams[i].minmaxposnegstress, beams[i].strength);
						}
						beams[i].strength -= deform * k;
						if (beamdeformdebug)
						{
							LOG(" YYY Beam " + TOSTRING(i) + " just deformed with extension force " + TOSTRING(len) + 
									" / " + TOSTRING(beams[i].strength) + ". It was between nodes " + TOSTRING(beams[i].p1->id) + " and " + TOSTRING(beams[i].p2->id) + ".");
						}
					}
				}

				// Test if the beam should break
				if (len > beams[i].strength)
				{
					// Sound effect.
					// Sound volume depends on springs stored energy
#ifdef USE_OPENAL
					SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_BREAK, 0.5*k*difftoBeamL*difftoBeamL);
					SoundScriptManager::getSingleton().trigOnce(trucknum, SS_TRIG_BREAK);
#endif //OPENAL
					increased_accuracy = true;

					//Break the beam only when it is not connected to a node
					//which is a part of a collision triangle and has 2 "live" beams or less
					//connected to it.
					if (!((beams[i].p1->contacter && nodeBeamConnections(beams[i].p1->pos)<3) || (beams[i].p2->contacter && nodeBeamConnections(beams[i].p2->pos)<3)))
					{
						slen = 0.0f;
						beams[i].broken     = true;
						beams[i].disabled   = true;

						if (beambreakdebug)
						{
							LOG(" XXX Beam " + TOSTRING(i) + " just broke with force " + TOSTRING(len) + 
									" / " + TOSTRING(beams[i].strength) + ". It was between nodes " + TOSTRING(beams[i].p1->id) + " and " + TOSTRING(beams[i].p2->id) + ".");
						}

						// detachergroup check: beam[i] is already broken, check detacher group# == 0/default skip the check ( performance bypass for beams with default setting )
						// only perform this check if this is a master detacher beams (positive detacher group id > 0)
						if (beams[i].detacher_group > 0)
						{
							// cycle once through the other beams
							for (int j = 0; j < free_beam; j++)
							{
								// beam[i] detacher group# == checked beams detacher group# -> delete & disable checked beam
								// do this with all master(positive id) and minor(negative id) beams of this detacher group
								if (abs(beams[j].detacher_group) == beams[i].detacher_group)
								{
									beams[j].broken     = true;
									beams[j].disabled   = true;
									if (beambreakdebug)
									{
										LOG("Deleting Detacher BeamID: " + TOSTRING(j) + ", Detacher Group: " + TOSTRING(beams[i].detacher_group)+  ", trucknum: " + TOSTRING(trucknum));
									}
								}
							}
						}
					} else
					{
						beams[i].strength = 2.0f * beams[i].minmaxposnegstress;
					}

					// something broke, check buoyant hull
					for (int mk=0; mk<free_buoycab; mk++)
					{
						int tmpv = buoycabs[mk] * 3;
						if (buoycabtypes[mk] == Buoyance::BUOY_DRAGONLY) continue;
						if ((beams[i].p1==&nodes[cabs[tmpv]] || beams[i].p1==&nodes[cabs[tmpv+1]] || beams[i].p1==&nodes[cabs[tmpv+2]]) &&
							(beams[i].p2==&nodes[cabs[tmpv]] || beams[i].p2==&nodes[cabs[tmpv+1]] || beams[i].p2==&nodes[cabs[tmpv+2]]))
						{
							buoyance->setsink(1);
						}
					}
				}
			}

			// At last update the beam forces
			Vector3 f = dis;
			f *= (slen * inverted_dislen);
			beams[i].p1->Forces += f;
			beams[i].p2->Forces -= f;
		}
	}
	BES_STOP(BES_CORE_Beams);
}

void Beam::calcBeamsInterTruck(int doUpdate, Ogre::Real dt, int step, int maxsteps)
{
	for (int i=0; i<interTruckBeams.size(); i++)
	{
		if (!interTruckBeams[i]->disabled && interTruckBeams[i]->p2truck)
		{
			// Calculate beam length
			Vector3 dis = interTruckBeams[i]->p1->AbsPosition - interTruckBeams[i]->p2->AbsPosition;

			Real dislen = dis.squaredLength();
			Real inverted_dislen = fast_invSqrt(dislen);
			
			dislen *= inverted_dislen;

			// Calculate beam's deviation from normal
			Real difftoBeamL = dislen - interTruckBeams[i]->L;

			Real k = interTruckBeams[i]->k;
			Real d = interTruckBeams[i]->d;

			if (interTruckBeams[i]->bounded == ROPE && difftoBeamL < 0.0f)
			{
				k  = 0.0f;
				d *= 0.1f;
			}

			// Calculate beam's rate of change
			Vector3 v = interTruckBeams[i]->p1->Velocity - interTruckBeams[i]->p2->Velocity;

			float slen = -k * (difftoBeamL) - d * v.dotProduct(dis) * inverted_dislen;
			interTruckBeams[i]->stress = slen;

			// Fast test for deformation
			float len = std::abs(slen);
			if (len > interTruckBeams[i]->minmaxposnegstress)
			{
				if ((interTruckBeams[i]->type == BEAM_NORMAL || interTruckBeams[i]->type == BEAM_INVISIBLE) && interTruckBeams[i]->bounded != SHOCK1 && k != 0.0f)
				{
					// Actual deformation tests
					if (slen > interTruckBeams[i]->maxposstress && difftoBeamL < 0.0f) // compression
					{
						Real yield_length = interTruckBeams[i]->maxposstress / k;
						Real deform = difftoBeamL + yield_length * (1.0f - interTruckBeams[i]->plastic_coef);
						Real Lold = interTruckBeams[i]->L;
						interTruckBeams[i]->L += deform;
						interTruckBeams[i]->L = std::max(MIN_BEAM_LENGTH, interTruckBeams[i]->L);
						slen = slen - (slen - interTruckBeams[i]->maxposstress) * 0.5f;
						len = slen;
						if (interTruckBeams[i]->L > 0.0f && Lold > interTruckBeams[i]->L)
						{
							interTruckBeams[i]->maxposstress *= Lold / interTruckBeams[i]->L;
							interTruckBeams[i]->minmaxposnegstress = std::min(interTruckBeams[i]->maxposstress, -interTruckBeams[i]->maxnegstress);
							interTruckBeams[i]->minmaxposnegstress = std::min(interTruckBeams[i]->minmaxposnegstress, interTruckBeams[i]->strength);
						}
						// For the compression case we do not remove any of the beam's
						// strength for structure stability reasons
						//interTruckBeams[i]->strength += deform * k * 0.5f;
						if (beamdeformdebug)
						{
							LOG(" YYY Beam " + TOSTRING(i) + " just deformed with extension force " + TOSTRING(len) + 
									" / " + TOSTRING(interTruckBeams[i]->strength) + ". It was between nodes " + TOSTRING(interTruckBeams[i]->p1->id) + " and " + TOSTRING(interTruckBeams[i]->p2->id) + ".");
						}
					} else if (slen < interTruckBeams[i]->maxnegstress && difftoBeamL > 0.0f) // expansion
					{
						Real yield_length = interTruckBeams[i]->maxnegstress / k;
						Real deform = difftoBeamL + yield_length * (1.0f - interTruckBeams[i]->plastic_coef);
						Real Lold = interTruckBeams[i]->L;
						interTruckBeams[i]->L += deform;
						slen = slen - (slen - interTruckBeams[i]->maxnegstress) * 0.5f;
						len = -slen;
						if (Lold > 0.0f && interTruckBeams[i]->L > Lold)
						{
							interTruckBeams[i]->maxnegstress *= interTruckBeams[i]->L / Lold;
							interTruckBeams[i]->minmaxposnegstress = std::min(interTruckBeams[i]->maxposstress, -interTruckBeams[i]->maxnegstress);
							interTruckBeams[i]->minmaxposnegstress = std::min(interTruckBeams[i]->minmaxposnegstress, interTruckBeams[i]->strength);
						}
						interTruckBeams[i]->strength -= deform * k;
						if (beamdeformdebug)
						{
							LOG(" YYY Beam " + TOSTRING(i) + " just deformed with extension force " + TOSTRING(len) + 
									" / " + TOSTRING(interTruckBeams[i]->strength) + ". It was between nodes " + TOSTRING(interTruckBeams[i]->p1->id) + " and " + TOSTRING(interTruckBeams[i]->p2->id) + ".");
						}
					}
				}

				// Test if the beam should break
				if (len > interTruckBeams[i]->strength)
				{
					// Sound effect.
					// Sound volume depends on springs stored energy
#ifdef USE_OPENAL
					SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_BREAK, 0.5*k*difftoBeamL*difftoBeamL);
					SoundScriptManager::getSingleton().trigOnce(trucknum, SS_TRIG_BREAK);
#endif //OPENAL

					//Break the beam only when it is not connected to a node
					//which is a part of a collision triangle and has 2 "live" beams or less
					//connected to it.
					if (!((interTruckBeams[i]->p1->contacter && nodeBeamConnections(interTruckBeams[i]->p1->pos)<3) || (interTruckBeams[i]->p2->contacter && nodeBeamConnections(interTruckBeams[i]->p2->pos)<3)))
					{
						slen = 0.0f;
						interTruckBeams[i]->broken     = true;
						interTruckBeams[i]->disabled   = true;

						if (beambreakdebug)
						{
							LOG(" XXX Beam " + TOSTRING(i) + " just broke with force " + TOSTRING(len) + 
									" / " + TOSTRING(interTruckBeams[i]->strength) + ". It was between nodes " + TOSTRING(interTruckBeams[i]->p1->id) + " and " + TOSTRING(interTruckBeams[i]->p2->id) + ".");
						}

						// detachergroup check: beam[i] is already broken, check detacher group# == 0/default skip the check ( performance bypass for beams with default setting )
						// only perform this check if this is a master detacher beams (positive detacher group id > 0)
						if (interTruckBeams[i]->detacher_group > 0)
						{
							// cycle once through the other beams
							for (int j = 0; j < free_beam; j++)
							{
								// beam[i] detacher group# == checked beams detacher group# -> delete & disable checked beam
								// do this with all master(positive id) and minor(negative id) beams of this detacher group
								if (abs(beams[j].detacher_group) == interTruckBeams[i]->detacher_group)
								{
									beams[j].broken     = true;
									beams[j].disabled   = true;
									if (beambreakdebug)
									{
										LOG("Deleting Detacher BeamID: " + TOSTRING(j) + ", Detacher Group: " + TOSTRING(interTruckBeams[i]->detacher_group)+  ", trucknum: " + TOSTRING(trucknum));
									}
								}
							}
						}
					} else
					{
						interTruckBeams[i]->strength = 2.0f * interTruckBeams[i]->minmaxposnegstress;
					}
				}
			}

			// At last update the beam forces
			Vector3 f = dis;
			f *= (slen * inverted_dislen);
			interTruckBeams[i]->p1->Forces += f;
			interTruckBeams[i]->p2->Forces -= f;
		}
	}
}

void Beam::calcNodes(int doUpdate, Ogre::Real dt, int step, int maxsteps)
{
	IWater *water = 0;
	float gravity = -9.81f;
	if (gEnv->terrainManager)
	{
		water = gEnv->terrainManager->getWater();
		gravity = gEnv->terrainManager->getGravity();
	}

	for (int i=0; i<free_node; i++)
	{
		// wetness
		if (doUpdate)
		{
			if (nodes[i].wetstate == DRIPPING && !nodes[i].contactless && !nodes[i].disable_particles)
			{
				nodes[i].wettime += dt * maxsteps;
				if (nodes[i].wettime > 5.0)
				{
					nodes[i].wetstate = DRY;
				} else
				{
					if (!nodes[i].iswheel && dripp) dripp->allocDrip(nodes[i].AbsPosition, nodes[i].Velocity, nodes[i].wettime);
					if (nodes[i].isHot && dustp) dustp->allocVapour(nodes[i].AbsPosition, nodes[i].Velocity, nodes[i].wettime);
				}
			}
		}

		// COLLISION
		if (!nodes[i].contactless)
		{
			nodes[i].collTestTimer += dt;
			if (nodes[i].contacted || nodes[i].collTestTimer>0.005 || (nodes[i].iswheel && (high_res_wheelnode_collisions || nodes[i].collTestTimer>0.0025)) || increased_accuracy)
			{
				float ns = 0;
				ground_model_t *gm = 0; // this is used as result storage, so we can use it later on
				bool contacted = false;
				// reverted this construct to the old form, don't mess with it, the binary operator is intentionally!
				if ((contacted=gEnv->collisions->groundCollision(&nodes[i], nodes[i].collTestTimer, &gm, &ns)) | gEnv->collisions->nodeCollision(&nodes[i], contacted, nodes[i].collTestTimer, &ns, &gm))
				{
					// FX
					if (gm && doUpdate && !nodes[i].disable_particles)
					{
						float thresold = 10.0f;

						switch (gm->fx_type)
						{
						case Collisions::FX_DUSTY:
							if (dustp) dustp->malloc(nodes[i].AbsPosition, nodes[i].Velocity/2.0, gm->fx_colour);
							break;

						case Collisions::FX_HARD:
							// smokey
							if (nodes[i].iswheel && ns > thresold)
							{
								if (dustp) dustp->allocSmoke(nodes[i].AbsPosition, nodes[i].Velocity);
#ifdef USE_OPENAL
								SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_SCREETCH, (ns-thresold) / thresold);
								SoundScriptManager::getSingleton().trigOnce(trucknum, SS_TRIG_SCREETCH);
#endif //USE_OPENAL
//Shouldn't skidmarks be activated from here?
								if (useSkidmarks)
								{
									wheels[nodes[i].wheelid].isSkiding = true;
									if (!(nodes[i].iswheel % 2))
										wheels[nodes[i].wheelid].lastContactInner = nodes[i].AbsPosition;
									else
										wheels[nodes[i].wheelid].lastContactOuter = nodes[i].AbsPosition;

									wheels[nodes[i].wheelid].lastContactType = (nodes[i].iswheel % 2);
									wheels[nodes[i].wheelid].lastSlip = ns;
									wheels[nodes[i].wheelid].lastGroundModel = gm;
								}
							}
							// sparks
							if (!nodes[i].iswheel && ns > 1.0 && !nodes[i].disable_sparks)
							{
								// friction < 10 will remove the 'f' nodes from the spark generation nodes
								if (sparksp) sparksp->allocSparks(nodes[i].AbsPosition, nodes[i].Velocity);
							}
							if (nodes[i].iswheel && ns < thresold)
							{
								if (useSkidmarks)
								{
									wheels[nodes[i].wheelid].isSkiding = false;
								}
							}
							break;

						case Collisions::FX_CLUMPY:
							if (nodes[i].Velocity.squaredLength() > 1.0)
							{
								if (clumpp) clumpp->allocClump(nodes[i].AbsPosition, nodes[i].Velocity/2.0, gm->fx_colour);
							}
							break;
						default:
							//Useless for the moment
							break;
						}
					}

					lastFuzzyGroundModel = gm;
				}
				nodes[i].collTestTimer = 0.0;
			}
		}

		// record g forces on cameras
		if (i == cameranodepos[0])
		{
			cameranodeacc += nodes[i].Forces / nodes[i].mass;
			cameranodecount++;
		}

		// integration
		if (!nodes[i].locked)
		{
			nodes[i].Velocity += nodes[i].Forces / nodes[i].mass * dt;
			nodes[i].RelPosition += nodes[i].Velocity * dt;
			nodes[i].AbsPosition = origin;
			nodes[i].AbsPosition += nodes[i].RelPosition;
		}

		// prepare next loop (optimisation)
		// we start forces from zero
		// start with gravity
		nodes[i].Forces = Vector3(0, nodes[i].mass * gravity, 0);

		if (fuseAirfoil)
		{
			// aerodynamics on steroids!
			nodes[i].Forces += fusedrag;
		} else if (!disableDrag)
		{
			// add viscous drag (turbulent model)
			Real speed = approx_sqrt(nodes[i].Velocity.squaredLength()); //we will (not) reuse this
			Real defdragxspeed = DEFAULT_DRAG * speed;
			Vector3 drag = -defdragxspeed * nodes[i].Velocity;
			// plus: turbulences
			Real maxtur = defdragxspeed * speed * 0.005f;
			drag += maxtur * Vector3(frand_11(), frand_11(), frand_11());
			nodes[i].Forces += drag;
		}

		if (water)
		{
			if (water->isUnderWater(nodes[i].AbsPosition))
			{
				watercontact = true;
				if (free_buoycab == 0)
				{
					// water drag (turbulent)
					Real speed = approx_sqrt(nodes[i].Velocity.squaredLength()); //we will (not) reuse this
					nodes[i].Forces -= (DEFAULT_WATERDRAG * speed) * nodes[i].Velocity;
					// basic buoyance
					nodes[i].Forces += nodes[i].buoyancy * Vector3::UNIT_Y;
					// basic splashing
					if (doUpdate && water->getHeight() - nodes[i].AbsPosition.y < 0.2 && nodes[i].Velocity.squaredLength() > 4.0 && !nodes[i].disable_particles)
					{
						if (splashp) splashp->allocSplash(nodes[i].AbsPosition, nodes[i].Velocity);
						if (ripplep) ripplep->allocRipple(nodes[i].AbsPosition, nodes[i].Velocity);
					}
				}
				// engine stall
				if (i == cinecameranodepos[0] && engine)
				{
					engine->stop();
				}
				nodes[i].wetstate = WET;
			} else if (nodes[i].wetstate == WET)
			{
				nodes[i].wetstate = DRIPPING;
				nodes[i].wettime = 0.0f;
			}
		}
	}
}

void Beam::forwardCommands()
{
	Beam** trucks = BeamFactory::getSingleton().getTrucks();
	int numtrucks = BeamFactory::getSingleton().getTruckCount();

	// forward things to trailers
	if (numtrucks > 1 && state==ACTIVATED && forwardcommands)
	{
		for (int i=0; i<numtrucks; i++)
		{
			if (!trucks[i]) continue;
			if (trucks[i]->state==DESACTIVATED && trucks[i]->importcommands)
			{
				// forward commands
				for (int j=1; j<=MAX_COMMANDS; j++)
				{
					trucks[i]->commandkey[j].playerInputValue = std::max(commandkey[j].playerInputValue, commandkey[j].commandValue);
				}
				// just send brake and lights to the connected truck, and no one else :)
				for (std::vector<hook_t>::iterator it=hooks.begin(); it!=hooks.end(); it++)
				{
					if (!it->lockTruck) continue;
					// forward brake
					it->lockTruck->brake = brake;
					it->lockTruck->parkingbrake = parkingbrake;

					// forward lights
					it->lockTruck->lights = lights;
					it->lockTruck->blinkingtype = blinkingtype;
					//for (int k=0; k<4; k++)
					//	lockTruck->setCustomLight(k, getCustomLight(k));
					//forward reverse light e.g. for trailers
					it->lockTruck->reverselight = getReverseLightVisible();
				}
			}
		}
	}
}

void Beam::calcHooks()
{
	BES_START(BES_CORE_Hooks);
	//locks - this is not active in network mode
	for (std::vector<hook_t>::iterator it=hooks.begin(); it!=hooks.end(); it++)
	{
		if (it->lockNode && it->locked == PRELOCK)
		{
			if (it->beam->disabled)
			{
				//enable beam if not enabled yet between those 2 nodes
				it->beam->p2       = it->lockNode;
				it->beam->p2truck  = it->lockTruck != 0;
				it->beam->L = (it->hookNode->AbsPosition - it->lockNode->AbsPosition).length();
				it->beam->disabled = false;
				if (it->beam->mSceneNode->numAttachedObjects() == 0 && it->is_hook_visible)
				{
					it->beam->mSceneNode->attachObject(it->beam->mEntity);
				}
				addInterTruckBeam(it->beam);
			} else
			{
				if (it->beam->L < it->beam->commandShort)
				{
					//shortlimit reached -> status LOCKED
					it->locked = LOCKED;
				} else
				{
					//shorten the connecting beam slowly to locking minrange
					if (it->beam->L > it->lockspeed && fabs(it->beam->stress) < it->maxforce)
					{
						it->beam->L = (it->beam->L - it->lockspeed);
					} else
					{
						if (fabs(it->beam->stress) < it->maxforce)
						{
							it->beam->L = 0.001f;
							//locking minrange or stress exeeded -> status LOCKED
							it->locked = LOCKED;
						} else
						{
							if (it->nodisable)
							{
								//force exceed, but beam is set to nodisable, just lock it in this position
								it->locked = LOCKED;
							} else
							{
								//force exceeded reset the hook node
								it->beam->mSceneNode->detachAllObjects();
								it->locked = UNLOCKED;
								it->lockNode       = 0;
								it->lockTruck      = 0;
								it->beam->p2       = &nodes[0];
								it->beam->p2truck  = false;
								it->beam->L        = (nodes[0].AbsPosition - it->hookNode->AbsPosition).length();
								it->beam->disabled = true;
								removeInterTruckBeam(it->beam);
							}
						}
					}
				}
			}
		}
	}
	BES_STOP(BES_CORE_Hooks);
}

void Beam::calcRopes()
{
	BES_START(BES_CORE_Ropes);
	if (ropes.size())
	{
		for (std::vector <rope_t>::iterator it = ropes.begin(); it!=ropes.end(); it++)
		{
			if (it->lockedto)
			{
				it->beam->p2->AbsPosition = it->lockedto->AbsPosition;
				it->beam->p2->RelPosition = it->lockedto->AbsPosition - origin; //ropes[i].lockedtruck->origin; //we have a problem here
				it->beam->p2->Velocity = it->lockedto->Velocity;
				it->lockedto->Forces = it->lockedto->Forces + it->beam->p2->Forces;
				it->beam->p2->Forces = Vector3::ZERO;
			}
		}
	}
	BES_STOP(BES_CORE_Ropes);
}


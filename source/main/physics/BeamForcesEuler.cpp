/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

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
#include "RoRFrameListener.h"
#include "ScrewProp.h"
#include "SoundScriptManager.h"
#include "Water.h"
#include "TerrainManager.h"
#include "VehicleAI.h"

using namespace Ogre;

// Param "doUpdate" means "also update things which should be updated only once per frame, not per every physics tick"
//     In this case, doUpdate is TRUE on first tick after rendering, FALSE in all other ticks 
void Beam::calcForcesEulerCompute(bool doUpdate, Real dt, int step, int maxsteps)
{
    IWater* water = nullptr;
    const bool is_player_truck = this == m_sim_controller->GetPlayerActor();

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
    for (std::vector<hook_t>::iterator it = hooks.begin(); it != hooks.end(); it++)
    {
        //we need to do this here to avoid countdown speedup by triggers
        it->timer = std::max(0.0f, it->timer - dt);
    }

    if (is_player_truck) //force feedback sensors
    {
        if (doUpdate)
        {
            m_force_sensors.Reset();
        }

        if (currentcamera != -1)
        {
            m_force_sensors.accu_body_forces += ar_nodes[cameranodepos[currentcamera]].Forces;
        }

        for (int i = 0; i < free_hydro; i++)
        {
            beam_t* hydrobeam = &ar_beams[hydro[i]];
            if ((hydrobeam->hydroFlags & (HYDRO_FLAG_DIR | HYDRO_FLAG_SPEED)) && !hydrobeam->bm_broken)
            {
                m_force_sensors.accu_hydros_forces += hydrobeam->hydroRatio * hydrobeam->refL * hydrobeam->stress;
            }
        }
    }

    //mouse stuff
    if (mousenode != -1)
    {
        Vector3 dir = mousepos - ar_nodes[mousenode].AbsPosition;
        ar_nodes[mousenode].Forces += mousemoveforce * dir;
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

    AxisAlignedBox tBoundingBox(ar_nodes[0].AbsPosition, ar_nodes[0].AbsPosition);

    for (unsigned int i = 0; i < collisionBoundingBoxes.size(); i++)
    {
        collisionBoundingBoxes[i].scale(Ogre::Vector3(0.0));
    }

    for (int i = 0; i < ar_num_nodes; i++)
    {
        tBoundingBox.merge(ar_nodes[i].AbsPosition);
        if (ar_nodes[i].collisionBoundingBoxID >= 0 && (unsigned int) ar_nodes[i].collisionBoundingBoxID < collisionBoundingBoxes.size())
        {
            AxisAlignedBox& bb = collisionBoundingBoxes[ar_nodes[i].collisionBoundingBoxID];
            if (bb.getSize().length() == 0.0 && bb.getMinimum().length() == 0.0)
            {
                bb.setExtents(ar_nodes[i].AbsPosition, ar_nodes[i].AbsPosition);
            }
            else
            {
                bb.merge(ar_nodes[i].AbsPosition);
            }
        }
    }

    for (unsigned int i = 0; i < collisionBoundingBoxes.size(); i++)
    {
        collisionBoundingBoxes[i].setMinimum(collisionBoundingBoxes[i].getMinimum() - Vector3(0.05f, 0.05f, 0.05f));
        collisionBoundingBoxes[i].setMaximum(collisionBoundingBoxes[i].getMaximum() + Vector3(0.05f, 0.05f, 0.05f));

        predictedCollisionBoundingBoxes[i].setExtents(collisionBoundingBoxes[i].getMinimum(), collisionBoundingBoxes[i].getMaximum());
        predictedCollisionBoundingBoxes[i].merge(collisionBoundingBoxes[i].getMinimum() + ar_nodes[0].Velocity);
        predictedCollisionBoundingBoxes[i].merge(collisionBoundingBoxes[i].getMaximum() + ar_nodes[0].Velocity);
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
    predictedBoundingBox.merge(boundingBox.getMinimum() + ar_nodes[0].Velocity);
    predictedBoundingBox.merge(boundingBox.getMaximum() + ar_nodes[0].Velocity);

    BES_STOP(BES_CORE_Nodes);

    BES_START(BES_CORE_Turboprop);

    //turboprop forces
    for (int i = 0; i < free_aeroengine; i++)
        if (aeroengines[i])
            aeroengines[i]->updateForces(dt, doUpdate);

    BES_STOP(BES_CORE_Turboprop);
    BES_START(BES_CORE_Screwprop);

    //screwprop forces
    for (int i = 0; i < free_screwprop; i++)
        if (screwprops[i])
            screwprops[i]->updateForces(doUpdate);

    BES_STOP(BES_CORE_Screwprop);
    BES_START(BES_CORE_Wing);

    //wing forces
    for (int i = 0; i < free_wing; i++)
        if (wings[i].fa)
            wings[i].fa->updateForces();

    BES_STOP(BES_CORE_Wing);
    BES_START(BES_CORE_FuseDrag);

    //compute fuse drag
    if (fuseAirfoil)
    {
        Vector3 wind = -fuseFront->Velocity;
        float wspeed = wind.length();
        Vector3 axis = fuseFront->RelPosition - fuseBack->RelPosition;
        float s = axis.length() * fuseWidth;
        float cz, cx, cm;
        float v = axis.getRotationTo(wind).w;
        float aoa = 0;
        if (v < 1.0 && v > -1.0)
            aoa = 2.0 * acos(v); //quaternion fun
        fuseAirfoil->getparams(aoa, 1.0, 0.0, &cz, &cx, &cm);

        //tropospheric model valid up to 11.000m (33.000ft)
        float altitude = fuseFront->AbsPosition.y;

        // TODO Unused Varaible
        //float sea_level_temperature=273.15f+15.0f; //in Kelvin
        float sea_level_pressure = 101325; //in Pa

        // TODO Unused Varaible
        //float airtemperature=sea_level_temperature-altitude*0.0065f; //in Kelvin
        float airpressure = sea_level_pressure * approx_pow(1.0 - 0.0065 * altitude / 288.1, 5.24947); //in Pa
        float airdensity = airpressure * 0.0000120896f;//1.225 at sea level

        //fuselage as an airfoil + parasitic drag (half fuselage front surface almost as a flat plane!)
        fusedrag = ((cx * s + fuseWidth * fuseWidth * 0.5) * 0.5 * airdensity * wspeed / ar_num_nodes) * wind; 
    }

    BES_STOP(BES_CORE_FuseDrag);
    BES_START(BES_CORE_Airbrakes);

    //airbrakes
    for (int i = 0; i < free_airbrake; i++)
    {
        airbrakes[i]->applyForce();
    }

    BES_STOP(BES_CORE_Airbrakes);
    BES_START(BES_CORE_Buoyance);

    //water buoyance
    if (free_buoycab && water)
    {
        for (int i = 0; i < free_buoycab; i++)
        {
            int tmpv = buoycabs[i] * 3;
            buoyance->computeNodeForce(&ar_nodes[cabs[tmpv]], &ar_nodes[cabs[tmpv + 1]], &ar_nodes[cabs[tmpv + 2]], doUpdate == 1, buoycabtypes[i]);
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
    float newspeeds[MAX_WHEELS] = {};
    float intertorque[MAX_WHEELS] = {};
    //old-style viscous code
    if (free_axle == 0)
    {
        //first, evaluate torque from inter-differential locking
        for (int i = 0; i < proped_wheels / 2 - 1; i++)
        {
            if (wheels[proppairs[i * 2 + 0]].wh_is_detached)
                wheels[proppairs[i * 2 + 0]].wh_speed = wheels[proppairs[i * 2 + 1]].wh_speed;
            if (wheels[proppairs[i * 2 + 1]].wh_is_detached)
                wheels[proppairs[i * 2 + 1]].wh_speed = wheels[proppairs[i * 2 + 0]].wh_speed;
            if (wheels[proppairs[i * 2 + 2]].wh_is_detached)
                wheels[proppairs[i * 2 + 2]].wh_speed = wheels[proppairs[i * 2 + 3]].wh_speed;
            if (wheels[proppairs[i * 2 + 3]].wh_is_detached)
                wheels[proppairs[i * 2 + 3]].wh_speed = wheels[proppairs[i * 2 + 2]].wh_speed;

            float speed1 = (wheels[proppairs[i * 2 + 0]].wh_speed + wheels[proppairs[i * 2 + 1]].wh_speed) * 0.5f;
            float speed2 = (wheels[proppairs[i * 2 + 2]].wh_speed + wheels[proppairs[i * 2 + 3]].wh_speed) * 0.5f;
            float torque = (speed1 - speed2) * 10000.0f;

            intertorque[i * 2 + 0] -= torque * 0.5f;
            intertorque[i * 2 + 1] -= torque * 0.5f;
            intertorque[i * 2 + 2] += torque * 0.5f;
            intertorque[i * 2 + 3] += torque * 0.5f;
        }
    }

    // new-style Axles
    // loop through all axles for inter axle torque, this is the torsion to keep
    // the axles aligned with each other as if they connected by a shaft
    for (int i = 1; i < free_axle; i++)
    {
        if (!axles[i])
            continue;

        if (wheels[axles[i - 1]->wheel_1].wh_is_detached)
            wheels[axles[i - 1]->wheel_1].wh_speed = wheels[axles[i - 1]->wheel_2].wh_speed;
        if (wheels[axles[i - 0]->wheel_1].wh_is_detached)
            wheels[axles[i - 0]->wheel_1].wh_speed = wheels[axles[i - 0]->wheel_2].wh_speed;
        if (wheels[axles[i - 1]->wheel_2].wh_is_detached)
            wheels[axles[i - 1]->wheel_2].wh_speed = wheels[axles[i - 1]->wheel_1].wh_speed;
        if (wheels[axles[i - 0]->wheel_2].wh_is_detached)
            wheels[axles[i - 0]->wheel_2].wh_speed = wheels[axles[i - 0]->wheel_1].wh_speed;

        if (wheels[axles[i - 1]->wheel_1].wh_is_detached && wheels[axles[i - 1]->wheel_2].wh_is_detached)
        {
            wheels[axles[i - 1]->wheel_1].wh_speed = wheels[axles[i - 0]->wheel_1].wh_speed;
            wheels[axles[i - 1]->wheel_2].wh_speed = wheels[axles[i - 0]->wheel_2].wh_speed;
        }
        if (wheels[axles[i - 0]->wheel_1].wh_is_detached && wheels[axles[i - 0]->wheel_2].wh_is_detached)
        {
            wheels[axles[i - 0]->wheel_1].wh_speed = wheels[axles[i - 1]->wheel_1].wh_speed;
            wheels[axles[i - 0]->wheel_2].wh_speed = wheels[axles[i - 1]->wheel_2].wh_speed;
        }

        Ogre::Real axle_torques[2] = {0.0f, 0.0f};
        differential_data_t diff_data =
        {
            {
                (wheels[axles[i - 1]->wheel_1].wh_speed + wheels[axles[i - 1]->wheel_2].wh_speed) * 0.5f,
                (wheels[axles[i]->wheel_1].wh_speed + wheels[axles[i]->wheel_2].wh_speed) * 0.5f
            },
            axles[i - 1]->delta_rotation,
            {axle_torques[0], axle_torques[1]},
            0, // no input torque, just calculate forces from different axle positions
            dt
        };

#if 0
        // use an open diff just for fun :)
		Axle::calcOpenDiff( diff_data );
#else
        // use the locked diff, most vehicles are setup this way...
        Axle::calcLockedDiff(diff_data);
#endif

        axles[i - 1]->delta_rotation = diff_data.delta_rotation;
        axles[i]->delta_rotation = -diff_data.delta_rotation;

        intertorque[axles[i - 1]->wheel_1] = diff_data.out_torque[0];
        intertorque[axles[i - 1]->wheel_2] = diff_data.out_torque[0];
        intertorque[axles[i]->wheel_1] = diff_data.out_torque[1];
        intertorque[axles[i]->wheel_2] = diff_data.out_torque[1];
    }

    // loop through all the wheels (new style again)
    for (int i = 0; i < free_axle; i++)
    {
        if (!axles[i])
            continue;
        Ogre::Real axle_torques[2] = {0.0f, 0.0f};
        wheel_t* axle_wheels[2] = {&wheels[axles[i]->wheel_1], &wheels[axles[i]->wheel_2]};

        if (axle_wheels[0]->wh_is_detached)
            axle_wheels[0]->wh_speed = axle_wheels[1]->wh_speed;
        if (axle_wheels[1]->wh_is_detached)
            axle_wheels[1]->wh_speed = axle_wheels[0]->wh_speed;

        differential_data_t diff_data =
        {
            {axle_wheels[0]->wh_speed, axle_wheels[1]->wh_speed},
            axle_wheels[0]->wh_delta_rotation,
            {axle_torques[0], axle_torques[1]},
            // twice the torque since this is for two wheels, plus extra torque from
            // inter-axle torsion
            2.0f * engine_torque + intertorque[axles[i]->wheel_1],
            dt
        };

        axles[i]->calcTorque(diff_data);

        axle_wheels[0]->wh_delta_rotation = diff_data.delta_rotation;
        axle_wheels[1]->wh_delta_rotation = -diff_data.delta_rotation;

        intertorque[axles[i]->wheel_1] = diff_data.out_torque[0];
        intertorque[axles[i]->wheel_2] = diff_data.out_torque[1];
    }

    BES_STOP(BES_CORE_Axles);
    BES_START(BES_CORE_Wheels);

    // driving aids traction control & anti-lock brake pulse
    tc_timer += dt;
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
    float curspeed = ar_nodes[0].Velocity.length();

    // fix for airplanes crashing when getAcc() is used
    float currentAcc = 0.0f;
    if (driveable == TRUCK && engine)
    {
        currentAcc = engine->getAcc();
    }

    for (int i = 0; i < free_wheel; i++)
    {
        Real speedacc = 0.0;

        // total torque estimation
        Real total_torque = 0.0;
        if (wheels[i].wh_propulsed > 0)
        {
            total_torque = (free_axle == 0) ? engine_torque : intertorque[i];
        }

        if (wheels[i].wh_braking != wheel_t::BrakeCombo::NONE)
        {
            // handbrake
            float hbrake = 0.0f;

            if (parkingbrake && (wheels[i].wh_braking != wheel_t::BrakeCombo::FOOT_ONLY))
            {
                hbrake = hbrakeforce;
            }

            // directional braking
            float dbrake = 0.0f;

            if ((WheelSpeed < 20.0f)
                && (((wheels[i].wh_braking == wheel_t::BrakeCombo::FOOT_HAND_SKID_LEFT)  && (hydrodirstate > 0.0f))
                 || ((wheels[i].wh_braking == wheel_t::BrakeCombo::FOOT_HAND_SKID_RIGHT) && (hydrodirstate < 0.0f))))
            {
                dbrake = brakeforce * abs(hydrodirstate);
            }

            if ((brake != 0.0 || dbrake != 0.0 || hbrake != 0.0) && braked_wheels != 0 && fabs(wheels[i].wh_speed) > 0.0f)
            {
                float brake_coef = 1.0f;
                float antilock_coef = 1.0f;
                // anti-lock braking
                if (alb_mode && alb_pulse_state && (brake > 0.0f || dbrake > 0.0f) && curspeed > fabs(wheels[i].wh_speed) && curspeed > alb_minspeed)
                {
                    antilock_coef = fabs(wheels[i].wh_speed) / curspeed;
                    antilock_coef = pow(antilock_coef, alb_ratio);
                    alb_active = (antilock_coef < 0.9);
                }
                if (fabs(wheels[i].wh_speed) < 1.0f)
                {
                    if (wheels[i].firstLock)
                    {
                        wheels[i].wh_avg_speed = 0.0f;
                        wheels[i].firstLock = false;
                    }
                    // anti-jitter
                    if (fabs(wheels[i].wh_avg_speed) < 2.0f)
                    {
                        brake_coef = pow(fabs(wheels[i].wh_speed), 2.0f);
                    }
                    else
                    {
                        brake_coef = pow(fabs(wheels[i].wh_speed), 0.5f);
                    }
                    // anti-skidding
                    wheels[i].wh_avg_speed += wheels[i].wh_speed;
                    wheels[i].wh_avg_speed = std::max(-10.0f, std::min(wheels[i].wh_avg_speed, 10.0f));
                    float speed_diff = wheels[i].wh_speed - wheels[i].wh_last_speed;
                    float speed_prediction = wheels[i].wh_speed + 0.5f * speed_diff;
                    if (speed_prediction * wheels[i].wh_avg_speed < 0.0f)
                    {
                        brake_coef = 0.0f;
                    }
                }
                else
                {
                    wheels[i].firstLock = true;
                }

                if (wheels[i].wh_speed > 0)
                    total_torque -= ((brake + dbrake) * antilock_coef + hbrake) * brake_coef;
                else
                    total_torque += ((brake + dbrake) * antilock_coef + hbrake) * brake_coef;
            }
        }
        else
        {
            wheels[i].firstLock = true;
        }

        // traction control
        if (tc_mode && tc_pulse_state && wheels[i].wh_propulsed > 0 && currentAcc > 0.0f && fabs(wheels[i].wh_speed) > curspeed)
        {
            curspeed = std::max(0.5f, curspeed);

            // tc_wheelslip = allowed amount of slip in percent
            float wheelslip = 1.0f + tc_wheelslip;
            // wheelslip allowed doubles up to tc_fade, a tribute to RoRs wheelspeed calculation and friction
            wheelslip += tc_wheelslip * (curspeed / tc_fade);

            if (fabs(wheels[i].wh_speed) > curspeed * wheelslip)
            {
                float torque_coef = (curspeed * wheelslip) / fabs(wheels[i].wh_speed);
                torque_coef = pow(torque_coef, tc_ratio);
                total_torque *= torque_coef;
                tc_active = (torque_coef < 0.9f);
            }
        }

        // old-style
        if (free_axle == 0 && wheels[i].wh_propulsed > 0)
        {
            // differential locking
            if (i % 2)
                if (!wheels[i].wh_is_detached && !wheels[i - 1].wh_is_detached)
                    total_torque -= (wheels[i].wh_speed - wheels[i - 1].wh_speed) * 10000.0;
                else if (!wheels[i].wh_is_detached && !wheels[i + 1].wh_is_detached)
                    total_torque -= (wheels[i].wh_speed - wheels[i + 1].wh_speed) * 10000.0;
            // inter differential locking
            total_torque += intertorque[propcounter];
            propcounter++;
        }

        if (wheels[i].wh_is_detached)
            continue;

        // application to wheel
        Vector3 axis = wheels[i].wh_axis_node_1->RelPosition - wheels[i].wh_axis_node_0->RelPosition;
        float axis_precalc = total_torque / (Real)(wheels[i].wh_num_nodes);
        axis = fast_normalise(axis);

        for (int j = 0; j < wheels[i].wh_num_nodes; j++)
        {
            Vector3 radius(Vector3::ZERO);

            if (j % 2)
                radius = wheels[i].wh_nodes[j]->RelPosition - wheels[i].wh_axis_node_1->RelPosition;
            else
                radius = wheels[i].wh_nodes[j]->RelPosition - wheels[i].wh_axis_node_0->RelPosition;

            float inverted_rlen = fast_invSqrt(radius.squaredLength());

            if (wheels[i].wh_propulsed == 2)
            {
                radius = -radius;
            }

            Vector3 dir = axis.crossProduct(radius);
            wheels[i].wh_nodes[j]->Forces += dir * (axis_precalc * inverted_rlen * inverted_rlen);
            //wheel speed
            if (j % 2)
                speedacc += (wheels[i].wh_nodes[j]->Velocity - wheels[i].wh_axis_node_1->Velocity).dotProduct(dir) * inverted_rlen;
            else
                speedacc += (wheels[i].wh_nodes[j]->Velocity - wheels[i].wh_axis_node_0->Velocity).dotProduct(dir) * inverted_rlen;
        }
        // wheel speed
        newspeeds[i] = speedacc / wheels[i].wh_num_nodes;
        if (wheels[i].wh_propulsed == 1)
        {
            wspeed += newspeeds[i];
        }
        // for network
        wheels[i].wh_net_rp += (newspeeds[i] / wheels[i].wh_radius) * dt;
        // reaction torque
        Vector3 rradius = wheels[i].wh_arm_node->RelPosition - wheels[i].wh_near_attach_node->RelPosition;
        Vector3 radius = Plane(axis, wheels[i].wh_near_attach_node->RelPosition).projectVector(rradius);
        Real rlen = radius.length(); // length of the projected arm
        float offset = (rradius - radius).length(); // length of the error arm
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
            wheels[i].wh_arm_node->Forces -= cforce;
            wheels[i].wh_near_attach_node->Forces += cforce;
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
            SOUND_STOP(trucknum, SS_TRIG_ALB_ACTIVE);
        }
        else
        {
            SOUND_START(trucknum, SS_TRIG_ALB_ACTIVE);
        }

        if (!tractioncontrol)
        {
            SOUND_STOP(trucknum, SS_TRIG_TC_ACTIVE);
        }
        else
        {
            SOUND_START(trucknum, SS_TRIG_TC_ACTIVE);
        }
    }

    for (int i = 0; i < free_wheel; i++)
    {
        wheels[i].wh_last_speed = wheels[i].wh_speed;
        wheels[i].wh_speed = newspeeds[i];
    }
    if (proped_wheels)
    {
        wspeed /= (float)proped_wheels;
    }

    // wheel speed  in m/s !
    WheelSpeed = wspeed;

    if (engine && free_wheel && wheels[0].wh_radius > 0.0f)
    {
        engine->setSpin(wspeed / wheels[0].wh_radius * RAD_PER_SEC_TO_RPM);
    }

    // calculate driven distance
    float distance_driven = fabs(wspeed * dt);
    odometerTotal += distance_driven;
    odometerUser += distance_driven;

    BES_STOP(BES_CORE_Wheels);
    BES_START(BES_CORE_Shocks);

    //variable shocks for stabilization
    if (this->has_active_shocks && stabcommand)
    {
        if ((stabcommand == 1 && stabratio < 0.1) || (stabcommand == -1 && stabratio > -0.1))
            stabratio = stabratio + (float)stabcommand * dt * STAB_RATE;
        for (int i = 0; i < free_shock; i++)
        {
            // active shocks now
            if (shocks[i].flags & SHOCK_FLAG_RACTIVE)
                ar_beams[shocks[i].beamid].L = ar_beams[shocks[i].beamid].refL * (1.0 + stabratio);
            else if (shocks[i].flags & SHOCK_FLAG_LACTIVE)
                ar_beams[shocks[i].beamid].L = ar_beams[shocks[i].beamid].refL * (1.0 - stabratio);
        }
    }
    //auto shock adjust
    if (this->has_active_shocks && doUpdate)
    {
        stabsleep -= dt * maxsteps;

        Vector3 dir = ar_nodes[cameranodepos[0]].RelPosition - ar_nodes[cameranoderoll[0]].RelPosition;
        dir.normalise();
        float roll = asin(dir.dotProduct(Vector3::UNIT_Y));
        //mWindow->setDebugText("Roll:"+ TOSTRING(roll));
        if (fabs(roll) > 0.2)
        {
            stabsleep = -1.0; //emergency timeout stop
        }
        if (fabs(roll) > 0.01 && stabsleep < 0.0)
        {
            if (roll > 0.0 && stabcommand != -1)
            {
                stabcommand = 1;
            }
            else if (roll < 0.0 && stabcommand != 1)
            {
                stabcommand = -1;
            }
            else
            {
                stabcommand = 0;
                stabsleep = 3.0;
            }
        }
        else
        {
            stabcommand = 0;
        }

        if (stabcommand && fabs(stabratio) < 0.1)
            SOUND_START(trucknum, SS_TRIG_AIR);
        else
            SOUND_STOP(trucknum, SS_TRIG_AIR);
    }

    BES_STOP(BES_CORE_Shocks);
    BES_START(BES_CORE_Hydros);

    //direction
    if (hydrodirstate != 0 || hydrodircommand != 0)
    {
        float rate = 1;
        if (hydroSpeedCoupling)
        {
            //new rate value (30 instead of 40) to deal with the changed cmdKeyInertia settings
            rate = 30.0 / (10.0 + fabs(wspeed / 2.0));
            // minimum rate: 20% --> enables to steer high velocity trucks
            if (rate < 1.2)
                rate = 1.2;
        }
        //need a maximum rate for analog devices, otherwise hydro beams break
        if (!hydroSpeedCoupling)
        {
            float hydrodirstateOld = hydrodirstate;
            hydrodirstate = hydrodircommand;
            if (abs(hydrodirstate - hydrodirstateOld) > 0.02)
            {
                hydrodirstate = (hydrodirstate - hydrodirstateOld) * 0.02 + hydrodirstateOld;
            }
        }
        if (hydrodircommand != 0 && hydroSpeedCoupling)
        {
            if (hydrodirstate > hydrodircommand)
                hydrodirstate -= dt * rate;
            else
                hydrodirstate += dt * rate;
        }
        if (hydroSpeedCoupling)
        {
            float dirdelta = dt;
            if (hydrodirstate > dirdelta)
                hydrodirstate -= dirdelta;
            else if (hydrodirstate < -dirdelta)
                hydrodirstate += dirdelta;
            else
                hydrodirstate = 0;
        }
    }
    //aileron
    if (hydroaileronstate != 0 || hydroaileroncommand != 0)
    {
        if (hydroaileroncommand != 0)
        {
            if (hydroaileronstate > hydroaileroncommand)
                hydroaileronstate -= dt * 4.0;
            else
                hydroaileronstate += dt * 4.0;
        }
        float delta = dt;
        if (hydroaileronstate > delta)
            hydroaileronstate -= delta;
        else if (hydroaileronstate < -delta)
            hydroaileronstate += delta;
        else
            hydroaileronstate = 0;
    }
    //rudder
    if (hydrorudderstate != 0 || hydroruddercommand != 0)
    {
        if (hydroruddercommand != 0)
        {
            if (hydrorudderstate > hydroruddercommand)
                hydrorudderstate -= dt * 4.0;
            else
                hydrorudderstate += dt * 4.0;
        }

        float delta = dt;
        if (hydrorudderstate > delta)
            hydrorudderstate -= delta;
        else if (hydrorudderstate < -delta)
            hydrorudderstate += delta;
        else
            hydrorudderstate = 0;
    }
    //elevator
    if (hydroelevatorstate != 0 || hydroelevatorcommand != 0)
    {
        if (hydroelevatorcommand != 0)
        {
            if (hydroelevatorstate > hydroelevatorcommand)
                hydroelevatorstate -= dt * 4.0;
            else
                hydroelevatorstate += dt * 4.0;
        }
        float delta = dt;
        if (hydroelevatorstate > delta)
            hydroelevatorstate -= delta;
        else if (hydroelevatorstate < -delta)
            hydroelevatorstate += delta;
        else
            hydroelevatorstate = 0;
    }
    //update length, dirstate between -1.0 and 1.0
    for (int i = 0; i < free_hydro; i++)
    {
        //compound hydro
        float cstate = 0.0f;
        int div = 0;
        if (ar_beams[hydro[i]].hydroFlags & HYDRO_FLAG_SPEED)
        {
            //special treatment for SPEED
            if (WheelSpeed < 12.0f)
                cstate += hydrodirstate * (12.0f - WheelSpeed) / 12.0f;
            div++;
        }
        if (ar_beams[hydro[i]].hydroFlags & HYDRO_FLAG_DIR)
        {
            cstate += hydrodirstate;
            div++;
        }
        if (ar_beams[hydro[i]].hydroFlags & HYDRO_FLAG_AILERON)
        {
            cstate += hydroaileronstate;
            div++;
        }
        if (ar_beams[hydro[i]].hydroFlags & HYDRO_FLAG_RUDDER)
        {
            cstate += hydrorudderstate;
            div++;
        }
        if (ar_beams[hydro[i]].hydroFlags & HYDRO_FLAG_ELEVATOR)
        {
            cstate += hydroelevatorstate;
            div++;
        }
        if (ar_beams[hydro[i]].hydroFlags & HYDRO_FLAG_REV_AILERON)
        {
            cstate -= hydroaileronstate;
            div++;
        }
        if (ar_beams[hydro[i]].hydroFlags & HYDRO_FLAG_REV_RUDDER)
        {
            cstate -= hydrorudderstate;
            div++;
        }
        if (ar_beams[hydro[i]].hydroFlags & HYDRO_FLAG_REV_ELEVATOR)
        {
            cstate -= hydroelevatorstate;
            div++;
        }

        if (cstate > 1.0)
            cstate = 1.0;
        if (cstate < -1.0)
            cstate = -1.0;
        // Animators following, if no animator, skip all the tests...
        int flagstate = ar_beams[hydro[i]].animFlags;
        if (flagstate)
        {
            float animoption = ar_beams[hydro[i]].animOption;
            calcAnimators(flagstate, cstate, div, dt, 0.0f, 0.0f, animoption);
        }

        if (div)
        {
            cstate /= (float)div;

            if (hydroInertia)
                cstate = hydroInertia->calcCmdKeyDelay(cstate, i, dt);

            if (!(ar_beams[hydro[i]].hydroFlags & HYDRO_FLAG_SPEED) && !flagstate)
                hydrodirwheeldisplay = cstate;

            float factor = 1.0 - cstate * ar_beams[hydro[i]].hydroRatio;

            // check and apply animators limits if set
            if (flagstate)
            {
                if (factor < 1.0f - ar_beams[hydro[i]].shortbound)
                    factor = 1.0f - ar_beams[hydro[i]].shortbound;
                if (factor > 1.0f + ar_beams[hydro[i]].longbound)
                    factor = 1.0f + ar_beams[hydro[i]].longbound;
            }

            ar_beams[hydro[i]].L = ar_beams[hydro[i]].Lhydro * factor;
        }
    }

    BES_STOP(BES_CORE_Hydros);
    BES_START(BES_CORE_Commands);

    // commands
    if (hascommands)
    {
        int active = 0;
        bool requested = false;
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
        if (driveable == MACHINE)
            crankfactor = 2;

        for (int i = 0; i <= MAX_COMMANDS; i++)
        {
            for (int j = 0; j < (int)commandkey[i].beams.size(); j++)
            {
                int k = abs(commandkey[i].beams[j]);
                if (k >= 0 && k < ar_num_beams)
                {
                    ar_beams[k].autoMoveLock = false;
                }
            }
        }

        for (int i = 0; i <= MAX_COMMANDS; i++)
        {
            float oldValue = commandkey[i].commandValue;

            commandkey[i].commandValue = std::max(commandkey[i].playerInputValue, commandkey[i].triggerInputValue);

            commandkey[i].triggerInputValue = 0.0f;

            if (commandkey[i].commandValue > 0.01f && oldValue < 0.01f)
            {
                // just started
                commandkey[i].commandValueState = 1;
            }
            else if (commandkey[i].commandValue < 0.01f && oldValue > 0.01f)
            {
                // just stopped
                commandkey[i].commandValueState = -1;
            }

            for (int j = 0; j < (int)commandkey[i].beams.size(); j++)
            {
                if (commandkey[i].commandValue >= 0.5)
                {
                    int k = abs(commandkey[i].beams[j]);
                    if (k >= 0 && k < ar_num_beams)
                    {
                        ar_beams[k].autoMoveLock = true;
                    }
                }
            }
        }

        // now process normal commands
        for (int i = 0; i <= MAX_COMMANDS; i++)
        {
            bool requestpower = false;
            for (int j = 0; j < (int)commandkey[i].beams.size(); j++)
            {
                int bbeam_dir = (commandkey[i].beams[j] > 0) ? 1 : -1;
                int bbeam = std::abs(commandkey[i].beams[j]);

                if (bbeam > ar_num_beams)
                    continue;

                // restrict forces
                if (ar_beams[bbeam].isForceRestricted)
                    crankfactor = std::min(crankfactor, 1.0f);

                float v = commandkey[i].commandValue;
                int& vst = commandkey[i].commandValueState;

                // self centering
                if (ar_beams[bbeam].isCentering && !ar_beams[bbeam].autoMoveLock)
                {
                    // check for some error
                    if (ar_beams[bbeam].refL == 0 || ar_beams[bbeam].L == 0)
                        continue;

                    float current = (ar_beams[bbeam].L / ar_beams[bbeam].refL);

                    if (fabs(current - ar_beams[bbeam].centerLength) < 0.0001)
                    {
                        // hold condition
                        ar_beams[bbeam].autoMovingMode = 0;
                    }
                    else
                    {
                        int mode = ar_beams[bbeam].autoMovingMode;

                        // determine direction
                        if (current > ar_beams[bbeam].centerLength)
                            ar_beams[bbeam].autoMovingMode = -1;
                        else
                            ar_beams[bbeam].autoMovingMode = 1;

                        // avoid overshooting
                        if (mode != 0 && mode != ar_beams[bbeam].autoMovingMode)
                        {
                            ar_beams[bbeam].L = ar_beams[bbeam].centerLength * ar_beams[bbeam].refL;
                            ar_beams[bbeam].autoMovingMode = 0;
                        }
                    }
                }

                if (ar_beams[bbeam].refL != 0 && ar_beams[bbeam].L != 0)
                {
                    float clen = ar_beams[bbeam].L / ar_beams[bbeam].refL;
                    if ((bbeam_dir > 0 && clen < ar_beams[bbeam].commandLong) || (bbeam_dir < 0 && clen > ar_beams[bbeam].commandShort))
                    {
                        float dl = ar_beams[bbeam].L;

                        if (ar_beams[bbeam].isOnePressMode == 2)
                        {
                            // one press + centering
                            if (bbeam_dir * ar_beams[bbeam].autoMovingMode > 0 && bbeam_dir * clen > bbeam_dir * ar_beams[bbeam].centerLength && !ar_beams[bbeam].pressedCenterMode)
                            {
                                ar_beams[bbeam].pressedCenterMode = true;
                                ar_beams[bbeam].autoMovingMode = 0;
                            }
                            else if (bbeam_dir * ar_beams[bbeam].autoMovingMode < 0 && bbeam_dir * clen > bbeam_dir * ar_beams[bbeam].centerLength && ar_beams[bbeam].pressedCenterMode)
                            {
                                ar_beams[bbeam].pressedCenterMode = false;
                            }
                        }
                        if (ar_beams[bbeam].isOnePressMode > 0)
                        {
                            bool key = (v > 0.5);
                            if (bbeam_dir * ar_beams[bbeam].autoMovingMode <= 0 && key)
                            {
                                ar_beams[bbeam].autoMovingMode = bbeam_dir * 1;
                            }
                            else if (ar_beams[bbeam].autoMovingMode == bbeam_dir * 1 && !key)
                            {
                                ar_beams[bbeam].autoMovingMode = bbeam_dir * 2;
                            }
                            else if (ar_beams[bbeam].autoMovingMode == bbeam_dir * 2 && key)
                            {
                                ar_beams[bbeam].autoMovingMode = bbeam_dir * 3;
                            }
                            else if (ar_beams[bbeam].autoMovingMode == bbeam_dir * 3 && !key)
                            {
                                ar_beams[bbeam].autoMovingMode = 0;
                            }
                        }

                        if (cmdInertia)
                            v = cmdInertia->calcCmdKeyDelay(v, i, dt);

                        if (bbeam_dir * ar_beams[bbeam].autoMovingMode > 0)
                            v = 1;

                        if (ar_beams[bbeam].commandNeedsEngine && ((engine && !engine->isRunning()) || !canwork))
                            continue;

                        if (v > 0.0f && ar_beams[bbeam].commandEngineCoupling > 0.0f)
                            requestpower = true;

#ifdef USE_OPENAL
                        if (ar_beams[bbeam].playsSound)
                        {
                            // command sounds
                            if (vst == 1)
                            {
                                // just started
                                SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_LINKED_COMMAND, SL_COMMAND, -i);
                                SoundScriptManager::getSingleton().trigStart(trucknum, SS_TRIG_LINKED_COMMAND, SL_COMMAND, i);
                                vst = 0;
                            }
                            else if (vst == -1)
                            {
                                // just stopped
                                SoundScriptManager::getSingleton().trigStop(trucknum, SS_TRIG_LINKED_COMMAND, SL_COMMAND, i);
                                vst = 0;
                            }
                            else if (vst == 0)
                            {
                                // already running, modulate
                                SoundScriptManager::getSingleton().modulate(trucknum, SS_MOD_LINKED_COMMANDRATE, v, SL_COMMAND, i);
                            }
                        }
#endif //USE_OPENAL
                        float cf = 1.0f;

                        if (ar_beams[bbeam].commandEngineCoupling > 0)
                            cf = crankfactor;

                        if (bbeam_dir > 0)
                            ar_beams[bbeam].L *= (1.0 + ar_beams[bbeam].commandRatioLong * v * cf * dt / ar_beams[bbeam].L);
                        else
                            ar_beams[bbeam].L *= (1.0 - ar_beams[bbeam].commandRatioShort * v * cf * dt / ar_beams[bbeam].L);

                        dl = fabs(dl - ar_beams[bbeam].L);
                        if (requestpower)
                        {
                            active++;
                            work += fabs(ar_beams[bbeam].stress) * dl * ar_beams[bbeam].commandEngineCoupling;
                        }
                    }
                    else if (ar_beams[bbeam].isOnePressMode > 0 && bbeam_dir * ar_beams[bbeam].autoMovingMode > 0)
                    {
                        // beyond length
                        ar_beams[bbeam].autoMovingMode = 0;
                    }
                }
            }
            // also for rotators
            for (int j = 0; j < (int)commandkey[i].rotators.size(); j++)
            {
                float v = 0.0f;
                int rota = std::abs(commandkey[i].rotators[j]) - 1;

                if (rotators[rota].rotatorNeedsEngine && ((engine && !engine->isRunning()) || !canwork))
                    continue;

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
                requested=true;
        }

        if (engine)
        {
            engine->setHydroPumpWork(work);
            engine->setPrime(requested);
        }

        if (doUpdate && is_player_truck)
        {
#ifdef USE_OPENAL
            if (active > 0)
            {
                SOUND_START(trucknum, SS_TRIG_PUMP);
                float pump_rpm = 660.0f * (1.0f - (work / (float)active) / 100.0f);
                SOUND_MODULATE(trucknum, SS_MOD_PUMP, pump_rpm);
            }
            else
            {
                SOUND_STOP(trucknum, SS_TRIG_PUMP);
            }
#endif //USE_OPENAL
        }
        // rotators
        for (int i = 0; i < free_rotator; i++)
        {
            // compute rotation axis
            Vector3 axis = ar_nodes[rotators[i].axis1].RelPosition - ar_nodes[rotators[i].axis2].RelPosition;
            //axis.normalise();
            axis = fast_normalise(axis);
            // find the reference plane
            Plane pl = Plane(axis, 0);
            // for each pair
            for (int k = 0; k < 2; k++)
            {
                // find the reference vectors
                Vector3 ref1 = pl.projectVector(ar_nodes[rotators[i].axis2].RelPosition - ar_nodes[rotators[i].nodes1[k]].RelPosition);
                Vector3 ref2 = pl.projectVector(ar_nodes[rotators[i].axis2].RelPosition - ar_nodes[rotators[i].nodes2[k]].RelPosition);
                // theory vector
                Vector3 th1 = Quaternion(Radian(rotators[i].angle + 3.14159 / 2.0), axis) * ref1;
                // find the angle error
                float aerror = asin((th1.normalisedCopy()).dotProduct(ref2.normalisedCopy()));
                //mWindow->setDebugText("Error:"+ TOSTRING(aerror));
                // exert forces
                float rigidity = rotators[i].force;
                Vector3 dir1 = ref1.crossProduct(axis);
                //dir1.normalise();
                dir1 = fast_normalise(dir1);
                Vector3 dir2 = ref2.crossProduct(axis);
                //dir2.normalise();
                dir2 = fast_normalise(dir2);
                float ref1len = ref1.length();
                float ref2len = ref2.length();

                // simple jitter fix
                if (ref1len <= rotators[i].tolerance)
                    ref1len = 0.0f;
                if (ref2len <= rotators[i].tolerance)
                    ref2len = 0.0f;

                ar_nodes[rotators[i].nodes1[k]].Forces += (aerror * ref1len * rigidity) * dir1;
                ar_nodes[rotators[i].nodes2[k]].Forces -= (aerror * ref2len * rigidity) * dir2;
                // symmetric
                ar_nodes[rotators[i].nodes1[k + 2]].Forces -= (aerror * ref1len * rigidity) * dir1;
                ar_nodes[rotators[i].nodes2[k + 2]].Forces += (aerror * ref2len * rigidity) * dir2;
            }
        }
    }

    BES_STOP(BES_CORE_Commands);

    // go through all ties and process them
    for (std::vector<tie_t>::iterator it = ties.begin(); it != ties.end(); it++)
    {
        // only process tying ties
        if (!it->tying)
            continue;

        // division through zero guard
        if (it->beam->refL == 0 || it->beam->L == 0)
            continue;

        float clen = it->beam->L / it->beam->refL;
        if (clen > it->beam->commandShort)
        {
            it->beam->L *= (1.0 - it->beam->commandRatioShort * dt / it->beam->L);
        }
        else
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
            node_simple_t* nbuff = (node_simple_t *)replay->getWriteBuffer(0);
            if (nbuff)
            {
                for (int i = 0; i < ar_num_nodes; i++)
                {
                    nbuff[i].position = ar_nodes[i].AbsPosition;
                    nbuff[i].velocity = ar_nodes[i].Velocity;
                    nbuff[i].forces = ar_nodes[i].Forces;
                }
            }

            // store beams
            beam_simple_t* bbuff = (beam_simple_t *)replay->getWriteBuffer(1);
            if (bbuff)
            {
                for (int i = 0; i < ar_num_beams; i++)
                {
                    bbuff[i].broken = ar_beams[i].bm_broken;
                    bbuff[i].disabled = ar_beams[i].bm_disabled;
                }
            }

            replay->writeDone();
            replayTimer = 0.0f;
        }
    }

    BES_STOP(BES_CORE_Replay);
}

bool Beam::calcForcesEulerPrepare(int doUpdate, Ogre::Real dt, int step, int maxsteps)
{
    if (dt == 0.0)
        return false;
    if (m_reset_request)
        return false;
    if (ar_sim_state != Beam::SimState::LOCAL_SIMULATED)
        return false;

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

template <size_t L>
void LogNodeId(RoR::Str<L>& msg, node_t* node) // Internal helper
{
    if (node->id <= 0) // Named nodes have '0', generated have '-1'
    {
        msg << "\"?unknown?\"";
    }
    else
    {
        msg << "\"" << node->id << "\"";
    }
    msg << " (index: " << node->pos << ")";
}

template <size_t L>
void LogBeamNodes(RoR::Str<L>& msg, beam_t& beam) // Internal helper
{
    msg << "It was between nodes ";
    LogNodeId(msg, beam.p1);
    msg << " and ";
    LogNodeId(msg, beam.p2);
    msg << ".";
}

void Beam::calcBeams(int doUpdate, Ogre::Real dt, int step, int maxsteps)
{
    BES_START(BES_CORE_Beams);
    // Springs
    for (int i = 0; i < ar_num_beams; i++)
    {
        if (!ar_beams[i].bm_disabled && !ar_beams[i].p2truck)
        {
            // Calculate beam length
            Vector3 dis = ar_beams[i].p1->RelPosition - ar_beams[i].p2->RelPosition;

            Real dislen = dis.squaredLength();
            Real inverted_dislen = fast_invSqrt(dislen);

            dislen *= inverted_dislen;

            // Calculate beam's deviation from normal
            Real difftoBeamL = dislen - ar_beams[i].L;

            Real k = ar_beams[i].k;
            Real d = ar_beams[i].d;

            switch (ar_beams[i].bounded)
            {
            case SHOCK1:
                {
                    float interp_ratio;

                    // Following code interpolates between defined beam parameters and default beam parameters
                    if (difftoBeamL > ar_beams[i].longbound * ar_beams[i].L)
                        interp_ratio = difftoBeamL - ar_beams[i].longbound * ar_beams[i].L;
                    else if (difftoBeamL < -ar_beams[i].shortbound * ar_beams[i].L)
                        interp_ratio = -difftoBeamL - ar_beams[i].shortbound * ar_beams[i].L;
                    else
                        break;

                    // Hard (normal) shock bump
                    float tspring = DEFAULT_SPRING;
                    float tdamp = DEFAULT_DAMP;

                    // Skip camera, wheels or any other shocks which are not generated in a shocks or shocks2 section
                    if (ar_beams[i].bm_type == BEAM_HYDRO || ar_beams[i].bm_type == BEAM_INVISIBLE_HYDRO)
                    {
                        tspring = ar_beams[i].shock->sbd_spring;
                        tdamp = ar_beams[i].shock->sbd_damp;
                    }

                    k += (tspring - k) * interp_ratio;
                    d += (tdamp - d) * interp_ratio;
                }
                break;

            case SHOCK2:
                calcShocks2(i, difftoBeamL, k, d, dt, doUpdate);
                break;

            case SUPPORTBEAM:
                if (difftoBeamL > 0.0f)
                {
                    k = 0.0f;
                    d *= 0.1f;
                    float break_limit = SUPPORT_BEAM_LIMIT_DEFAULT;
                    if (ar_beams[i].longbound > 0.0f)
                    {
                        // This is a supportbeam with a user set break limit, get the user set limit
                        break_limit = ar_beams[i].longbound;
                    }

                    // If support beam is extended the originallength * break_limit, break and disable it
                    if (difftoBeamL > ar_beams[i].L * break_limit)
                    {
                        ar_beams[i].bm_broken = true;
                        ar_beams[i].bm_disabled = true;
                        if (beambreakdebug)
                        {
                            RoR::Str<300> msg;
                            msg << "[RoR|Diag] XXX Support-Beam " << i << " limit extended and broke. "
                                << "Length: " << difftoBeamL << " / max. Length: " << (ar_beams[i].L*break_limit) << ". ";
                            LogBeamNodes(msg, ar_beams[i]);
                            RoR::Log(msg.ToCStr());
                        }
                    }
                }
                break;

            case ROPE:
                if (difftoBeamL < 0.0f)
                {
                    k = 0.0f;
                    d *= 0.1f;
                }
                break;
            }

            // Calculate beam's rate of change
            Vector3 v = ar_beams[i].p1->Velocity - ar_beams[i].p2->Velocity;

            float slen = -k * (difftoBeamL) - d * v.dotProduct(dis) * inverted_dislen;
            ar_beams[i].stress = slen;

            // Fast test for deformation
            float len = std::abs(slen);
            if (len > ar_beams[i].minmaxposnegstress)
            {
                if ((ar_beams[i].bm_type == BEAM_NORMAL || ar_beams[i].bm_type == BEAM_INVISIBLE) && ar_beams[i].bounded != SHOCK1 && k != 0.0f)
                {
                    // Actual deformation tests
                    if (slen > ar_beams[i].maxposstress && difftoBeamL < 0.0f) // compression
                    {
                        increased_accuracy = true;
                        Real yield_length = ar_beams[i].maxposstress / k;
                        Real deform = difftoBeamL + yield_length * (1.0f - ar_beams[i].plastic_coef);
                        Real Lold = ar_beams[i].L;
                        ar_beams[i].L += deform;
                        ar_beams[i].L = std::max(MIN_BEAM_LENGTH, ar_beams[i].L);
                        slen = slen - (slen - ar_beams[i].maxposstress) * 0.5f;
                        len = slen;
                        if (ar_beams[i].L > 0.0f && Lold > ar_beams[i].L)
                        {
                            ar_beams[i].maxposstress *= Lold / ar_beams[i].L;
                            ar_beams[i].minmaxposnegstress = std::min(ar_beams[i].maxposstress, -ar_beams[i].maxnegstress);
                            ar_beams[i].minmaxposnegstress = std::min(ar_beams[i].minmaxposnegstress, ar_beams[i].strength);
                        }
                        // For the compression case we do not remove any of the beam's
                        // strength for structure stability reasons
                        //ar_beams[i].strength += deform * k * 0.5f;
                        if (beamdeformdebug)
                        {
                            RoR::Str<300> msg;
                            msg << "[RoR|Diag] YYY Beam " << i << " just deformed with extension force "
                                << len << " / " << ar_beams[i].strength << ". ";
                            LogBeamNodes(msg, ar_beams[i]);
                            RoR::Log(msg.ToCStr());
                        }
                    }
                    else if (slen < ar_beams[i].maxnegstress && difftoBeamL > 0.0f) // expansion
                    {
                        increased_accuracy = true;
                        Real yield_length = ar_beams[i].maxnegstress / k;
                        Real deform = difftoBeamL + yield_length * (1.0f - ar_beams[i].plastic_coef);
                        Real Lold = ar_beams[i].L;
                        ar_beams[i].L += deform;
                        slen = slen - (slen - ar_beams[i].maxnegstress) * 0.5f;
                        len = -slen;
                        if (Lold > 0.0f && ar_beams[i].L > Lold)
                        {
                            ar_beams[i].maxnegstress *= ar_beams[i].L / Lold;
                            ar_beams[i].minmaxposnegstress = std::min(ar_beams[i].maxposstress, -ar_beams[i].maxnegstress);
                            ar_beams[i].minmaxposnegstress = std::min(ar_beams[i].minmaxposnegstress, ar_beams[i].strength);
                        }
                        ar_beams[i].strength -= deform * k;
                        if (beamdeformdebug)
                        {
                            RoR::Str<300> msg;
                            msg << "[RoR|Diag] YYY Beam " << i << " just deformed with extension force "
                                << len << " / " << ar_beams[i].strength << ". ";
                            LogBeamNodes(msg, ar_beams[i]);
                            RoR::Log(msg.ToCStr());
                        }
                    }
                }

                // Test if the beam should break
                if (len > ar_beams[i].strength)
                {
                    // Sound effect.
                    // Sound volume depends on springs stored energy
                    SOUND_MODULATE(trucknum, SS_MOD_BREAK, 0.5 * k * difftoBeamL * difftoBeamL);
                    SOUND_PLAY_ONCE(trucknum, SS_TRIG_BREAK);

                    increased_accuracy = true;

                    //Break the beam only when it is not connected to a node
                    //which is a part of a collision triangle and has 2 "live" beams or less
                    //connected to it.
                    if (!((ar_beams[i].p1->contacter && nodeBeamConnections(ar_beams[i].p1->pos) < 3) || (ar_beams[i].p2->contacter && nodeBeamConnections(ar_beams[i].p2->pos) < 3)))
                    {
                        slen = 0.0f;
                        ar_beams[i].bm_broken = true;
                        ar_beams[i].bm_disabled = true;

                        if (beambreakdebug)
                        {
                            RoR::Str<200> msg;
                            msg << "[RoR|Diag] XXX Beam " << i << " just broke with force " << len << " / " << ar_beams[i].strength << ". ";
                            LogBeamNodes(msg, ar_beams[i]);
                            RoR::Log(msg.ToCStr());
                        }

                        // detachergroup check: beam[i] is already broken, check detacher group# == 0/default skip the check ( performance bypass for beams with default setting )
                        // only perform this check if this is a master detacher beams (positive detacher group id > 0)
                        if (ar_beams[i].detacher_group > 0)
                        {
                            // cycle once through the other beams
                            for (int j = 0; j < ar_num_beams; j++)
                            {
                                // beam[i] detacher group# == checked beams detacher group# -> delete & disable checked beam
                                // do this with all master(positive id) and minor(negative id) beams of this detacher group
                                if (abs(ar_beams[j].detacher_group) == ar_beams[i].detacher_group)
                                {
                                    ar_beams[j].bm_broken = true;
                                    ar_beams[j].bm_disabled = true;
                                    if (beambreakdebug)
                                    {
                                        LOG("Deleting Detacher BeamID: " + TOSTRING(j) + ", Detacher Group: " + TOSTRING(ar_beams[i].detacher_group)+ ", trucknum: " + TOSTRING(trucknum));
                                    }
                                }
                            }
                            // cycle once through all wheels
                            for (int j = 0; j < free_wheel; j++)
                            {
                                if (wheels[j].wh_detacher_group == ar_beams[i].detacher_group)
                                {
                                    wheels[j].wh_is_detached = true;
                                }
                            }
                        }
                    }
                    else
                    {
                        ar_beams[i].strength = 2.0f * ar_beams[i].minmaxposnegstress;
                    }

                    // something broke, check buoyant hull
                    for (int mk = 0; mk < free_buoycab; mk++)
                    {
                        int tmpv = buoycabs[mk] * 3;
                        if (buoycabtypes[mk] == Buoyance::BUOY_DRAGONLY)
                            continue;
                        if ((ar_beams[i].p1 == &ar_nodes[cabs[tmpv]] || ar_beams[i].p1 == &ar_nodes[cabs[tmpv + 1]] || ar_beams[i].p1 == &ar_nodes[cabs[tmpv + 2]]) &&
                            (ar_beams[i].p2 == &ar_nodes[cabs[tmpv]] || ar_beams[i].p2 == &ar_nodes[cabs[tmpv + 1]] || ar_beams[i].p2 == &ar_nodes[cabs[tmpv + 2]]))
                        {
                            buoyance->setsink(1);
                        }
                    }
                }
            }

            // At last update the beam forces
            Vector3 f = dis;
            f *= (slen * inverted_dislen);
            ar_beams[i].p1->Forces += f;
            ar_beams[i].p2->Forces -= f;
        }
    }
    BES_STOP(BES_CORE_Beams);
}

void Beam::calcBeamsInterTruck(int doUpdate, Ogre::Real dt, int step, int maxsteps)
{
    for (int i = 0; i < static_cast<int>(interTruckBeams.size()); i++)
    {
        if (!interTruckBeams[i]->bm_disabled && interTruckBeams[i]->p2truck)
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
                k = 0.0f;
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
                if ((interTruckBeams[i]->bm_type == BEAM_NORMAL || interTruckBeams[i]->bm_type == BEAM_INVISIBLE) && interTruckBeams[i]->bounded != SHOCK1 && k != 0.0f)
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
                            RoR::Str<300> msg;
                            msg << "[RoR|Diag] YYY Beam " << i << " just deformed with extension force "
                                << len << " / " << interTruckBeams[i]->strength << ". ";
                            LogBeamNodes(msg, (*interTruckBeams[i]));
                            RoR::Log(msg.ToCStr());
                        }
                    }
                    else if (slen < interTruckBeams[i]->maxnegstress && difftoBeamL > 0.0f) // expansion
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
                            RoR::Str<300> msg;
                            msg << "[RoR|Diag] YYY Beam " << i << " just deformed with extension force "
                                << len << " / " << interTruckBeams[i]->strength << ". ";
                            LogBeamNodes(msg, (*interTruckBeams[i]));
                            RoR::Log(msg.ToCStr());
                        }
                    }
                }

                // Test if the beam should break
                if (len > interTruckBeams[i]->strength)
                {
                    // Sound effect.
                    // Sound volume depends on springs stored energy
                    SOUND_MODULATE(trucknum, SS_MOD_BREAK, 0.5 * k * difftoBeamL * difftoBeamL);
                    SOUND_PLAY_ONCE(trucknum, SS_TRIG_BREAK);

                    //Break the beam only when it is not connected to a node
                    //which is a part of a collision triangle and has 2 "live" beams or less
                    //connected to it.
                    if (!((interTruckBeams[i]->p1->contacter && nodeBeamConnections(interTruckBeams[i]->p1->pos) < 3) || (interTruckBeams[i]->p2->contacter && nodeBeamConnections(interTruckBeams[i]->p2->pos) < 3)))
                    {
                        slen = 0.0f;
                        interTruckBeams[i]->bm_broken = true;
                        interTruckBeams[i]->bm_disabled = true;

                        if (beambreakdebug)
                        {
                            RoR::Str<200> msg;
                            msg << "[RoR|Diag] XXX Beam " << i << " just broke with force " << len << " / " << interTruckBeams[i]->strength << ". ";
                            LogBeamNodes(msg, (*interTruckBeams[i]));
                            RoR::Log(msg.ToCStr());
                        }
                    }
                    else
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
    IWater* water = 0;
    float gravity = -9.81f;
    if (gEnv->terrainManager)
    {
        water = gEnv->terrainManager->getWater();
        gravity = gEnv->terrainManager->getGravity();
    }

    for (int i = 0; i < ar_num_nodes; i++)
    {
        // wetness
        if (doUpdate)
        {
            if (ar_nodes[i].wetstate == DRIPPING && !ar_nodes[i].contactless && !ar_nodes[i].disable_particles)
            {
                ar_nodes[i].wettime += dt * maxsteps;
                if (ar_nodes[i].wettime > 5.0)
                {
                    ar_nodes[i].wetstate = DRY;
                }
                else
                {
                    if (!ar_nodes[i].iswheel && dripp)
                        dripp->allocDrip(ar_nodes[i].AbsPosition, ar_nodes[i].Velocity, ar_nodes[i].wettime);
                    if (ar_nodes[i].isHot && dustp)
                        dustp->allocVapour(ar_nodes[i].AbsPosition, ar_nodes[i].Velocity, ar_nodes[i].wettime);
                }
            }
        }

        // COLLISION
        if (!ar_nodes[i].contactless)
        {
            ar_nodes[i].collTestTimer += dt;
            if (ar_nodes[i].contacted || ar_nodes[i].collTestTimer > 0.005 || ((ar_nodes[i].iswheel || ar_nodes[i].wheelid != -1) && (high_res_wheelnode_collisions || ar_nodes[i].collTestTimer > 0.0025)) || increased_accuracy)
            {
                float ns = 0;
                ground_model_t* gm = 0; // this is used as result storage, so we can use it later on
                bool contacted = gEnv->collisions->groundCollision(&ar_nodes[i], ar_nodes[i].collTestTimer, &gm, &ns);
                // reverted this construct to the old form, don't mess with it, the binary operator is intentionally!
                if (contacted | gEnv->collisions->nodeCollision(&ar_nodes[i], contacted, ar_nodes[i].collTestTimer, &ns, &gm))
                {
                    // FX
                    if (gm && doUpdate && !ar_nodes[i].disable_particles)
                    {
                        float thresold = 10.0f;

                        switch (gm->fx_type)
                        {
                        case Collisions::FX_DUSTY:
                            if (dustp)
                                dustp->malloc(ar_nodes[i].AbsPosition, ar_nodes[i].Velocity / 2.0, gm->fx_colour);
                            break;

                        case Collisions::FX_HARD:
                            // smokey
                            if (ar_nodes[i].iswheel && ns > thresold)
                            {
                                if (dustp)
                                    dustp->allocSmoke(ar_nodes[i].AbsPosition, ar_nodes[i].Velocity);

                                SOUND_MODULATE(trucknum, SS_MOD_SCREETCH, (ns - thresold) / thresold);
                                SOUND_PLAY_ONCE(trucknum, SS_TRIG_SCREETCH);

                                //Shouldn't skidmarks be activated from here?
                                if (useSkidmarks)
                                {
                                    wheels[ar_nodes[i].wheelid].isSkiding = true;
                                    if (!(ar_nodes[i].iswheel % 2))
                                        wheels[ar_nodes[i].wheelid].lastContactInner = ar_nodes[i].AbsPosition;
                                    else
                                        wheels[ar_nodes[i].wheelid].lastContactOuter = ar_nodes[i].AbsPosition;

                                    wheels[ar_nodes[i].wheelid].lastContactType = (ar_nodes[i].iswheel % 2);
                                    wheels[ar_nodes[i].wheelid].lastSlip = ns;
                                    wheels[ar_nodes[i].wheelid].lastGroundModel = gm;
                                }
                            }
                            // sparks
                            if (!ar_nodes[i].iswheel && ns > 1.0 && !ar_nodes[i].disable_sparks)
                            {
                                // friction < 10 will remove the 'f' nodes from the spark generation nodes
                                if (sparksp)
                                    sparksp->allocSparks(ar_nodes[i].AbsPosition, ar_nodes[i].Velocity);
                            }
                            if (ar_nodes[i].iswheel && ns < thresold)
                            {
                                if (useSkidmarks)
                                {
                                    wheels[ar_nodes[i].wheelid].isSkiding = false;
                                }
                            }
                            break;

                        case Collisions::FX_CLUMPY:
                            if (ar_nodes[i].Velocity.squaredLength() > 1.0)
                            {
                                if (clumpp)
                                    clumpp->allocClump(ar_nodes[i].AbsPosition, ar_nodes[i].Velocity / 2.0, gm->fx_colour);
                            }
                            break;
                        default:
                            //Useless for the moment
                            break;
                        }
                    }

                    lastFuzzyGroundModel = gm;
                }
                ar_nodes[i].collTestTimer = 0.0;
            }
        }

        // record g forces on cameras
        if (i == cameranodepos[0])
        {
            cameranodeacc += ar_nodes[i].Forces / ar_nodes[i].mass;
            cameranodecount++;
        }

        // integration
        if (!ar_nodes[i].locked)
        {
            ar_nodes[i].Velocity += ar_nodes[i].Forces / ar_nodes[i].mass * dt;
            ar_nodes[i].RelPosition += ar_nodes[i].Velocity * dt;
            ar_nodes[i].AbsPosition = origin;
            ar_nodes[i].AbsPosition += ar_nodes[i].RelPosition;
        }

        // prepare next loop (optimisation)
        // we start forces from zero
        // start with gravity
        ar_nodes[i].Forces = Vector3(0, ar_nodes[i].mass * gravity, 0);

        if (fuseAirfoil)
        {
            // aerodynamics on steroids!
            ar_nodes[i].Forces += fusedrag;
        }
        else if (!disableDrag)
        {
            // add viscous drag (turbulent model)
            Real speed = approx_sqrt(ar_nodes[i].Velocity.squaredLength()); //we will (not) reuse this
            Real defdragxspeed = DEFAULT_DRAG * speed;
            Vector3 drag = -defdragxspeed * ar_nodes[i].Velocity;
            // plus: turbulences
            Real maxtur = defdragxspeed * speed * 0.005f;
            drag += maxtur * Vector3(frand_11(), frand_11(), frand_11());
            ar_nodes[i].Forces += drag;
        }

        if (water)
        {
            if (water->isUnderWater(ar_nodes[i].AbsPosition))
            {
                watercontact = true;
                if (free_buoycab == 0)
                {
                    // water drag (turbulent)
                    Real speed = approx_sqrt(ar_nodes[i].Velocity.squaredLength()); //we will (not) reuse this
                    ar_nodes[i].Forces -= (DEFAULT_WATERDRAG * speed) * ar_nodes[i].Velocity;
                    // basic buoyance
                    ar_nodes[i].Forces += ar_nodes[i].buoyancy * Vector3::UNIT_Y;
                    // basic splashing
                    if (doUpdate && water->GetStaticWaterHeight() - ar_nodes[i].AbsPosition.y < 0.2 && ar_nodes[i].Velocity.squaredLength() > 4.0 && !ar_nodes[i].disable_particles)
                    {
                        if (splashp)
                            splashp->allocSplash(ar_nodes[i].AbsPosition, ar_nodes[i].Velocity);
                        if (ripplep)
                            ripplep->allocRipple(ar_nodes[i].AbsPosition, ar_nodes[i].Velocity);
                    }
                }
                // engine stall
                if (i == cinecameranodepos[0] && engine)
                {
                    engine->stop();
                }
                ar_nodes[i].wetstate = WET;
            }
            else if (ar_nodes[i].wetstate == WET)
            {
                ar_nodes[i].wetstate = DRIPPING;
                ar_nodes[i].wettime = 0.0f;
            }
        }
    }
}

void Beam::forwardCommands()
{
    Beam* current_truck = m_sim_controller->GetPlayerActor();
    auto bf = m_sim_controller->GetBeamFactory();
    Beam** trucks = bf->getTrucks();
    int numtrucks = bf->getTruckCount();

    // forward things to trailers
    if (numtrucks > 1 && this == current_truck && forwardcommands)
    {
        for (int i = 0; i < numtrucks; i++)
        {
            if (!trucks[i])
                continue;
            if (trucks[i] != current_truck && trucks[i]->importcommands)
            {
                // forward commands
                for (int j = 1; j <= MAX_COMMANDS; j++)
                {
                    trucks[i]->commandkey[j].playerInputValue = std::max(commandkey[j].playerInputValue, commandkey[j].commandValue);
                }
                // just send brake and lights to the connected truck, and no one else :)
                for (std::vector<hook_t>::iterator it = hooks.begin(); it != hooks.end(); it++)
                {
                    if (!it->lockTruck)
                        continue;
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
    for (std::vector<hook_t>::iterator it = hooks.begin(); it != hooks.end(); it++)
    {
        if (it->lockNode && it->locked == PRELOCK)
        {
            if (it->beam->bm_disabled)
            {
                //enable beam if not enabled yet between those 2 nodes
                it->beam->p2 = it->lockNode;
                it->beam->p2truck = it->lockTruck != 0;
                it->beam->L = (it->hookNode->AbsPosition - it->lockNode->AbsPosition).length();
                it->beam->bm_disabled = false;
                addInterTruckBeam(it->beam, this, it->lockTruck);
            }
            else
            {
                if (it->beam->L < it->beam->commandShort)
                {
                    //shortlimit reached -> status LOCKED
                    it->locked = LOCKED;
                }
                else
                {
                    //shorten the connecting beam slowly to locking minrange
                    if (it->beam->L > it->lockspeed && fabs(it->beam->stress) < it->maxforce)
                    {
                        it->beam->L = (it->beam->L - it->lockspeed);
                    }
                    else
                    {
                        if (fabs(it->beam->stress) < it->maxforce)
                        {
                            it->beam->L = 0.001f;
                            //locking minrange or stress exeeded -> status LOCKED
                            it->locked = LOCKED;
                        }
                        else
                        {
                            if (it->nodisable)
                            {
                                //force exceed, but beam is set to nodisable, just lock it in this position
                                it->locked = LOCKED;
                            }
                            else
                            {
                                //force exceeded reset the hook node
                                it->beam->mSceneNode->detachAllObjects();
                                it->locked = UNLOCKED;
                                it->lockNode = 0;
                                it->lockTruck = 0;
                                it->beam->p2 = &ar_nodes[0];
                                it->beam->p2truck = false;
                                it->beam->L = (ar_nodes[0].AbsPosition - it->hookNode->AbsPosition).length();
                                it->beam->bm_disabled = true;
                                removeInterTruckBeam(it->beam);
                            }
                        }
                    }
                }
            }
        }
        if (it->locked == PREUNLOCK)
        {
            it->locked = UNLOCKED;
            removeInterTruckBeam(it->beam);
        }
    }
    BES_STOP(BES_CORE_Hooks);
}

void Beam::calcRopes()
{
    BES_START(BES_CORE_Ropes);
    if (ropes.size())
    {
        for (std::vector<rope_t>::iterator it = ropes.begin(); it != ropes.end(); it++)
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

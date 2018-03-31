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

#include "Application.h"
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
using namespace RoR;

// Param "doUpdate" means "also update things which should be updated only once per frame, not per every physics tick"
//     In this case, doUpdate is TRUE on first tick after rendering, FALSE in all other ticks 
void Actor::calcForcesEulerCompute(bool doUpdate, Real dt, int step, int maxsteps)
{
    IWater* water = nullptr;
    const bool is_player_actor = (this == RoR::App::GetSimController()->GetPlayerActor());

    if (App::GetSimTerrain())
        water = App::GetSimTerrain()->getWater();

    m_increased_accuracy = false;

    //engine callback
    if (ar_engine)
    {
        ar_engine->UpdateEngineSim(dt, doUpdate);
    }

    calcBeams(doUpdate, dt, step, maxsteps);

    if (doUpdate)
    {
        //just call this once per frame to avoid performance impact
        ToggleHooks(-2, HOOK_LOCK, -1);
    }

    //auto locks (scan just once per frame, need to use a timer)
    for (std::vector<hook_t>::iterator it = ar_hooks.begin(); it != ar_hooks.end(); it++)
    {
        //we need to do this here to avoid countdown speedup by triggers
        it->hk_timer = std::max(0.0f, it->hk_timer - dt);
    }

    if (is_player_actor) //force feedback sensors
    {
        if (doUpdate)
        {
            m_force_sensors.Reset();
        }

        if (ar_current_cinecam != -1)
        {
            m_force_sensors.accu_body_forces += ar_nodes[ar_camera_node_pos[ar_current_cinecam]].Forces;
        }

        for (int i = 0; i < ar_num_hydros; i++)
        {
            beam_t* hydrobeam = &ar_beams[ar_hydro[i]];
            if ((hydrobeam->hydroFlags & (HYDRO_FLAG_DIR | HYDRO_FLAG_SPEED)) && !hydrobeam->bm_broken)
            {
                m_force_sensors.accu_hydros_forces += hydrobeam->hydroRatio * hydrobeam->refL * hydrobeam->stress;
            }
        }
    }

    //mouse stuff
    if (m_mouse_grab_node != -1)
    {
        Vector3 dir = m_mouse_grab_pos - ar_nodes[m_mouse_grab_node].AbsPosition;
        ar_nodes[m_mouse_grab_node].Forces += m_mouse_grab_move_force * dir;
    }

    // START Slidenode section /////////////////////////////////////////////////
    // these must be done before the integrator, or else the forces are not calculated properly
    updateSlideNodeForces(dt);
    // END Slidenode section   /////////////////////////////////////////////////

    m_water_contact = false;

    calcNodes(doUpdate, dt, step, maxsteps);

    AxisAlignedBox tBoundingBox(ar_nodes[0].AbsPosition, ar_nodes[0].AbsPosition);

    for (unsigned int i = 0; i < ar_collision_bounding_boxes.size(); i++)
    {
        ar_collision_bounding_boxes[i].scale(Ogre::Vector3(0.0));
    }

    for (int i = 0; i < ar_num_nodes; i++)
    {
        tBoundingBox.merge(ar_nodes[i].AbsPosition);
        if (ar_nodes[i].collisionBoundingBoxID >= 0 && (unsigned int) ar_nodes[i].collisionBoundingBoxID < ar_collision_bounding_boxes.size())
        {
            AxisAlignedBox& bb = ar_collision_bounding_boxes[ar_nodes[i].collisionBoundingBoxID];
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

    for (unsigned int i = 0; i < ar_collision_bounding_boxes.size(); i++)
    {
        ar_collision_bounding_boxes[i].setMinimum(ar_collision_bounding_boxes[i].getMinimum() - Vector3(0.05f, 0.05f, 0.05f));
        ar_collision_bounding_boxes[i].setMaximum(ar_collision_bounding_boxes[i].getMaximum() + Vector3(0.05f, 0.05f, 0.05f));

        ar_predicted_coll_bounding_boxes[i].setExtents(ar_collision_bounding_boxes[i].getMinimum(), ar_collision_bounding_boxes[i].getMaximum());
        ar_predicted_coll_bounding_boxes[i].merge(ar_collision_bounding_boxes[i].getMinimum() + ar_nodes[0].Velocity);
        ar_predicted_coll_bounding_boxes[i].merge(ar_collision_bounding_boxes[i].getMaximum() + ar_nodes[0].Velocity);
    }

    // anti-explosion guard
    // rationale behind 1e9 number:
    // - while 1e6 is reachable by a fast vehicle, it will be badly deformed and shaking due to loss of precision in calculations
    // - at 1e7 any typical RoR vehicle falls apart and stops functioning
    // - 1e9 may be reachable only by a vehicle that is 1000 times bigger than a typical RoR vehicle, and it will be a loooong trip
    // to be able to travel such long distances will require switching physics calculations to higher precision numbers
    // or taking a different approach to the simulation (actor-local coordinate system?)
    if (!inRange(tBoundingBox.getMinimum().x + tBoundingBox.getMaximum().x +
        tBoundingBox.getMinimum().y + tBoundingBox.getMaximum().y +
        tBoundingBox.getMinimum().z + tBoundingBox.getMaximum().z, -1e9, 1e9))
    {
        m_reset_request = REQUEST_RESET_ON_INIT_POS; // actor exploded, schedule reset
        return; // return early to avoid propagating invalid values
    }

    ar_bounding_box.setMinimum(tBoundingBox.getMinimum() - Vector3(0.05f, 0.05f, 0.05f));
    ar_bounding_box.setMaximum(tBoundingBox.getMaximum() + Vector3(0.05f, 0.05f, 0.05f));

    ar_predicted_bounding_box.setExtents(ar_bounding_box.getMinimum(), ar_bounding_box.getMaximum());
    ar_predicted_bounding_box.merge(ar_bounding_box.getMinimum() + ar_nodes[0].Velocity);
    ar_predicted_bounding_box.merge(ar_bounding_box.getMaximum() + ar_nodes[0].Velocity);

    //turboprop forces
    for (int i = 0; i < ar_num_aeroengines; i++)
        if (ar_aeroengines[i])
            ar_aeroengines[i]->updateForces(dt, doUpdate);

    //screwprop forces
    for (int i = 0; i < ar_num_screwprops; i++)
        if (ar_screwprops[i])
            ar_screwprops[i]->updateForces(doUpdate);

    //wing forces
    for (int i = 0; i < ar_num_wings; i++)
        if (ar_wings[i].fa)
            ar_wings[i].fa->updateForces();

    //compute fuse drag
    if (m_fusealge_airfoil)
    {
        Vector3 wind = -m_fusealge_front->Velocity;
        float wspeed = wind.length();
        Vector3 axis = m_fusealge_front->RelPosition - m_fusealge_back->RelPosition;
        float s = axis.length() * m_fusealge_width;
        float cz, cx, cm;
        float v = axis.getRotationTo(wind).w;
        float aoa = 0;
        if (v < 1.0 && v > -1.0)
            aoa = 2.0 * acos(v); //quaternion fun
        m_fusealge_airfoil->getparams(aoa, 1.0, 0.0, &cz, &cx, &cm);

        //tropospheric model valid up to 11.000m (33.000ft)
        float altitude = m_fusealge_front->AbsPosition.y;

        // TODO Unused Varaible
        //float sea_level_temperature=273.15f+15.0f; //in Kelvin
        float sea_level_pressure = 101325; //in Pa

        // TODO Unused Varaible
        //float airtemperature=sea_level_temperature-altitude*0.0065f; //in Kelvin
        float airpressure = sea_level_pressure * approx_pow(1.0 - 0.0065 * altitude / 288.1, 5.24947); //in Pa
        float airdensity = airpressure * 0.0000120896f;//1.225 at sea level

        //fuselage as an airfoil + parasitic drag (half fuselage front surface almost as a flat plane!)
        ar_fusedrag = ((cx * s + m_fusealge_width * m_fusealge_width * 0.5) * 0.5 * airdensity * wspeed / ar_num_nodes) * wind; 
    }

    //airbrakes
    for (int i = 0; i < ar_num_airbrakes; i++)
    {
        ar_airbrakes[i]->applyForce();
    }

    //water buoyance
    if (ar_num_buoycabs && water)
    {
        for (int i = 0; i < ar_num_buoycabs; i++)
        {
            int tmpv = ar_buoycabs[i] * 3;
            m_buoyance->computeNodeForce(&ar_nodes[ar_cabs[tmpv]], &ar_nodes[ar_cabs[tmpv + 1]], &ar_nodes[ar_cabs[tmpv + 2]], doUpdate == 1, ar_buoycab_types[i]);
        }
    }

    //wheel speed
    Real wspeed = 0.0;
    //wheel stuff

    float engine_torque = 0.0;

    // calculate torque per wheel
    if (ar_engine && m_num_proped_wheels != 0)
        engine_torque = ar_engine->GetTorque() / m_num_proped_wheels;

    int propcounter = 0;
    float newspeeds[MAX_WHEELS] = {};
    float intertorque[MAX_WHEELS] = {};
    //old-style viscous code
    if (m_num_axles == 0)
    {
        //first, evaluate torque from inter-differential locking
        for (int i = 0; i < m_num_proped_wheels / 2 - 1; i++)
        {
            if (ar_wheels[m_proped_wheel_pairs[i * 2 + 0]].wh_is_detached)
                ar_wheels[m_proped_wheel_pairs[i * 2 + 0]].wh_speed = ar_wheels[m_proped_wheel_pairs[i * 2 + 1]].wh_speed;
            if (ar_wheels[m_proped_wheel_pairs[i * 2 + 1]].wh_is_detached)
                ar_wheels[m_proped_wheel_pairs[i * 2 + 1]].wh_speed = ar_wheels[m_proped_wheel_pairs[i * 2 + 0]].wh_speed;
            if (ar_wheels[m_proped_wheel_pairs[i * 2 + 2]].wh_is_detached)
                ar_wheels[m_proped_wheel_pairs[i * 2 + 2]].wh_speed = ar_wheels[m_proped_wheel_pairs[i * 2 + 3]].wh_speed;
            if (ar_wheels[m_proped_wheel_pairs[i * 2 + 3]].wh_is_detached)
                ar_wheels[m_proped_wheel_pairs[i * 2 + 3]].wh_speed = ar_wheels[m_proped_wheel_pairs[i * 2 + 2]].wh_speed;

            float speed1 = (ar_wheels[m_proped_wheel_pairs[i * 2 + 0]].wh_speed + ar_wheels[m_proped_wheel_pairs[i * 2 + 1]].wh_speed) * 0.5f;
            float speed2 = (ar_wheels[m_proped_wheel_pairs[i * 2 + 2]].wh_speed + ar_wheels[m_proped_wheel_pairs[i * 2 + 3]].wh_speed) * 0.5f;
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
    for (int i = 1; i < m_num_axles; i++)
    {
        if (!m_axles[i])
            continue;

        if (ar_wheels[m_axles[i - 1]->ax_wheel_1].wh_is_detached)
            ar_wheels[m_axles[i - 1]->ax_wheel_1].wh_speed = ar_wheels[m_axles[i - 1]->ax_wheel_2].wh_speed;
        if (ar_wheels[m_axles[i - 0]->ax_wheel_1].wh_is_detached)
            ar_wheels[m_axles[i - 0]->ax_wheel_1].wh_speed = ar_wheels[m_axles[i - 0]->ax_wheel_2].wh_speed;
        if (ar_wheels[m_axles[i - 1]->ax_wheel_2].wh_is_detached)
            ar_wheels[m_axles[i - 1]->ax_wheel_2].wh_speed = ar_wheels[m_axles[i - 1]->ax_wheel_1].wh_speed;
        if (ar_wheels[m_axles[i - 0]->ax_wheel_2].wh_is_detached)
            ar_wheels[m_axles[i - 0]->ax_wheel_2].wh_speed = ar_wheels[m_axles[i - 0]->ax_wheel_1].wh_speed;

        if (ar_wheels[m_axles[i - 1]->ax_wheel_1].wh_is_detached && ar_wheels[m_axles[i - 1]->ax_wheel_2].wh_is_detached)
        {
            ar_wheels[m_axles[i - 1]->ax_wheel_1].wh_speed = ar_wheels[m_axles[i - 0]->ax_wheel_1].wh_speed;
            ar_wheels[m_axles[i - 1]->ax_wheel_2].wh_speed = ar_wheels[m_axles[i - 0]->ax_wheel_2].wh_speed;
        }
        if (ar_wheels[m_axles[i - 0]->ax_wheel_1].wh_is_detached && ar_wheels[m_axles[i - 0]->ax_wheel_2].wh_is_detached)
        {
            ar_wheels[m_axles[i - 0]->ax_wheel_1].wh_speed = ar_wheels[m_axles[i - 1]->ax_wheel_1].wh_speed;
            ar_wheels[m_axles[i - 0]->ax_wheel_2].wh_speed = ar_wheels[m_axles[i - 1]->ax_wheel_2].wh_speed;
        }

        Ogre::Real axle_torques[2] = {0.0f, 0.0f};
        DifferentialData diff_data =
        {
            {
                (ar_wheels[m_axles[i - 1]->ax_wheel_1].wh_speed + ar_wheels[m_axles[i - 1]->ax_wheel_2].wh_speed) * 0.5f,
                (ar_wheels[m_axles[i]->ax_wheel_1].wh_speed + ar_wheels[m_axles[i]->ax_wheel_2].wh_speed) * 0.5f
            },
            m_axles[i - 1]->ax_delta_rotation,
            {axle_torques[0], axle_torques[1]},
            0, // no input torque, just calculate forces from different axle positions
            dt
        };

#if 0
        // use an open diff just for fun :)
		Axle::CalcOpenDiff( diff_data );
#else
        // use the locked diff, most vehicles are setup this way...
        Axle::CalcLockedDiff(diff_data);
#endif

        m_axles[i - 1]->ax_delta_rotation = diff_data.delta_rotation;
        m_axles[i]->ax_delta_rotation = -diff_data.delta_rotation;

        intertorque[m_axles[i - 1]->ax_wheel_1] = diff_data.out_torque[0];
        intertorque[m_axles[i - 1]->ax_wheel_2] = diff_data.out_torque[0];
        intertorque[m_axles[i]->ax_wheel_1] = diff_data.out_torque[1];
        intertorque[m_axles[i]->ax_wheel_2] = diff_data.out_torque[1];
    }

    // loop through all the wheels (new style again)
    for (int i = 0; i < m_num_axles; i++)
    {
        if (!m_axles[i])
            continue;
        Ogre::Real axle_torques[2] = {0.0f, 0.0f};
        wheel_t* axle_wheels[2] = {&ar_wheels[m_axles[i]->ax_wheel_1], &ar_wheels[m_axles[i]->ax_wheel_2]};

        if (axle_wheels[0]->wh_is_detached)
            axle_wheels[0]->wh_speed = axle_wheels[1]->wh_speed;
        if (axle_wheels[1]->wh_is_detached)
            axle_wheels[1]->wh_speed = axle_wheels[0]->wh_speed;

        DifferentialData diff_data =
        {
            {axle_wheels[0]->wh_speed, axle_wheels[1]->wh_speed},
            axle_wheels[0]->wh_delta_rotation,
            {axle_torques[0], axle_torques[1]},
            // twice the torque since this is for two wheels, plus extra torque from
            // inter-axle torsion
            2.0f * engine_torque + intertorque[m_axles[i]->ax_wheel_1],
            dt
        };

        m_axles[i]->CalcAxleTorque(diff_data);

        axle_wheels[0]->wh_delta_rotation = diff_data.delta_rotation;
        axle_wheels[1]->wh_delta_rotation = -diff_data.delta_rotation;

        intertorque[m_axles[i]->ax_wheel_1] = diff_data.out_torque[0];
        intertorque[m_axles[i]->ax_wheel_2] = diff_data.out_torque[1];
    }

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

    // fix for airplanes crashing when GetAcceleration() is used
    float currentAcc = 0.0f;
    if (ar_driveable == TRUCK && ar_engine)
    {
        currentAcc = ar_engine->GetAcceleration();
    }

    for (int i = 0; i < ar_num_wheels; i++)
    {
        Real speedacc = 0.0;

        // total torque estimation
        Real total_torque = 0.0;
        if (ar_wheels[i].wh_propulsed > 0)
        {
            total_torque = (m_num_axles == 0) ? engine_torque : intertorque[i];
        }

        if (ar_wheels[i].wh_braking != wheel_t::BrakeCombo::NONE)
        {
            // handbrake
            float hbrake = 0.0f;

            if (ar_parking_brake && (ar_wheels[i].wh_braking != wheel_t::BrakeCombo::FOOT_ONLY))
            {
                hbrake = m_handbrake_force;
            }

            // directional braking
            float dbrake = 0.0f;

            if ((ar_wheel_speed < 20.0f)
                && (((ar_wheels[i].wh_braking == wheel_t::BrakeCombo::FOOT_HAND_SKID_LEFT)  && (ar_hydro_dir_state > 0.0f))
                 || ((ar_wheels[i].wh_braking == wheel_t::BrakeCombo::FOOT_HAND_SKID_RIGHT) && (ar_hydro_dir_state < 0.0f))))
            {
                dbrake = ar_brake_force * abs(ar_hydro_dir_state);
            }

            if ((ar_brake != 0.0 || dbrake != 0.0 || hbrake != 0.0) && m_num_braked_wheels != 0 && fabs(ar_wheels[i].wh_speed) > 0.0f)
            {
                float brake_coef = 1.0f;
                float antilock_coef = 1.0f;
                // anti-lock braking
                if (alb_mode && alb_pulse_state && (ar_brake > 0.0f || dbrake > 0.0f) && curspeed > fabs(ar_wheels[i].wh_speed) && curspeed > alb_minspeed)
                {
                    antilock_coef = fabs(ar_wheels[i].wh_speed) / curspeed;
                    antilock_coef = pow(antilock_coef, alb_ratio);
                    alb_active = (antilock_coef < 0.9);
                }
                if (fabs(ar_wheels[i].wh_speed) < 1.0f)
                {
                    if (ar_wheels[i].firstLock)
                    {
                        ar_wheels[i].wh_avg_speed = 0.0f;
                        ar_wheels[i].firstLock = false;
                    }
                    // anti-jitter
                    if (fabs(ar_wheels[i].wh_avg_speed) < 2.0f)
                    {
                        brake_coef = pow(fabs(ar_wheels[i].wh_speed), 2.0f);
                    }
                    else
                    {
                        brake_coef = pow(fabs(ar_wheels[i].wh_speed), 0.5f);
                    }
                    // anti-skidding
                    ar_wheels[i].wh_avg_speed += ar_wheels[i].wh_speed;
                    ar_wheels[i].wh_avg_speed = std::max(-10.0f, std::min(ar_wheels[i].wh_avg_speed, 10.0f));
                    float speed_diff = ar_wheels[i].wh_speed - ar_wheels[i].wh_last_speed;
                    float speed_prediction = ar_wheels[i].wh_speed + 0.5f * speed_diff;
                    if (speed_prediction * ar_wheels[i].wh_avg_speed < 0.0f)
                    {
                        brake_coef = 0.0f;
                    }
                }
                else
                {
                    ar_wheels[i].firstLock = true;
                }

                if (ar_wheels[i].wh_speed > 0)
                    total_torque -= ((ar_brake + dbrake) * antilock_coef + hbrake) * brake_coef;
                else
                    total_torque += ((ar_brake + dbrake) * antilock_coef + hbrake) * brake_coef;
            }
        }
        else
        {
            ar_wheels[i].firstLock = true;
        }

        // traction control
        if (tc_mode && tc_pulse_state && ar_wheels[i].wh_propulsed > 0 && currentAcc > 0.0f && fabs(ar_wheels[i].wh_speed) > curspeed)
        {
            curspeed = std::max(0.5f, curspeed);

            // tc_wheelslip = allowed amount of slip in percent
            float wheelslip = 1.0f + tc_wheelslip;
            // wheelslip allowed doubles up to tc_fade, a tribute to RoRs wheelspeed calculation and friction
            wheelslip += tc_wheelslip * (curspeed / tc_fade);

            if (fabs(ar_wheels[i].wh_speed) > curspeed * wheelslip)
            {
                float torque_coef = (curspeed * wheelslip) / fabs(ar_wheels[i].wh_speed);
                torque_coef = pow(torque_coef, tc_ratio);
                total_torque *= torque_coef;
                tc_active = (torque_coef < 0.9f);
            }
        }

        // old-style
        if (m_num_axles == 0 && ar_wheels[i].wh_propulsed > 0)
        {
            // differential locking
            if (i % 2)
                if (!ar_wheels[i].wh_is_detached && !ar_wheels[i - 1].wh_is_detached)
                    total_torque -= (ar_wheels[i].wh_speed - ar_wheels[i - 1].wh_speed) * 10000.0;
                else if (!ar_wheels[i].wh_is_detached && !ar_wheels[i + 1].wh_is_detached)
                    total_torque -= (ar_wheels[i].wh_speed - ar_wheels[i + 1].wh_speed) * 10000.0;
            // inter differential locking
            total_torque += intertorque[propcounter];
            propcounter++;
        }

        if (ar_wheels[i].wh_is_detached)
            continue;

        // application to wheel
        Vector3 axis = ar_wheels[i].wh_axis_node_1->RelPosition - ar_wheels[i].wh_axis_node_0->RelPosition;
        float axis_precalc = total_torque / (Real)(ar_wheels[i].wh_num_nodes);
        axis = fast_normalise(axis);

        for (int j = 0; j < ar_wheels[i].wh_num_nodes; j++)
        {
            Vector3 radius(Vector3::ZERO);

            if (j % 2)
                radius = ar_wheels[i].wh_nodes[j]->RelPosition - ar_wheels[i].wh_axis_node_1->RelPosition;
            else
                radius = ar_wheels[i].wh_nodes[j]->RelPosition - ar_wheels[i].wh_axis_node_0->RelPosition;

            float inverted_rlen = fast_invSqrt(radius.squaredLength());

            if (ar_wheels[i].wh_propulsed == 2)
            {
                radius = -radius;
            }

            Vector3 dir = axis.crossProduct(radius);
            ar_wheels[i].wh_nodes[j]->Forces += dir * (axis_precalc * inverted_rlen * inverted_rlen);
            //wheel speed
            if (j % 2)
                speedacc += (ar_wheels[i].wh_nodes[j]->Velocity - ar_wheels[i].wh_axis_node_1->Velocity).dotProduct(dir) * inverted_rlen;
            else
                speedacc += (ar_wheels[i].wh_nodes[j]->Velocity - ar_wheels[i].wh_axis_node_0->Velocity).dotProduct(dir) * inverted_rlen;
        }
        // wheel speed
        newspeeds[i] = speedacc / ar_wheels[i].wh_num_nodes;
        if (ar_wheels[i].wh_propulsed == 1)
        {
            wspeed += newspeeds[i];
        }
        // for network
        ar_wheels[i].wh_net_rp += (newspeeds[i] / ar_wheels[i].wh_radius) * dt;
        // reaction torque
        Vector3 rradius = ar_wheels[i].wh_arm_node->RelPosition - ar_wheels[i].wh_near_attach_node->RelPosition;
        Vector3 radius = Plane(axis, ar_wheels[i].wh_near_attach_node->RelPosition).projectVector(rradius);
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
            ar_wheels[i].wh_arm_node->Forces -= cforce;
            ar_wheels[i].wh_near_attach_node->Forces += cforce;
        }
    }

    // dashboard overlays for tc+alb
    if (doUpdate)
    {
        m_antilockbrake = false;
        m_tractioncontrol = false;
    }

    m_antilockbrake = std::max(m_antilockbrake, (int)alb_active);
    m_tractioncontrol = std::max(m_tractioncontrol, (int)tc_active);

    if (step == maxsteps)
    {
        if (!m_antilockbrake)
        {
            SOUND_STOP(ar_instance_id, SS_TRIG_ALB_ACTIVE);
        }
        else
        {
            SOUND_START(ar_instance_id, SS_TRIG_ALB_ACTIVE);
        }

        if (!m_tractioncontrol)
        {
            SOUND_STOP(ar_instance_id, SS_TRIG_TC_ACTIVE);
        }
        else
        {
            SOUND_START(ar_instance_id, SS_TRIG_TC_ACTIVE);
        }
    }

    for (int i = 0; i < ar_num_wheels; i++)
    {
        ar_wheels[i].wh_last_speed = ar_wheels[i].wh_speed;
        ar_wheels[i].wh_speed = newspeeds[i];
    }
    if (m_num_proped_wheels)
    {
        wspeed /= (float)m_num_proped_wheels;
    }

    // wheel speed  in m/s !
    ar_wheel_speed = wspeed;

    if (ar_engine && ar_num_wheels && ar_wheels[0].wh_radius > 0.0f)
    {
        ar_engine->SetWheelSpin(wspeed / ar_wheels[0].wh_radius * RAD_PER_SEC_TO_RPM);
    }

    // calculate driven distance
    float distance_driven = fabs(wspeed * dt);
    m_odometer_total += distance_driven;
    m_odometer_user += distance_driven;

    //variable shocks for stabilization
    if (this->ar_has_active_shocks && m_stabilizer_shock_request)
    {
        if ((m_stabilizer_shock_request == 1 && m_stabilizer_shock_ratio < 0.1) || (m_stabilizer_shock_request == -1 && m_stabilizer_shock_ratio > -0.1))
            m_stabilizer_shock_ratio = m_stabilizer_shock_ratio + (float)m_stabilizer_shock_request * dt * STAB_RATE;
        for (int i = 0; i < ar_num_shocks; i++)
        {
            // active shocks now
            if (ar_shocks[i].flags & SHOCK_FLAG_RACTIVE)
                ar_beams[ar_shocks[i].beamid].L = ar_beams[ar_shocks[i].beamid].refL * (1.0 + m_stabilizer_shock_ratio);
            else if (ar_shocks[i].flags & SHOCK_FLAG_LACTIVE)
                ar_beams[ar_shocks[i].beamid].L = ar_beams[ar_shocks[i].beamid].refL * (1.0 - m_stabilizer_shock_ratio);
        }
    }
    //auto shock adjust
    if (this->ar_has_active_shocks && doUpdate)
    {
        m_stabilizer_shock_sleep -= dt * maxsteps;

        Vector3 dir = ar_nodes[ar_camera_node_pos[0]].RelPosition - ar_nodes[ar_camera_node_roll[0]].RelPosition;
        dir.normalise();
        float roll = asin(dir.dotProduct(Vector3::UNIT_Y));
        //mWindow->setDebugText("Roll:"+ TOSTRING(roll));
        if (fabs(roll) > 0.2)
        {
            m_stabilizer_shock_sleep = -1.0; //emergency timeout stop
        }
        if (fabs(roll) > 0.01 && m_stabilizer_shock_sleep < 0.0)
        {
            if (roll > 0.0 && m_stabilizer_shock_request != -1)
            {
                m_stabilizer_shock_request = 1;
            }
            else if (roll < 0.0 && m_stabilizer_shock_request != 1)
            {
                m_stabilizer_shock_request = -1;
            }
            else
            {
                m_stabilizer_shock_request = 0;
                m_stabilizer_shock_sleep = 3.0;
            }
        }
        else
        {
            m_stabilizer_shock_request = 0;
        }

        if (m_stabilizer_shock_request && fabs(m_stabilizer_shock_ratio) < 0.1)
            SOUND_START(ar_instance_id, SS_TRIG_AIR);
        else
            SOUND_STOP(ar_instance_id, SS_TRIG_AIR);
    }

    //direction
    if (ar_hydro_dir_state != 0 || ar_hydro_dir_command != 0)
    {
        float rate = 1;
        if (ar_hydro_speed_coupling)
        {
            //new rate value (30 instead of 40) to deal with the changed cmdKeyInertia settings
            rate = 30.0 / (10.0 + fabs(wspeed / 2.0));
            // minimum rate: 20% --> enables to steer high velocity vehicles
            if (rate < 1.2)
                rate = 1.2;
        }
        //need a maximum rate for analog devices, otherwise hydro beams break
        if (!ar_hydro_speed_coupling)
        {
            float hydrodirstateOld = ar_hydro_dir_state;
            ar_hydro_dir_state = ar_hydro_dir_command;
            if (abs(ar_hydro_dir_state - hydrodirstateOld) > 0.02)
            {
                ar_hydro_dir_state = (ar_hydro_dir_state - hydrodirstateOld) * 0.02 + hydrodirstateOld;
            }
        }
        if (ar_hydro_dir_command != 0 && ar_hydro_speed_coupling)
        {
            if (ar_hydro_dir_state > ar_hydro_dir_command)
                ar_hydro_dir_state -= dt * rate;
            else
                ar_hydro_dir_state += dt * rate;
        }
        if (ar_hydro_speed_coupling)
        {
            float dirdelta = dt;
            if (ar_hydro_dir_state > dirdelta)
                ar_hydro_dir_state -= dirdelta;
            else if (ar_hydro_dir_state < -dirdelta)
                ar_hydro_dir_state += dirdelta;
            else
                ar_hydro_dir_state = 0;
        }
    }
    //aileron
    if (ar_hydro_aileron_state != 0 || ar_hydro_aileron_command != 0)
    {
        if (ar_hydro_aileron_command != 0)
        {
            if (ar_hydro_aileron_state > ar_hydro_aileron_command)
                ar_hydro_aileron_state -= dt * 4.0;
            else
                ar_hydro_aileron_state += dt * 4.0;
        }
        float delta = dt;
        if (ar_hydro_aileron_state > delta)
            ar_hydro_aileron_state -= delta;
        else if (ar_hydro_aileron_state < -delta)
            ar_hydro_aileron_state += delta;
        else
            ar_hydro_aileron_state = 0;
    }
    //rudder
    if (ar_hydro_rudder_state != 0 || ar_hydro_rudder_command != 0)
    {
        if (ar_hydro_rudder_command != 0)
        {
            if (ar_hydro_rudder_state > ar_hydro_rudder_command)
                ar_hydro_rudder_state -= dt * 4.0;
            else
                ar_hydro_rudder_state += dt * 4.0;
        }

        float delta = dt;
        if (ar_hydro_rudder_state > delta)
            ar_hydro_rudder_state -= delta;
        else if (ar_hydro_rudder_state < -delta)
            ar_hydro_rudder_state += delta;
        else
            ar_hydro_rudder_state = 0;
    }
    //elevator
    if (ar_hydro_elevator_state != 0 || ar_hydro_elevator_command != 0)
    {
        if (ar_hydro_elevator_command != 0)
        {
            if (ar_hydro_elevator_state > ar_hydro_elevator_command)
                ar_hydro_elevator_state -= dt * 4.0;
            else
                ar_hydro_elevator_state += dt * 4.0;
        }
        float delta = dt;
        if (ar_hydro_elevator_state > delta)
            ar_hydro_elevator_state -= delta;
        else if (ar_hydro_elevator_state < -delta)
            ar_hydro_elevator_state += delta;
        else
            ar_hydro_elevator_state = 0;
    }
    //update length, dirstate between -1.0 and 1.0
    for (int i = 0; i < ar_num_hydros; i++)
    {
        //compound hydro
        float cstate = 0.0f;
        int div = 0;
        if (ar_beams[ar_hydro[i]].hydroFlags & HYDRO_FLAG_SPEED)
        {
            //special treatment for SPEED
            if (ar_wheel_speed < 12.0f)
                cstate += ar_hydro_dir_state * (12.0f - ar_wheel_speed) / 12.0f;
            div++;
        }
        if (ar_beams[ar_hydro[i]].hydroFlags & HYDRO_FLAG_DIR)
        {
            cstate += ar_hydro_dir_state;
            div++;
        }
        if (ar_beams[ar_hydro[i]].hydroFlags & HYDRO_FLAG_AILERON)
        {
            cstate += ar_hydro_aileron_state;
            div++;
        }
        if (ar_beams[ar_hydro[i]].hydroFlags & HYDRO_FLAG_RUDDER)
        {
            cstate += ar_hydro_rudder_state;
            div++;
        }
        if (ar_beams[ar_hydro[i]].hydroFlags & HYDRO_FLAG_ELEVATOR)
        {
            cstate += ar_hydro_elevator_state;
            div++;
        }
        if (ar_beams[ar_hydro[i]].hydroFlags & HYDRO_FLAG_REV_AILERON)
        {
            cstate -= ar_hydro_aileron_state;
            div++;
        }
        if (ar_beams[ar_hydro[i]].hydroFlags & HYDRO_FLAG_REV_RUDDER)
        {
            cstate -= ar_hydro_rudder_state;
            div++;
        }
        if (ar_beams[ar_hydro[i]].hydroFlags & HYDRO_FLAG_REV_ELEVATOR)
        {
            cstate -= ar_hydro_elevator_state;
            div++;
        }

        if (cstate > 1.0)
            cstate = 1.0;
        if (cstate < -1.0)
            cstate = -1.0;
        // Animators following, if no animator, skip all the tests...
        int flagstate = ar_beams[ar_hydro[i]].animFlags;
        if (flagstate)
        {
            float animoption = ar_beams[ar_hydro[i]].animOption;
            calcAnimators(flagstate, cstate, div, dt, 0.0f, 0.0f, animoption);
        }

        if (div)
        {
            cstate /= (float)div;

            if (m_hydro_inertia)
                cstate = m_hydro_inertia->calcCmdKeyDelay(cstate, i, dt);

            if (!(ar_beams[ar_hydro[i]].hydroFlags & HYDRO_FLAG_SPEED) && !flagstate)
                ar_hydro_dir_wheel_display = cstate;

            float factor = 1.0 - cstate * ar_beams[ar_hydro[i]].hydroRatio;

            // check and apply animators limits if set
            if (flagstate)
            {
                if (factor < 1.0f - ar_beams[ar_hydro[i]].shortbound)
                    factor = 1.0f - ar_beams[ar_hydro[i]].shortbound;
                if (factor > 1.0f + ar_beams[ar_hydro[i]].longbound)
                    factor = 1.0f + ar_beams[ar_hydro[i]].longbound;
            }

            ar_beams[ar_hydro[i]].L = ar_beams[ar_hydro[i]].Lhydro * factor;
        }
    }

    // commands
    if (m_has_command_beams)
    {
        int active = 0;
        bool requested = false;
        float work = 0.0;

        // hydraulics ready?
        if (ar_engine)
            ar_engine_hydraulics_ready = ar_engine->GetEngineRpm() > ar_engine->getIdleRPM() * 0.95f;
        else
            ar_engine_hydraulics_ready = true;

        // crankfactor
        float crankfactor = 1.0f;
        if (ar_engine)
            crankfactor = ar_engine->GetCrankFactor();

        // speed up machines
        if (ar_driveable == MACHINE)
            crankfactor = 2;

        for (int i = 0; i <= MAX_COMMANDS; i++)
        {
            for (int j = 0; j < (int)ar_command_key[i].beams.size(); j++)
            {
                int k = abs(ar_command_key[i].beams[j]);
                if (k >= 0 && k < ar_num_beams)
                {
                    ar_beams[k].autoMoveLock = false;
                }
            }
        }

        for (int i = 0; i <= MAX_COMMANDS; i++)
        {
            float oldValue = ar_command_key[i].commandValue;

            ar_command_key[i].commandValue = std::max(ar_command_key[i].playerInputValue, ar_command_key[i].triggerInputValue);

            ar_command_key[i].triggerInputValue = 0.0f;

            if (ar_command_key[i].commandValue > 0.01f && oldValue < 0.01f)
            {
                // just started
                ar_command_key[i].commandValueState = 1;
            }
            else if (ar_command_key[i].commandValue < 0.01f && oldValue > 0.01f)
            {
                // just stopped
                ar_command_key[i].commandValueState = -1;
            }

            for (int j = 0; j < (int)ar_command_key[i].beams.size(); j++)
            {
                if (ar_command_key[i].commandValue >= 0.5)
                {
                    int k = abs(ar_command_key[i].beams[j]);
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
            for (int j = 0; j < (int)ar_command_key[i].beams.size(); j++)
            {
                int bbeam_dir = (ar_command_key[i].beams[j] > 0) ? 1 : -1;
                int bbeam = std::abs(ar_command_key[i].beams[j]);

                if (bbeam > ar_num_beams)
                    continue;

                // restrict forces
                if (ar_beams[bbeam].isForceRestricted)
                    crankfactor = std::min(crankfactor, 1.0f);

                float v = ar_command_key[i].commandValue;
                int& vst = ar_command_key[i].commandValueState;

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

                        if (m_command_inertia)
                            v = m_command_inertia->calcCmdKeyDelay(v, i, dt);

                        if (bbeam_dir * ar_beams[bbeam].autoMovingMode > 0)
                            v = 1;

                        if (ar_beams[bbeam].commandNeedsEngine && ((ar_engine && !ar_engine->IsRunning()) || !ar_engine_hydraulics_ready))
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
                                SoundScriptManager::getSingleton().trigStop(ar_instance_id, SS_TRIG_LINKED_COMMAND, SL_COMMAND, -i);
                                SoundScriptManager::getSingleton().trigStart(ar_instance_id, SS_TRIG_LINKED_COMMAND, SL_COMMAND, i);
                                vst = 0;
                            }
                            else if (vst == -1)
                            {
                                // just stopped
                                SoundScriptManager::getSingleton().trigStop(ar_instance_id, SS_TRIG_LINKED_COMMAND, SL_COMMAND, i);
                                vst = 0;
                            }
                            else if (vst == 0)
                            {
                                // already running, modulate
                                SoundScriptManager::getSingleton().modulate(ar_instance_id, SS_MOD_LINKED_COMMANDRATE, v, SL_COMMAND, i);
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
            for (int j = 0; j < (int)ar_command_key[i].rotators.size(); j++)
            {
                float v = 0.0f;
                int rota = std::abs(ar_command_key[i].rotators[j]) - 1;

                if (ar_rotators[rota].rotatorNeedsEngine && ((ar_engine && !ar_engine->IsRunning()) || !ar_engine_hydraulics_ready))
                    continue;

                if (m_rotator_inertia)
                {
                    v = m_rotator_inertia->calcCmdKeyDelay(ar_command_key[i].commandValue, i, dt);

                    if (v > 0.0f && ar_rotators[rota].rotatorEngineCoupling > 0.0f)
                        requestpower = true;
                }

                float cf = 1.0f;

                if (ar_rotators[rota].rotatorEngineCoupling > 0.0f)
                    cf = crankfactor;

                if (ar_command_key[i].rotators[j] > 0)
                    ar_rotators[rota].angle += ar_rotators[rota].rate * v * cf * dt;
                else
                    ar_rotators[rota].angle -= ar_rotators[rota].rate * v * cf * dt;
            }
            if (requestpower)
                requested=true;
        }

        if (ar_engine)
        {
            ar_engine->SetHydroPumpWork(work);
            ar_engine->SetEnginePriming(requested);
        }

        if (doUpdate && is_player_actor)
        {
#ifdef USE_OPENAL
            if (active > 0)
            {
                SOUND_START(ar_instance_id, SS_TRIG_PUMP);
                float pump_rpm = 660.0f * (1.0f - (work / (float)active) / 100.0f);
                SOUND_MODULATE(ar_instance_id, SS_MOD_PUMP, pump_rpm);
            }
            else
            {
                SOUND_STOP(ar_instance_id, SS_TRIG_PUMP);
            }
#endif //USE_OPENAL
        }
        // rotators
        for (int i = 0; i < ar_num_rotators; i++)
        {
            // compute rotation axis
            Vector3 axis = ar_nodes[ar_rotators[i].axis1].RelPosition - ar_nodes[ar_rotators[i].axis2].RelPosition;
            //axis.normalise();
            axis = fast_normalise(axis);
            // find the reference plane
            Plane pl = Plane(axis, 0);
            // for each pairar
            for (int k = 0; k < 2; k++)
            {
                // find the reference vectors
                Vector3 ref1 = pl.projectVector(ar_nodes[ar_rotators[i].axis2].RelPosition - ar_nodes[ar_rotators[i].nodes1[k]].RelPosition);
                Vector3 ref2 = pl.projectVector(ar_nodes[ar_rotators[i].axis2].RelPosition - ar_nodes[ar_rotators[i].nodes2[k]].RelPosition);
                // theory vector
                Vector3 th1 = Quaternion(Radian(ar_rotators[i].angle + 3.14159 / 2.0), axis) * ref1;
                // find the angle error
                float aerror = asin((th1.normalisedCopy()).dotProduct(ref2.normalisedCopy()));
                //mWindow->setDebugText("Error:"+ TOSTRING(aerror));
                // exert forces
                float rigidity = ar_rotators[i].force;
                Vector3 dir1 = ref1.crossProduct(axis);
                //dir1.normalise();
                dir1 = fast_normalise(dir1);
                Vector3 dir2 = ref2.crossProduct(axis);
                //dir2.normalise();
                dir2 = fast_normalise(dir2);
                float ref1len = ref1.length();
                float ref2len = ref2.length();

                // simple jitter fix
                if (ref1len <= ar_rotators[i].tolerance)
                    ref1len = 0.0f;
                if (ref2len <= ar_rotators[i].tolerance)
                    ref2len = 0.0f;

                ar_nodes[ar_rotators[i].nodes1[k]].Forces += (aerror * ref1len * rigidity) * dir1;
                ar_nodes[ar_rotators[i].nodes2[k]].Forces -= (aerror * ref2len * rigidity) * dir2;
                // symmetric
                ar_nodes[ar_rotators[i].nodes1[k + 2]].Forces -= (aerror * ref1len * rigidity) * dir1;
                ar_nodes[ar_rotators[i].nodes2[k + 2]].Forces += (aerror * ref2len * rigidity) * dir2;
            }
        }
    }

    // go through all ties and process them
    for (std::vector<tie_t>::iterator it = ar_ties.begin(); it != ar_ties.end(); it++)
    {
        // only process tying ties
        if (!it->ti_tying)
            continue;

        // division through zero guard
        if (it->ti_beam->refL == 0 || it->ti_beam->L == 0)
            continue;

        float clen = it->ti_beam->L / it->ti_beam->refL;
        if (clen > it->ti_beam->commandShort)
        {
            it->ti_beam->L *= (1.0 - it->ti_beam->commandRatioShort * dt / it->ti_beam->L);
        }
        else
        {
            // tying finished, end reached
            it->ti_tying = false;
        }

        // check if we hit a certain force limit, then abort the tying process
        if (fabs(it->ti_beam->stress) > it->ti_beam->maxtiestress)
            it->ti_tying = false;
    }

    // we also store a new replay frame
    if (m_replay_handler && m_replay_handler->isValid())
    {
        m_replay_timer += dt;
        if (m_replay_timer > ar_replay_precision)
        {
            // store nodes
            node_simple_t* nbuff = (node_simple_t *)m_replay_handler->getWriteBuffer(0);
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
            beam_simple_t* bbuff = (beam_simple_t *)m_replay_handler->getWriteBuffer(1);
            if (bbuff)
            {
                for (int i = 0; i < ar_num_beams; i++)
                {
                    bbuff[i].broken = ar_beams[i].bm_broken;
                    bbuff[i].disabled = ar_beams[i].bm_disabled;
                }
            }

            m_replay_handler->writeDone();
            m_replay_timer = 0.0f;
        }
    }
}

bool Actor::CalcForcesEulerPrepare(int doUpdate, Ogre::Real dt, int step, int maxsteps)
{
    if (dt == 0.0)
        return false;
    if (m_reset_request)
        return false;
    if (ar_sim_state != Actor::SimState::LOCAL_SIMULATED)
        return false;

    forwardCommands();
    CalcBeamsInterActor(doUpdate, dt, step, maxsteps);

    return true;
}

void Actor::calcForcesEulerFinal(int doUpdate, Ogre::Real dt, int step, int maxsteps)
{
    calcHooks();
    calcRopes();
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

void Actor::calcBeams(int doUpdate, Ogre::Real dt, int step, int maxsteps)
{
    // HOT DATA: 
    //  node_t:
    //     RelPosition, velocity, Forces
    //  beam_t:
    //     bm_disabled, bm_inter_actor
    //     L, k, d
    //     bounded, stress, minmaxposnegstress
    //  <>beam-bounded-SHOCK1:
    //     longbound, shortbound, bm_type, shock(ptr)
    //  <>beam-bounded-SHOCK2
    //     !! function-call !!
    //  <>beam-bounded-SUPPORTBEAM:
    //     longbound
    //  <>beam ~ deformation:
    //     bm_type, bounded, maxposstress, maxnegstress, strength, plastic_coef, detacher_group

    // Springs
    for (int i = 0; i < ar_num_beams; i++)
    {
        if (!ar_beams[i].bm_disabled && !ar_beams[i].bm_inter_actor)
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
                        if (m_beam_break_debug_enabled)
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
                        m_increased_accuracy = true;
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
                        if (m_beam_deform_debug_enabled)
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
                        m_increased_accuracy = true;
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
                        if (m_beam_deform_debug_enabled)
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
                    SOUND_MODULATE(ar_instance_id, SS_MOD_BREAK, 0.5 * k * difftoBeamL * difftoBeamL);
                    SOUND_PLAY_ONCE(ar_instance_id, SS_TRIG_BREAK);

                    m_increased_accuracy = true;

                    //Break the beam only when it is not connected to a node
                    //which is a part of a collision triangle and has 2 "live" beams or less
                    //connected to it.
                    if (!((ar_beams[i].p1->contacter && GetNumActiveConnectedBeams(ar_beams[i].p1->pos) < 3) || (ar_beams[i].p2->contacter && GetNumActiveConnectedBeams(ar_beams[i].p2->pos) < 3)))
                    {
                        slen = 0.0f;
                        ar_beams[i].bm_broken = true;
                        ar_beams[i].bm_disabled = true;

                        if (m_beam_break_debug_enabled)
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
                                    if (m_beam_break_debug_enabled)
                                    {
                                        LOG("Deleting Detacher BeamID: " + TOSTRING(j) + ", Detacher Group: " + TOSTRING(ar_beams[i].detacher_group)+ ", actor ID: " + TOSTRING(ar_instance_id));
                                    }
                                }
                            }
                            // cycle once through all wheels
                            for (int j = 0; j < ar_num_wheels; j++)
                            {
                                if (ar_wheels[j].wh_detacher_group == ar_beams[i].detacher_group)
                                {
                                    ar_wheels[j].wh_is_detached = true;
                                }
                            }
                        }
                    }
                    else
                    {
                        ar_beams[i].strength = 2.0f * ar_beams[i].minmaxposnegstress;
                    }

                    // something broke, check buoyant hull
                    for (int mk = 0; mk < ar_num_buoycabs; mk++)
                    {
                        int tmpv = ar_buoycabs[mk] * 3;
                        if (ar_buoycab_types[mk] == Buoyance::BUOY_DRAGONLY)
                            continue;
                        if ((ar_beams[i].p1 == &ar_nodes[ar_cabs[tmpv]] || ar_beams[i].p1 == &ar_nodes[ar_cabs[tmpv + 1]] || ar_beams[i].p1 == &ar_nodes[ar_cabs[tmpv + 2]]) &&
                            (ar_beams[i].p2 == &ar_nodes[ar_cabs[tmpv]] || ar_beams[i].p2 == &ar_nodes[ar_cabs[tmpv + 1]] || ar_beams[i].p2 == &ar_nodes[ar_cabs[tmpv + 2]]))
                        {
                            m_buoyance->setsink(1);
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
}

void Actor::CalcBeamsInterActor(int doUpdate, Ogre::Real dt, int step, int maxsteps)
{
    for (int i = 0; i < static_cast<int>(ar_inter_beams.size()); i++)
    {
        if (!ar_inter_beams[i]->bm_disabled && ar_inter_beams[i]->bm_inter_actor)
        {
            // Calculate beam length
            Vector3 dis = ar_inter_beams[i]->p1->AbsPosition - ar_inter_beams[i]->p2->AbsPosition;

            Real dislen = dis.squaredLength();
            Real inverted_dislen = fast_invSqrt(dislen);

            dislen *= inverted_dislen;

            // Calculate beam's deviation from normal
            Real difftoBeamL = dislen - ar_inter_beams[i]->L;

            Real k = ar_inter_beams[i]->k;
            Real d = ar_inter_beams[i]->d;

            if (ar_inter_beams[i]->bounded == ROPE && difftoBeamL < 0.0f)
            {
                k = 0.0f;
                d *= 0.1f;
            }

            // Calculate beam's rate of change
            Vector3 v = ar_inter_beams[i]->p1->Velocity - ar_inter_beams[i]->p2->Velocity;

            float slen = -k * (difftoBeamL) - d * v.dotProduct(dis) * inverted_dislen;
            ar_inter_beams[i]->stress = slen;

            // Fast test for deformation
            float len = std::abs(slen);
            if (len > ar_inter_beams[i]->minmaxposnegstress)
            {
                if ((ar_inter_beams[i]->bm_type == BEAM_NORMAL || ar_inter_beams[i]->bm_type == BEAM_INVISIBLE) && ar_inter_beams[i]->bounded != SHOCK1 && k != 0.0f)
                {
                    // Actual deformation tests
                    if (slen > ar_inter_beams[i]->maxposstress && difftoBeamL < 0.0f) // compression
                    {
                        Real yield_length = ar_inter_beams[i]->maxposstress / k;
                        Real deform = difftoBeamL + yield_length * (1.0f - ar_inter_beams[i]->plastic_coef);
                        Real Lold = ar_inter_beams[i]->L;
                        ar_inter_beams[i]->L += deform;
                        ar_inter_beams[i]->L = std::max(MIN_BEAM_LENGTH, ar_inter_beams[i]->L);
                        slen = slen - (slen - ar_inter_beams[i]->maxposstress) * 0.5f;
                        len = slen;
                        if (ar_inter_beams[i]->L > 0.0f && Lold > ar_inter_beams[i]->L)
                        {
                            ar_inter_beams[i]->maxposstress *= Lold / ar_inter_beams[i]->L;
                            ar_inter_beams[i]->minmaxposnegstress = std::min(ar_inter_beams[i]->maxposstress, -ar_inter_beams[i]->maxnegstress);
                            ar_inter_beams[i]->minmaxposnegstress = std::min(ar_inter_beams[i]->minmaxposnegstress, ar_inter_beams[i]->strength);
                        }
                        // For the compression case we do not remove any of the beam's
                        // strength for structure stability reasons
                        //ar_inter_beams[i]->strength += deform * k * 0.5f;
                        if (m_beam_deform_debug_enabled)
                        {
                            RoR::Str<300> msg;
                            msg << "[RoR|Diag] YYY Beam " << i << " just deformed with extension force "
                                << len << " / " << ar_inter_beams[i]->strength << ". ";
                            LogBeamNodes(msg, (*ar_inter_beams[i]));
                            RoR::Log(msg.ToCStr());
                        }
                    }
                    else if (slen < ar_inter_beams[i]->maxnegstress && difftoBeamL > 0.0f) // expansion
                    {
                        Real yield_length = ar_inter_beams[i]->maxnegstress / k;
                        Real deform = difftoBeamL + yield_length * (1.0f - ar_inter_beams[i]->plastic_coef);
                        Real Lold = ar_inter_beams[i]->L;
                        ar_inter_beams[i]->L += deform;
                        slen = slen - (slen - ar_inter_beams[i]->maxnegstress) * 0.5f;
                        len = -slen;
                        if (Lold > 0.0f && ar_inter_beams[i]->L > Lold)
                        {
                            ar_inter_beams[i]->maxnegstress *= ar_inter_beams[i]->L / Lold;
                            ar_inter_beams[i]->minmaxposnegstress = std::min(ar_inter_beams[i]->maxposstress, -ar_inter_beams[i]->maxnegstress);
                            ar_inter_beams[i]->minmaxposnegstress = std::min(ar_inter_beams[i]->minmaxposnegstress, ar_inter_beams[i]->strength);
                        }
                        ar_inter_beams[i]->strength -= deform * k;
                        if (m_beam_deform_debug_enabled)
                        {
                            RoR::Str<300> msg;
                            msg << "[RoR|Diag] YYY Beam " << i << " just deformed with extension force "
                                << len << " / " << ar_inter_beams[i]->strength << ". ";
                            LogBeamNodes(msg, (*ar_inter_beams[i]));
                            RoR::Log(msg.ToCStr());
                        }
                    }
                }

                // Test if the beam should break
                if (len > ar_inter_beams[i]->strength)
                {
                    // Sound effect.
                    // Sound volume depends on springs stored energy
                    SOUND_MODULATE(ar_instance_id, SS_MOD_BREAK, 0.5 * k * difftoBeamL * difftoBeamL);
                    SOUND_PLAY_ONCE(ar_instance_id, SS_TRIG_BREAK);

                    //Break the beam only when it is not connected to a node
                    //which is a part of a collision triangle and has 2 "live" beams or less
                    //connected to it.
                    if (!((ar_inter_beams[i]->p1->contacter && GetNumActiveConnectedBeams(ar_inter_beams[i]->p1->pos) < 3) || (ar_inter_beams[i]->p2->contacter && GetNumActiveConnectedBeams(ar_inter_beams[i]->p2->pos) < 3)))
                    {
                        slen = 0.0f;
                        ar_inter_beams[i]->bm_broken = true;
                        ar_inter_beams[i]->bm_disabled = true;

                        if (m_beam_break_debug_enabled)
                        {
                            RoR::Str<200> msg;
                            msg << "[RoR|Diag] XXX Beam " << i << " just broke with force " << len << " / " << ar_inter_beams[i]->strength << ". ";
                            LogBeamNodes(msg, (*ar_inter_beams[i]));
                            RoR::Log(msg.ToCStr());
                        }
                    }
                    else
                    {
                        ar_inter_beams[i]->strength = 2.0f * ar_inter_beams[i]->minmaxposnegstress;
                    }
                }
            }

            // At last update the beam forces
            Vector3 f = dis;
            f *= (slen * inverted_dislen);
            ar_inter_beams[i]->p1->Forces += f;
            ar_inter_beams[i]->p2->Forces -= f;
        }
    }
}

void Actor::calcNodes(int doUpdate, Ogre::Real dt, int step, int maxsteps)
{
    IWater* water = 0;
    float gravity = -9.81f;
    if (App::GetSimTerrain())
    {
        water = App::GetSimTerrain()->getWater();
        gravity = App::GetSimTerrain()->getGravity();
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
                    if (!ar_nodes[i].iswheel && m_particles_drip)
                        m_particles_drip->allocDrip(ar_nodes[i].AbsPosition, ar_nodes[i].Velocity, ar_nodes[i].wettime);
                    if (ar_nodes[i].isHot && m_particles_dust)
                        m_particles_dust->allocVapour(ar_nodes[i].AbsPosition, ar_nodes[i].Velocity, ar_nodes[i].wettime);
                }
            }
        }

        // COLLISION
        if (!ar_nodes[i].contactless)
        {
            ar_nodes[i].collTestTimer += dt;
            if (ar_nodes[i].contacted || ar_nodes[i].collTestTimer > 0.005 || ((ar_nodes[i].iswheel || ar_nodes[i].wheelid != -1) && (m_high_res_wheelnode_collisions || ar_nodes[i].collTestTimer > 0.0025)) || m_increased_accuracy)
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
                            if (m_particles_dust)
                                m_particles_dust->malloc(ar_nodes[i].AbsPosition, ar_nodes[i].Velocity / 2.0, gm->fx_colour);
                            break;

                        case Collisions::FX_HARD:
                            // smokey
                            if (ar_nodes[i].iswheel && ns > thresold)
                            {
                                if (m_particles_dust)
                                    m_particles_dust->allocSmoke(ar_nodes[i].AbsPosition, ar_nodes[i].Velocity);

                                SOUND_MODULATE(ar_instance_id, SS_MOD_SCREETCH, (ns - thresold) / thresold);
                                SOUND_PLAY_ONCE(ar_instance_id, SS_TRIG_SCREETCH);

                                //Shouldn't skidmarks be activated from here?
                                if (m_use_skidmarks)
                                {
                                    ar_wheels[ar_nodes[i].wheelid].isSkiding = true;
                                    if (!(ar_nodes[i].iswheel % 2))
                                        ar_wheels[ar_nodes[i].wheelid].lastContactInner = ar_nodes[i].AbsPosition;
                                    else
                                        ar_wheels[ar_nodes[i].wheelid].lastContactOuter = ar_nodes[i].AbsPosition;

                                    ar_wheels[ar_nodes[i].wheelid].lastContactType = (ar_nodes[i].iswheel % 2);
                                    ar_wheels[ar_nodes[i].wheelid].lastSlip = ns;
                                    ar_wheels[ar_nodes[i].wheelid].lastGroundModel = gm;
                                }
                            }
                            // sparks
                            if (!ar_nodes[i].iswheel && ns > 1.0 && !ar_nodes[i].disable_sparks)
                            {
                                // friction < 10 will remove the 'f' nodes from the spark generation nodes
                                if (m_particles_sparks)
                                    m_particles_sparks->allocSparks(ar_nodes[i].AbsPosition, ar_nodes[i].Velocity);
                            }
                            if (ar_nodes[i].iswheel && ns < thresold)
                            {
                                if (m_use_skidmarks)
                                {
                                    ar_wheels[ar_nodes[i].wheelid].isSkiding = false;
                                }
                            }
                            break;

                        case Collisions::FX_CLUMPY:
                            if (ar_nodes[i].Velocity.squaredLength() > 1.0)
                            {
                                if (m_particles_clump)
                                    m_particles_clump->allocClump(ar_nodes[i].AbsPosition, ar_nodes[i].Velocity / 2.0, gm->fx_colour);
                            }
                            break;
                        default:
                            //Useless for the moment
                            break;
                        }
                    }

                    m_last_fuzzy_ground_model = gm;
                }
                ar_nodes[i].collTestTimer = 0.0;
            }
        }

        // record g forces on cameras
        if (i == ar_camera_node_pos[0])
        {
            m_camera_gforces_accu += ar_nodes[i].Forces / ar_nodes[i].mass;
            m_camera_gforces_count++;
        }

        // integration
        if (!ar_nodes[i].locked)
        {
            ar_nodes[i].Velocity += ar_nodes[i].Forces / ar_nodes[i].mass * dt;
            ar_nodes[i].RelPosition += ar_nodes[i].Velocity * dt;
            ar_nodes[i].AbsPosition = ar_origin;
            ar_nodes[i].AbsPosition += ar_nodes[i].RelPosition;
        }

        // prepare next loop (optimisation)
        // we start forces from zero
        // start with gravity
        ar_nodes[i].Forces = Vector3(0, ar_nodes[i].mass * gravity, 0);

        if (m_fusealge_airfoil)
        {
            // aerodynamics on steroids!
            ar_nodes[i].Forces += ar_fusedrag;
        }
        else if (!ar_disable_aerodyn_turbulent_drag)
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
            if (water->IsUnderWater(ar_nodes[i].AbsPosition))
            {
                m_water_contact = true;
                if (ar_num_buoycabs == 0)
                {
                    // water drag (turbulent)
                    Real speed = approx_sqrt(ar_nodes[i].Velocity.squaredLength()); //we will (not) reuse this
                    ar_nodes[i].Forces -= (DEFAULT_WATERDRAG * speed) * ar_nodes[i].Velocity;
                    // basic buoyance
                    ar_nodes[i].Forces += ar_nodes[i].buoyancy * Vector3::UNIT_Y;
                    // basic splashing
                    if (doUpdate && water->GetStaticWaterHeight() - ar_nodes[i].AbsPosition.y < 0.2 && ar_nodes[i].Velocity.squaredLength() > 4.0 && !ar_nodes[i].disable_particles)
                    {
                        if (m_particles_splash)
                            m_particles_splash->allocSplash(ar_nodes[i].AbsPosition, ar_nodes[i].Velocity);
                        if (m_particles_ripple)
                            m_particles_ripple->allocRipple(ar_nodes[i].AbsPosition, ar_nodes[i].Velocity);
                    }
                }
                // engine stall
                if (i == ar_cinecam_node[0] && ar_engine)
                {
                    ar_engine->StopEngine();
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

void Actor::forwardCommands()
{
    Actor* current_truck = RoR::App::GetSimController()->GetPlayerActor();
    auto bf = RoR::App::GetSimController()->GetBeamFactory();
    Actor** actor_slots = bf->GetInternalActorSlots();
    int num_actor_slots = bf->GetNumUsedActorSlots();

    // forward things to trailers
    if (num_actor_slots > 1 && this == current_truck && ar_forward_commands)
    {
        for (int i = 0; i < num_actor_slots; i++)
        {
            if (!actor_slots[i])
                continue;
            if (actor_slots[i] != current_truck && actor_slots[i]->ar_import_commands)
            {
                // forward commands
                for (int j = 1; j <= MAX_COMMANDS; j++)
                {
                    actor_slots[i]->ar_command_key[j].playerInputValue = std::max(ar_command_key[j].playerInputValue, ar_command_key[j].commandValue);
                }
                // just send brake and lights to the connected truck, and no one else :)
                for (std::vector<hook_t>::iterator it = ar_hooks.begin(); it != ar_hooks.end(); it++)
                {
                    if (!it->hk_locked_actor)
                        continue;
                    // forward brake
                    it->hk_locked_actor->ar_brake = ar_brake;
                    it->hk_locked_actor->ar_parking_brake = ar_parking_brake;

                    // forward lights
                    it->hk_locked_actor->ar_lights = ar_lights;
                    it->hk_locked_actor->m_blink_type = m_blink_type;
                    //for (int k=0; k<4; k++)
                    //	hk_locked_actor->setCustomLight(k, getCustomLight(k));
                    //forward reverse light e.g. for trailers
                    it->hk_locked_actor->m_reverse_light_active = getReverseLightVisible();
                }
            }
        }
    }
}

void Actor::calcHooks()
{
    //locks - this is not active in network mode
    for (std::vector<hook_t>::iterator it = ar_hooks.begin(); it != ar_hooks.end(); it++)
    {
        if (it->hk_lock_node && it->hk_locked == PRELOCK)
        {
            if (it->hk_beam->bm_disabled)
            {
                //enable beam if not enabled yet between those 2 nodes
                it->hk_beam->p2 = it->hk_lock_node;
                it->hk_beam->bm_inter_actor = it->hk_locked_actor != 0;
                it->hk_beam->L = (it->hk_hook_node->AbsPosition - it->hk_lock_node->AbsPosition).length();
                it->hk_beam->bm_disabled = false;
                AddInterActorBeam(it->hk_beam, this, it->hk_locked_actor);
            }
            else
            {
                if (it->hk_beam->L < it->hk_beam->commandShort)
                {
                    //shortlimit reached -> status LOCKED
                    it->hk_locked = LOCKED;
                }
                else
                {
                    //shorten the connecting beam slowly to locking minrange
                    if (it->hk_beam->L > it->hk_lockspeed && fabs(it->hk_beam->stress) < it->hk_maxforce)
                    {
                        it->hk_beam->L = (it->hk_beam->L - it->hk_lockspeed);
                    }
                    else
                    {
                        if (fabs(it->hk_beam->stress) < it->hk_maxforce)
                        {
                            it->hk_beam->L = 0.001f;
                            //locking minrange or stress exeeded -> status LOCKED
                            it->hk_locked = LOCKED;
                        }
                        else
                        {
                            if (it->hk_nodisable)
                            {
                                //force exceed, but beam is set to nodisable, just lock it in this position
                                it->hk_locked = LOCKED;
                            }
                            else
                            {
                                //force exceeded reset the hook node
                                it->hk_beam->mSceneNode->detachAllObjects();
                                it->hk_locked = UNLOCKED;
                                it->hk_lock_node = 0;
                                it->hk_locked_actor = 0;
                                it->hk_beam->p2 = &ar_nodes[0];
                                it->hk_beam->bm_inter_actor = false;
                                it->hk_beam->L = (ar_nodes[0].AbsPosition - it->hk_hook_node->AbsPosition).length();
                                it->hk_beam->bm_disabled = true;
                                RemoveInterActorBeam(it->hk_beam);
                            }
                        }
                    }
                }
            }
        }
        if (it->hk_locked == PREUNLOCK)
        {
            it->hk_locked = UNLOCKED;
            RemoveInterActorBeam(it->hk_beam);
        }
    }
}

void Actor::calcRopes()
{
    if (ar_ropes.size())
    {
        for (std::vector<rope_t>::iterator it = ar_ropes.begin(); it != ar_ropes.end(); it++)
        {
            if (it->rp_locked_node)
            {
                it->rp_beam->p2->AbsPosition = it->rp_locked_node->AbsPosition;
                it->rp_beam->p2->RelPosition = it->rp_locked_node->AbsPosition - ar_origin; //ropes[i].rp_locked_actor->origin; //we have a problem here
                it->rp_beam->p2->Velocity = it->rp_locked_node->Velocity;
                it->rp_locked_node->Forces = it->rp_locked_node->Forces + it->rp_beam->p2->Forces;
                it->rp_beam->p2->Forces = Vector3::ZERO;
            }
        }
    }
}

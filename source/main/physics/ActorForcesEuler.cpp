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

#include "Application.h"
#include "AeroEngine.h"
#include "AirBrake.h"
#include "Airfoil.h"
#include "ApproxMath.h"
#include "Actor.h"
#include "ActorManager.h"
#include "Buoyance.h"
#include "CmdKeyInertia.h"
#include "Collisions.h"
#include "Differentials.h"
#include "EngineSim.h"
#include "FlexAirfoil.h"
#include "GameContext.h"
#include "Replay.h"
#include "ScrewProp.h"
#include "SoundScriptManager.h"
#include "TerrainManager.h"
#include "Water.h"

using namespace Ogre;
using namespace RoR;

void Actor::CalcForcesEulerCompute(bool doUpdate, int num_steps)
{
    this->CalcNodes(); // must be done directly after the inter truck collisions are handled
    this->CalcReplay();
    this->CalcAircraftForces(doUpdate);
    this->CalcFuseDrag();
    this->CalcBuoyance(doUpdate);
    this->CalcDifferentials();
    this->CalcWheels(doUpdate, num_steps);
    this->CalcShocks(doUpdate, num_steps);
    this->CalcHydros();
    this->CalcCommands(doUpdate);
    this->CalcTies();
    this->CalcTruckEngine(doUpdate); // must be done after the commands / engine triggers are updated
    this->CalcMouse();
    this->CalcBeams(doUpdate);
    this->CalcCabCollisions();
    this->UpdateSlideNodeForces(PHYSICS_DT); // must be done after the contacters are updated
    this->CalcForceFeedback(doUpdate);
}

void Actor::CalcForceFeedback(bool doUpdate)
{
    if (this == App::GetGameContext()->GetPlayerActor())
    {
        if (doUpdate)
        {
            m_force_sensors.Reset();
        }

        if (ar_current_cinecam != -1)
        {
            m_force_sensors.accu_body_forces += ar_nodes[ar_camera_node_pos[ar_current_cinecam]].Forces;
        }

        for (hydrobeam_t& hydrobeam: ar_hydros)
        {
            beam_t* beam = &ar_beams[hydrobeam.hb_beam_index];
            if ((hydrobeam.hb_flags & (HYDRO_FLAG_DIR | HYDRO_FLAG_SPEED)) && !beam->bm_broken)
            {
                m_force_sensors.accu_hydros_forces += hydrobeam.hb_speed * beam->refL * beam->stress;
            }
        }
    }
}

void Actor::CalcMouse()
{
    if (m_mouse_grab_node != -1)
    {
        Vector3 dir = m_mouse_grab_pos - ar_nodes[m_mouse_grab_node].AbsPosition;
        ar_nodes[m_mouse_grab_node].Forces += m_mouse_grab_move_force * dir;
    }
}

void Actor::CalcAircraftForces(bool doUpdate)
{
    //airbrake forces
    for (Airbrake* ab: ar_airbrakes)
        ab->applyForce();

    //turboprop forces
    for (int i = 0; i < ar_num_aeroengines; i++)
        if (ar_aeroengines[i])
            ar_aeroengines[i]->updateForces(PHYSICS_DT, doUpdate);

    //screwprop forces
    for (int i = 0; i < ar_num_screwprops; i++)
        if (ar_screwprops[i])
            ar_screwprops[i]->updateForces(doUpdate);

    //wing forces
    for (int i = 0; i < ar_num_wings; i++)
        if (ar_wings[i].fa)
            ar_wings[i].fa->updateForces();
}

void Actor::CalcFuseDrag()
{
    if (m_fusealge_airfoil && m_fusealge_width > 0.0f)
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
        float sea_level_pressure = 101325; //in Pa
        float airpressure = sea_level_pressure * approx_pow(1.0 - 0.0065 * altitude / 288.1, 5.24947); //in Pa
        float airdensity = airpressure * 0.0000120896f;//1.225 at sea level

        //fuselage as an airfoil + parasitic drag (half fuselage front surface almost as a flat plane!)
        ar_fusedrag = ((cx * s + m_fusealge_width * m_fusealge_width * 0.5) * 0.5 * airdensity * wspeed / ar_num_nodes) * wind; 
    }
}

void Actor::CalcBuoyance(bool doUpdate)
{
    if (ar_num_buoycabs && App::GetSimTerrain()->getWater())
    {
        for (int i = 0; i < ar_num_buoycabs; i++)
        {
            int tmpv = ar_buoycabs[i] * 3;
            m_buoyance->computeNodeForce(&ar_nodes[ar_cabs[tmpv]], &ar_nodes[ar_cabs[tmpv + 1]], &ar_nodes[ar_cabs[tmpv + 2]], doUpdate == 1, ar_buoycab_types[i]);
        }
    }
}

void Actor::CalcDifferentials()
{
    if (ar_engine && m_num_proped_wheels > 0)
    {
        float torque = ar_engine->GetTorque() / m_num_proped_wheels;
        if (m_has_axles_section)
        {
            torque *= 2.0f; // Required to stay backwards compatible
        }
        for (int i = 0; i < ar_num_wheels; i++)
        {
            if (ar_wheels[i].wh_propulsed && !ar_wheels[i].wh_is_detached)
                ar_wheels[i].wh_torque += torque;
        }
    }

    int num_axle_diffs = (m_transfer_case && m_transfer_case->tr_4wd_mode) ? m_num_axle_diffs + 1 : m_num_axle_diffs;

    // Handle detached wheels
    for (int i = 0; i < num_axle_diffs; i++)
    {
        int a_1 = m_axle_diffs[i]->di_idx_1;
        int a_2 = m_axle_diffs[i]->di_idx_2;
        wheel_t* axle_1_wheels[2] = {&ar_wheels[m_wheel_diffs[a_1]->di_idx_1], &ar_wheels[m_wheel_diffs[a_1]->di_idx_2]};
        wheel_t* axle_2_wheels[2] = {&ar_wheels[m_wheel_diffs[a_2]->di_idx_1], &ar_wheels[m_wheel_diffs[a_2]->di_idx_2]};
        if (axle_1_wheels[0]->wh_is_detached && axle_1_wheels[1]->wh_is_detached)
        {
            axle_1_wheels[0]->wh_speed = axle_2_wheels[0]->wh_speed;
            axle_1_wheels[1]->wh_speed = axle_2_wheels[1]->wh_speed;
        }
        if (axle_2_wheels[0]->wh_is_detached && axle_2_wheels[1]->wh_is_detached)
        {
            axle_2_wheels[0]->wh_speed = axle_1_wheels[0]->wh_speed;
            axle_2_wheels[1]->wh_speed = axle_1_wheels[1]->wh_speed;
        }
    }
    for (int i = 0; i < m_num_wheel_diffs; i++)
    {
        wheel_t* axle_wheels[2] = {&ar_wheels[m_wheel_diffs[i]->di_idx_1], &ar_wheels[m_wheel_diffs[i]->di_idx_2]};
        if (axle_wheels[0]->wh_is_detached) axle_wheels[0]->wh_speed = axle_wheels[1]->wh_speed;
        if (axle_wheels[1]->wh_is_detached) axle_wheels[1]->wh_speed = axle_wheels[0]->wh_speed;
    }

    // loop through all interaxle differentials, this is the torsion to keep
    // the axles aligned with each other as if they connected by a shaft
    for (int i = 0; i < num_axle_diffs; i++)
    {
        int a_1 = m_axle_diffs[i]->di_idx_1;
        int a_2 = m_axle_diffs[i]->di_idx_2;

        Ogre::Real axle_torques[2] = {0.0f, 0.0f};
        DifferentialData diff_data =
        {
            {
                (ar_wheels[m_wheel_diffs[a_1]->di_idx_1].wh_speed + ar_wheels[m_wheel_diffs[a_1]->di_idx_2].wh_speed) * 0.5f,
                (ar_wheels[m_wheel_diffs[a_2]->di_idx_1].wh_speed + ar_wheels[m_wheel_diffs[a_2]->di_idx_2].wh_speed) * 0.5f
            },
            m_axle_diffs[i]->di_delta_rotation,
            {axle_torques[0], axle_torques[1]},
            ar_wheels[m_wheel_diffs[a_1]->di_idx_1].wh_torque + ar_wheels[m_wheel_diffs[a_1]->di_idx_2].wh_torque +
            ar_wheels[m_wheel_diffs[a_2]->di_idx_1].wh_torque + ar_wheels[m_wheel_diffs[a_2]->di_idx_2].wh_torque,
            PHYSICS_DT
        };

        m_axle_diffs[i]->CalcAxleTorque(diff_data);

        m_axle_diffs[i]->di_delta_rotation = diff_data.delta_rotation;

        ar_wheels[m_wheel_diffs[a_1]->di_idx_1].wh_torque = diff_data.out_torque[0] * 0.5f;
        ar_wheels[m_wheel_diffs[a_1]->di_idx_2].wh_torque = diff_data.out_torque[0] * 0.5f;
        ar_wheels[m_wheel_diffs[a_2]->di_idx_1].wh_torque = diff_data.out_torque[1] * 0.5f;
        ar_wheels[m_wheel_diffs[a_2]->di_idx_2].wh_torque = diff_data.out_torque[1] * 0.5f;
    }

    // loop through all interwheel differentials, this is the torsion to keep
    // the wheels aligned with each other as if they connected by a shaft
    for (int i = 0; i < m_num_wheel_diffs; i++)
    {
        Ogre::Real axle_torques[2] = {0.0f, 0.0f};
        wheel_t* axle_wheels[2] = {&ar_wheels[m_wheel_diffs[i]->di_idx_1], &ar_wheels[m_wheel_diffs[i]->di_idx_2]};

        DifferentialData diff_data =
        {
            {axle_wheels[0]->wh_speed, axle_wheels[1]->wh_speed},
            m_wheel_diffs[i]->di_delta_rotation,
            {axle_torques[0], axle_torques[1]},
            axle_wheels[0]->wh_torque + axle_wheels[1]->wh_torque,
            PHYSICS_DT
        };

        m_wheel_diffs[i]->CalcAxleTorque(diff_data);

        m_wheel_diffs[i]->di_delta_rotation = diff_data.delta_rotation;

        ar_wheels[m_wheel_diffs[i]->di_idx_1].wh_torque = diff_data.out_torque[0];
        ar_wheels[m_wheel_diffs[i]->di_idx_2].wh_torque = diff_data.out_torque[1];
    }
}

void Actor::CalcWheels(bool doUpdate, int num_steps)
{
    // driving aids traction control & anti-lock brake pulse
    tc_timer += PHYSICS_DT;
    alb_timer += PHYSICS_DT;

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

    m_antilockbrake = false;
    m_tractioncontrol = false;

    ar_wheel_spin = 0.0f;
    ar_wheel_speed = 0.0f;

    for (int i = 0; i < ar_num_wheels; i++)
    {
        if (doUpdate)
        {
            ar_wheels[i].debug_rpm = 0.0f;
            ar_wheels[i].debug_torque = 0.0f;
            ar_wheels[i].debug_vel = Vector3::ZERO;
            ar_wheels[i].debug_slip = Vector3::ZERO;
            ar_wheels[i].debug_force = Vector3::ZERO;
            ar_wheels[i].debug_scaled_cforce = Vector3::ZERO;
        }

        if (ar_wheels[i].wh_is_detached)
            continue;

        float relspeed = ar_nodes[0].Velocity.dotProduct(getDirection());
        float curspeed = fabs(relspeed);
        float wheel_slip = fabs(ar_wheels[i].wh_speed - relspeed) / std::max(1.0f, curspeed);

        // traction control
        if (tc_mode && fabs(ar_wheels[i].wh_torque) > 0.0f && fabs(ar_wheels[i].wh_speed) > curspeed && wheel_slip > 0.25f)
        {
            if (tc_pulse_state)
            {
                ar_wheels[i].wh_tc_coef = curspeed / fabs(ar_wheels[i].wh_speed);
                ar_wheels[i].wh_tc_coef = pow(ar_wheels[i].wh_tc_coef, tc_ratio);
            }
            float tc_coef = pow(ar_wheels[i].wh_tc_coef, std::min(std::abs(ar_wheels[i].wh_speed) / 5.0f, 1.0f));
            ar_wheels[i].wh_torque *= tc_coef;
            m_tractioncontrol = true;
        }
        else
        {
            ar_wheels[i].wh_tc_coef = 1.0f;
        }

        if (ar_wheels[i].wh_braking != wheel_t::BrakeCombo::NONE)
        {
            // footbrake
            float abrake = ar_brake_force * ar_brake;

            // handbrake
            float hbrake = 0.0f;
            if (ar_parking_brake && (ar_wheels[i].wh_braking != wheel_t::BrakeCombo::FOOT_ONLY))
            {
                hbrake = m_handbrake_force;
            }

            // directional braking
            float dbrake = 0.0f;
            if ((ar_wheels[i].wh_speed < 20.0f)
                && (((ar_wheels[i].wh_braking == wheel_t::BrakeCombo::FOOT_HAND_SKID_LEFT)  && (ar_hydro_dir_state > 0.0f))
                 || ((ar_wheels[i].wh_braking == wheel_t::BrakeCombo::FOOT_HAND_SKID_RIGHT) && (ar_hydro_dir_state < 0.0f))))
            {
                dbrake = ar_brake_force * abs(ar_hydro_dir_state);
            }

            if (abrake != 0.0 || dbrake != 0.0 || hbrake != 0.0)
            {
                float adbrake = abrake + dbrake; 

                // anti-lock braking
                if (alb_mode && curspeed > alb_minspeed && curspeed > fabs(ar_wheels[i].wh_speed) && (adbrake > 0.0f) && wheel_slip > 0.25f) 
                {
                    if (alb_pulse_state)
                    {
                        ar_wheels[i].wh_alb_coef = fabs(ar_wheels[i].wh_speed) / curspeed;
                        ar_wheels[i].wh_alb_coef = pow(ar_wheels[i].wh_alb_coef, alb_ratio);
                    }
                    adbrake *= ar_wheels[i].wh_alb_coef;
                    m_antilockbrake = true;
                }

                float force = -ar_wheels[i].wh_avg_speed * ar_wheels[i].wh_radius * ar_wheels[i].wh_mass / PHYSICS_DT;
                force -= ar_wheels[i].wh_last_retorque;

                if (ar_wheels[i].wh_speed > 0)
                    ar_wheels[i].wh_torque += Math::Clamp(force, -(adbrake + hbrake), 0.0f);
                else
                    ar_wheels[i].wh_torque += Math::Clamp(force, 0.0f, +(adbrake + hbrake));
            }
            else
            {
                ar_wheels[i].wh_alb_coef = 1.0f;
            }
        }

        ar_wheels[i].debug_torque += ar_wheels[i].wh_torque / (float)num_steps;

        // application to wheel
        Vector3 axis = (ar_wheels[i].wh_axis_node_1->RelPosition - ar_wheels[i].wh_axis_node_0->RelPosition).normalisedCopy();
        float axis_precalc = ar_wheels[i].wh_torque / (Real)(ar_wheels[i].wh_num_nodes);

        float expected_wheel_speed = ar_wheels[i].wh_speed;
        ar_wheels[i].wh_speed = 0.0f;

        Real contact_counter = 0.0f;
        Vector3 slip = Vector3::ZERO;
        Vector3 force = Vector3::ZERO;
        for (int j = 0; j < ar_wheels[i].wh_num_nodes; j++)
        {
            node_t* outer_node = ar_wheels[i].wh_nodes[j];
            node_t* inner_node = (j % 2) ? ar_wheels[i].wh_axis_node_1 : ar_wheels[i].wh_axis_node_0;

            Vector3 radius = outer_node->RelPosition - inner_node->RelPosition;
            float inverted_rlen = 1.0f / radius.length();

            if (ar_wheels[i].wh_propulsed == 2)
            {
                radius = -radius;
            }

            Vector3 dir = axis.crossProduct(radius) * inverted_rlen;
            ar_wheels[i].wh_nodes[j]->Forces += dir * axis_precalc * inverted_rlen;
            ar_wheels[i].wh_speed += (outer_node->Velocity - inner_node->Velocity).dotProduct(dir);

            if (ar_wheels[i].wh_nodes[j]->nd_has_ground_contact || ar_wheels[i].wh_nodes[j]->nd_has_mesh_contact)
            {
                contact_counter += 1.0f;
                float force_ratio = ar_wheels[i].wh_nodes[j]->nd_last_collision_force.length();
                slip  += ar_wheels[i].wh_nodes[j]->nd_last_collision_slip * force_ratio;
                force += ar_wheels[i].wh_nodes[j]->nd_last_collision_force;
            }
        }
        if (contact_counter > 0.0f && !force.isZeroLength())
        {
            slip /= force.length(); // slip vector weighted by down force
            slip /= contact_counter; // average slip vector
            force /= contact_counter; // average force vector
            Vector3 normal = force.normalisedCopy(); // contact plane normal
            Vector3 v = ar_wheels[i].wh_axis_node_0->Velocity.midPoint(ar_wheels[i].wh_axis_node_1->Velocity);
            Vector3 vel = v - v.dotProduct(normal) * normal;
            ar_wheels[i].debug_vel   += vel / (float)num_steps;
            ar_wheels[i].debug_slip  += slip / (float)num_steps;
            ar_wheels[i].debug_force += force / (float)num_steps;
        }

        ar_wheels[i].wh_speed /= (Real)ar_wheels[i].wh_num_nodes;
        ar_wheels[i].wh_net_rp += (ar_wheels[i].wh_speed / ar_wheels[i].wh_radius) * PHYSICS_DT;
        // We overestimate the average speed on purpose in order to improve the quality of the braking force estimate
        ar_wheels[i].wh_avg_speed = ar_wheels[i].wh_avg_speed * 0.99 + ar_wheels[i].wh_speed * 0.1;
        ar_wheels[i].debug_rpm += RAD_PER_SEC_TO_RPM * ar_wheels[i].wh_speed / ar_wheels[i].wh_radius / (float)num_steps;
        if (ar_wheels[i].wh_propulsed == 1)
        {
            float speedacc = ar_wheels[i].wh_speed / (float)m_num_proped_wheels;
            ar_wheel_speed += speedacc;                          // Accumulate the average wheel speed (m/s)
            ar_wheel_spin  += speedacc / ar_wheels[i].wh_radius; // Accumulate the average wheel spin  (radians)
        }

        expected_wheel_speed += ((ar_wheels[i].wh_last_torque / ar_wheels[i].wh_radius) / ar_wheels[i].wh_mass) * PHYSICS_DT;
        ar_wheels[i].wh_last_retorque = ar_wheels[i].wh_mass * (ar_wheels[i].wh_speed - expected_wheel_speed) / PHYSICS_DT;

        // reaction torque
        Vector3 rradius = ar_wheels[i].wh_arm_node->RelPosition - ar_wheels[i].wh_near_attach_node->RelPosition;
        Vector3 radius = Plane(axis, ar_wheels[i].wh_near_attach_node->RelPosition).projectVector(rradius);
        float offset = (rradius - radius).length(); // length of the error arm
        Real rlen = radius.normalise(); // length of the projected arm
        // TODO: Investigate the offset length abort condition ~ ulteq 10/2018
        if (rlen > 0.01 && offset * 2.0f < rlen && fabs(ar_wheels[i].wh_torque) > 0.01f)
        {
            Vector3 cforce = axis.crossProduct(radius);
            // modulate the force according to induced torque error
            cforce *= (0.5f * ar_wheels[i].wh_torque / rlen) * (1.0f - ((offset * 2.0f) / rlen)); // linear modulation
            ar_wheels[i].wh_arm_node->Forces -= cforce;
            ar_wheels[i].wh_near_attach_node->Forces += cforce;
            ar_wheels[i].debug_scaled_cforce += cforce / m_total_mass / (float)num_steps;
        }

        ar_wheels[i].wh_last_torque = ar_wheels[i].wh_torque;
        ar_wheels[i].wh_torque = 0.0f;
    }

    ar_avg_wheel_speed = ar_avg_wheel_speed * 0.995 + ar_wheel_speed * 0.005;

    if (ar_engine)
    {
        ar_engine->SetWheelSpin(ar_wheel_spin * RAD_PER_SEC_TO_RPM); // Update the driveshaft speed
    }

    if (doUpdate)
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

    // calculate driven distance
    float distance_driven = fabs(ar_wheel_speed * PHYSICS_DT);
    m_odometer_total += distance_driven;
    m_odometer_user += distance_driven;
}

void Actor::CalcShocks(bool doUpdate, int num_steps)
{
    //variable shocks for stabilization
    if (this->ar_has_active_shocks && m_stabilizer_shock_request)
    {
        if ((m_stabilizer_shock_request == 1 && m_stabilizer_shock_ratio < 0.1) || (m_stabilizer_shock_request == -1 && m_stabilizer_shock_ratio > -0.1))
            m_stabilizer_shock_ratio = m_stabilizer_shock_ratio + (float)m_stabilizer_shock_request * PHYSICS_DT * STAB_RATE;
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
        m_stabilizer_shock_sleep -= PHYSICS_DT * num_steps;

        float roll = asin(GetCameraRoll().dotProduct(Vector3::UNIT_Y));
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
}

void Actor::CalcHydros()
{
    //direction
    if (ar_hydro_dir_state != 0 || ar_hydro_dir_command != 0)
    {
        if (!ar_hydro_speed_coupling)
        {
            // need a maximum rate for analog devices, otherwise hydro beams break
            float smoothing   = Math::Clamp(App::io_analog_smoothing->GetFloat(),   0.5f, 2.0f);
            float sensitivity = Math::Clamp(App::io_analog_sensitivity->GetFloat(), 0.5f, 2.0f);
            float diff = ar_hydro_dir_command - ar_hydro_dir_state;
            float rate = std::exp(-std::min(std::abs(diff), 1.0f) / sensitivity) * diff;
            ar_hydro_dir_state += (10.0f / smoothing) * PHYSICS_DT * rate;
        }
        else
        {
            if (ar_hydro_dir_command != 0)
            {
                if (!App::io_hydro_coupling->GetBool())
                {
                    float rate = std::max(1.2f, 30.0f / (10.0f));
                    if (ar_hydro_dir_state > ar_hydro_dir_command)
                        ar_hydro_dir_state -= PHYSICS_DT * rate;
                    else
                        ar_hydro_dir_state += PHYSICS_DT * rate;
                }
                else
                {
                    // minimum rate: 20% --> enables to steer high velocity vehicles
                    float rate = std::max(1.2f, 30.0f / (10.0f + std::abs(ar_wheel_speed / 2.0f)));
                    if (ar_hydro_dir_state > ar_hydro_dir_command)
                        ar_hydro_dir_state -= PHYSICS_DT * rate;
                    else
                        ar_hydro_dir_state += PHYSICS_DT * rate;
                }
            }
            float dirdelta = PHYSICS_DT;
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
                ar_hydro_aileron_state -= PHYSICS_DT * 4.0;
            else
                ar_hydro_aileron_state += PHYSICS_DT * 4.0;
        }
        float delta = PHYSICS_DT;
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
                ar_hydro_rudder_state -= PHYSICS_DT * 4.0;
            else
                ar_hydro_rudder_state += PHYSICS_DT * 4.0;
        }

        float delta = PHYSICS_DT;
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
                ar_hydro_elevator_state -= PHYSICS_DT * 4.0;
            else
                ar_hydro_elevator_state += PHYSICS_DT * 4.0;
        }
        float delta = PHYSICS_DT;
        if (ar_hydro_elevator_state > delta)
            ar_hydro_elevator_state -= delta;
        else if (ar_hydro_elevator_state < -delta)
            ar_hydro_elevator_state += delta;
        else
            ar_hydro_elevator_state = 0;
    }
    //update length, dirstate between -1.0 and 1.0
    const int num_hydros = static_cast<int>(ar_hydros.size());
    for (int i = 0; i < num_hydros; ++i)
    {
        hydrobeam_t& hydrobeam = ar_hydros[i];

        //compound hydro
        float cstate = 0.0f;
        int div = 0;
        if (hydrobeam.hb_flags & HYDRO_FLAG_SPEED)
        {
            //special treatment for SPEED
            if (ar_wheel_speed < 12.0f)
                cstate += ar_hydro_dir_state * (12.0f - ar_wheel_speed) / 12.0f;
            div++;
        }
        if (hydrobeam.hb_flags & HYDRO_FLAG_DIR)
        {
            cstate += ar_hydro_dir_state;
            div++;
        }
        if (hydrobeam.hb_flags & HYDRO_FLAG_AILERON)
        {
            cstate += ar_hydro_aileron_state;
            div++;
        }
        if (hydrobeam.hb_flags & HYDRO_FLAG_RUDDER)
        {
            cstate += ar_hydro_rudder_state;
            div++;
        }
        if (hydrobeam.hb_flags & HYDRO_FLAG_ELEVATOR)
        {
            cstate += ar_hydro_elevator_state;
            div++;
        }
        if (hydrobeam.hb_flags & HYDRO_FLAG_REV_AILERON)
        {
            cstate -= ar_hydro_aileron_state;
            div++;
        }
        if (hydrobeam.hb_flags & HYDRO_FLAG_REV_RUDDER)
        {
            cstate -= ar_hydro_rudder_state;
            div++;
        }
        if (hydrobeam.hb_flags & HYDRO_FLAG_REV_ELEVATOR)
        {
            cstate -= ar_hydro_elevator_state;
            div++;
        }

        const uint16_t beam_idx = hydrobeam.hb_beam_index;

        if (cstate > 1.0)
            cstate = 1.0;
        if (cstate < -1.0)
            cstate = -1.0;

        // Animators
        if (hydrobeam.hb_anim_flags)
        {
            this->CalcAnimators(hydrobeam, cstate, div);
        }

        // Final composition
        if (div)
        {
            cstate /= (float)div;

            cstate = hydrobeam.hb_inertia.CalcCmdKeyDelay(cstate, PHYSICS_DT);

            if (!(hydrobeam.hb_flags & HYDRO_FLAG_SPEED) && !hydrobeam.hb_anim_flags)
                ar_hydro_dir_wheel_display = cstate;

            float factor = 1.0 - cstate * hydrobeam.hb_speed;

            // check and apply animators limits if set
            if (hydrobeam.hb_anim_flags)
            {
                if (factor < 1.0f - ar_beams[beam_idx].shortbound)
                    factor = 1.0f - ar_beams[beam_idx].shortbound;
                if (factor > 1.0f + ar_beams[beam_idx].longbound)
                    factor = 1.0f + ar_beams[beam_idx].longbound;
            }

            ar_beams[beam_idx].L = hydrobeam.hb_ref_length * factor;
        }
    }
}

void Actor::CalcCommands(bool doUpdate)
{
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
                ar_command_key[i].beams[j].cmb_state->auto_move_lock = false;
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
                    ar_command_key[i].beams[j].cmb_state->auto_move_lock = true;
                    if (ar_command_key[i].beams[j].cmb_is_autocentering)
                    {
                        ar_command_key[i].beams[j].cmb_state->auto_moving_mode = 0;
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
                commandbeam_t& cmd_beam = ar_command_key[i].beams[j];
                int bbeam_dir = (cmd_beam.cmb_is_contraction) ? -1 : 1;
                int bbeam = cmd_beam.cmb_beam_index;

                if (bbeam > ar_num_beams)
                    continue;

                // restrict forces
                if (cmd_beam.cmb_is_force_restricted)
                    crankfactor = std::min(crankfactor, 1.0f);

                float v = ar_command_key[i].commandValue;
                int& vst = ar_command_key[i].commandValueState;

                // self centering
                if (cmd_beam.cmb_is_autocentering && !cmd_beam.cmb_state->auto_move_lock)
                {
                    // check for some error
                    if (ar_beams[bbeam].refL == 0 || ar_beams[bbeam].L == 0)
                        continue;

                    float current = (ar_beams[bbeam].L / ar_beams[bbeam].refL);

                    if (fabs(current - cmd_beam.cmb_center_length) < 0.0001)
                    {
                        // hold condition
                        cmd_beam.cmb_state->auto_moving_mode = 0;
                    }
                    else
                    {
                        int mode = cmd_beam.cmb_state->auto_moving_mode;

                        // determine direction
                        if (current > cmd_beam.cmb_center_length)
                            cmd_beam.cmb_state->auto_moving_mode = -1;
                        else
                            cmd_beam.cmb_state->auto_moving_mode = 1;

                        // avoid overshooting
                        if (mode != 0 && mode != cmd_beam.cmb_state->auto_moving_mode)
                        {
                            ar_beams[bbeam].L = cmd_beam.cmb_center_length * ar_beams[bbeam].refL;
                            cmd_beam.cmb_state->auto_moving_mode = 0;
                        }
                    }
                }

                if (ar_beams[bbeam].refL != 0 && ar_beams[bbeam].L != 0)
                {
                    float clen = ar_beams[bbeam].L / ar_beams[bbeam].refL;
                    if ((bbeam_dir > 0 && clen < cmd_beam.cmb_boundary_length) || (bbeam_dir < 0 && clen > cmd_beam.cmb_boundary_length))
                    {
                        float dl = ar_beams[bbeam].L;

                        if (cmd_beam.cmb_is_1press_center)
                        {
                            // one press + centering
                            if (bbeam_dir * cmd_beam.cmb_state->auto_moving_mode > 0 && bbeam_dir * clen > bbeam_dir * cmd_beam.cmb_center_length && !cmd_beam.cmb_state->pressed_center_mode)
                            {
                                cmd_beam.cmb_state->pressed_center_mode = true;
                                cmd_beam.cmb_state->auto_moving_mode = 0;
                            }
                            else if (bbeam_dir * cmd_beam.cmb_state->auto_moving_mode < 0 && bbeam_dir * clen > bbeam_dir * cmd_beam.cmb_center_length && cmd_beam.cmb_state->pressed_center_mode)
                            {
                                cmd_beam.cmb_state->pressed_center_mode = false;
                            }
                        }
                        if (cmd_beam.cmb_is_1press || cmd_beam.cmb_is_1press_center)
                        {
                            bool key = (v > 0.5);
                            if (bbeam_dir * cmd_beam.cmb_state->auto_moving_mode <= 0 && key)
                            {
                                cmd_beam.cmb_state->auto_moving_mode = bbeam_dir * 1;
                            }
                            else if (cmd_beam.cmb_state->auto_moving_mode == bbeam_dir * 1 && !key)
                            {
                                cmd_beam.cmb_state->auto_moving_mode = bbeam_dir * 2;
                            }
                            else if (cmd_beam.cmb_state->auto_moving_mode == bbeam_dir * 2 && key)
                            {
                                cmd_beam.cmb_state->auto_moving_mode = bbeam_dir * 3;
                            }
                            else if (cmd_beam.cmb_state->auto_moving_mode == bbeam_dir * 3 && !key)
                            {
                                cmd_beam.cmb_state->auto_moving_mode = 0;
                            }
                        }

                        v = ar_command_key[i].command_inertia.CalcCmdKeyDelay(v, PHYSICS_DT);

                        if (bbeam_dir * cmd_beam.cmb_state->auto_moving_mode > 0)
                            v = 1;

                        if (cmd_beam.cmb_needs_engine && ((ar_engine && !ar_engine->IsRunning()) || !ar_engine_hydraulics_ready))
                            continue;

                        if (v > 0.0f && cmd_beam.cmb_engine_coupling > 0.0f)
                            requestpower = true;

#ifdef USE_OPENAL
                        if (cmd_beam.cmb_plays_sound)
                        {
                            // command sounds
                            if (vst == 1)
                            {
                                // just started
                                App::GetSoundScriptManager()->trigStop(ar_instance_id, SS_TRIG_LINKED_COMMAND, SL_COMMAND, -i);
                                App::GetSoundScriptManager()->trigStart(ar_instance_id, SS_TRIG_LINKED_COMMAND, SL_COMMAND, i);
                                vst = 0;
                            }
                            else if (vst == -1)
                            {
                                // just stopped
                                App::GetSoundScriptManager()->trigStop(ar_instance_id, SS_TRIG_LINKED_COMMAND, SL_COMMAND, i);
                                vst = 0;
                            }
                            else if (vst == 0)
                            {
                                // already running, modulate
                                App::GetSoundScriptManager()->modulate(ar_instance_id, SS_MOD_LINKED_COMMANDRATE, v, SL_COMMAND, i);
                            }
                        }
#endif //USE_OPENAL
                        float cf = 1.0f;

                        if (cmd_beam.cmb_engine_coupling > 0)
                            cf = crankfactor;

                        if (bbeam_dir > 0)
                            ar_beams[bbeam].L *= (1.0 + cmd_beam.cmb_speed * v * cf * PHYSICS_DT / ar_beams[bbeam].L);
                        else
                            ar_beams[bbeam].L *= (1.0 - cmd_beam.cmb_speed * v * cf * PHYSICS_DT / ar_beams[bbeam].L);

                        dl = fabs(dl - ar_beams[bbeam].L);
                        if (requestpower)
                        {
                            active++;
                            work += fabs(ar_beams[bbeam].stress) * dl * cmd_beam.cmb_engine_coupling;
                        }
                    }
                    else if ((cmd_beam.cmb_is_1press || cmd_beam.cmb_is_1press_center) && bbeam_dir * cmd_beam.cmb_state->auto_moving_mode > 0)
                    {
                        // beyond length
                        cmd_beam.cmb_state->auto_moving_mode = 0;
                    }
                }
            }
            // also for rotators
            for (int j = 0; j < (int)ar_command_key[i].rotators.size(); j++)
            {
                float v = 0.0f;
                int rota = std::abs(ar_command_key[i].rotators[j]) - 1;

                if (ar_rotators[rota].needs_engine && ((ar_engine && !ar_engine->IsRunning()) || !ar_engine_hydraulics_ready))
                    continue;

                v = ar_command_key[i].rotator_inertia.CalcCmdKeyDelay(ar_command_key[i].commandValue, PHYSICS_DT);

                if (v > 0.0f && ar_rotators[rota].engine_coupling > 0.0f)
                    requestpower = true;

                float cf = 1.0f;

                if (ar_rotators[rota].engine_coupling > 0.0f)
                    cf = crankfactor;

                if (ar_command_key[i].rotators[j] > 0)
                    ar_rotators[rota].angle += ar_rotators[rota].rate * v * cf * PHYSICS_DT;
                else
                    ar_rotators[rota].angle -= ar_rotators[rota].rate * v * cf * PHYSICS_DT;

                if (doUpdate || v != 0.0f)
                {
                    ar_rotators[rota].debug_rate = ar_rotators[rota].rate * v * cf;
                }
            }
            if (requestpower)
                requested=true;
        }

        if (ar_engine)
        {
            ar_engine->SetHydroPumpWork(work);
            ar_engine->SetEnginePriming(requested);
        }

        if (doUpdate && this == App::GetGameContext()->GetPlayerActor())
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
            Vector3 ax1 = ar_nodes[ar_rotators[i].axis1].RelPosition;
            Vector3 ax2 = ar_nodes[ar_rotators[i].axis2].RelPosition;
            Vector3 axis = ax1 - ax2;
            axis.normalise();
            // find the reference plane
            Plane pl = Plane(axis, 0);
            // for each pairar
            ar_rotators[i].debug_aerror = 0;
            for (int k = 0; k < 2; k++)
            {
                // find the reference vectors
                Vector3 ref1 = pl.projectVector(ax1 - ar_nodes[ar_rotators[i].nodes1[k]].RelPosition);
                Vector3 ref2 = pl.projectVector(ax2 - ar_nodes[ar_rotators[i].nodes2[k]].RelPosition);
                float ref1len = ref1.normalise();
                float ref2len = ref2.normalise();
                // theory vector
                Vector3 th1 = Quaternion(Radian(ar_rotators[i].angle + Math::HALF_PI), axis) * ref1;
                // find the angle error
                float aerror = asin(th1.dotProduct(ref2));
                ar_rotators[i].debug_aerror += 0.5f * aerror;
                // exert forces
                float rigidity = ar_rotators[i].force;
                Vector3 dir1 = ref1.crossProduct(axis);
                Vector3 dir2 = ref2.crossProduct(axis);

                // simple jitter fix
                if (ref1len <= ar_rotators[i].tolerance)
                    ref1len = 0.0f;
                if (ref2len <= ar_rotators[i].tolerance)
                    ref2len = 0.0f;

                ar_nodes[ar_rotators[i].nodes1[k    ]].Forces += (aerror * ref1len * rigidity) * dir1;
                ar_nodes[ar_rotators[i].nodes2[k    ]].Forces -= (aerror * ref2len * rigidity) * dir2;
                // symmetric
                ar_nodes[ar_rotators[i].nodes1[k + 2]].Forces -= (aerror * ref1len * rigidity) * dir1;
                ar_nodes[ar_rotators[i].nodes2[k + 2]].Forces += (aerror * ref2len * rigidity) * dir2;
            }
        }
    }
}

void Actor::CalcTies()
{
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
        if (clen > it->ti_min_length)
        {
            it->ti_beam->L *= (1.0 - it->ti_contract_speed * PHYSICS_DT / it->ti_beam->L);
        }
        else
        {
            // tying finished, end reached
            it->ti_tying = false;
        }

        // check if we hit a certain force limit, then abort the tying process
        if (fabs(it->ti_beam->stress) > it->ti_max_stress)
        {
            it->ti_tying = false;
        }
    }
}
void Actor::CalcTruckEngine(bool doUpdate)
{
    if (ar_engine)
    {
        ar_engine->UpdateEngineSim(PHYSICS_DT, doUpdate);
    }
}

void Actor::CalcReplay()
{
    if (m_replay_handler && m_replay_handler->isValid())
    {
        m_replay_handler->onPhysicsStep();
    }
}

bool Actor::CalcForcesEulerPrepare(bool doUpdate)
{
    if (m_ongoing_reset)
        return false;
    if (ar_physics_paused)
        return false;
    if (ar_sim_state != Actor::SimState::LOCAL_SIMULATED)
        return false;

    if (doUpdate)
        this->ToggleHooks(-2, HOOK_LOCK, -1);

    this->CalcHooks();
    this->CalcRopes();

    return true;
}

template <size_t L>
void LogNodeId(RoR::Str<L>& msg, node_t* node) // Internal helper
{
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

void Actor::CalcBeams(bool trigger_hooks)
{
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

            // Calculate beam's rate of change
            float v = (ar_beams[i].p1->Velocity - ar_beams[i].p2->Velocity).dotProduct(dis) * inverted_dislen;

            if (ar_beams[i].bounded == SHOCK1)
            {
                float interp_ratio = 0.0f;

                // Following code interpolates between defined beam parameters and default beam parameters
                if (difftoBeamL > ar_beams[i].longbound * ar_beams[i].L)
                    interp_ratio = difftoBeamL - ar_beams[i].longbound * ar_beams[i].L;
                else if (difftoBeamL < -ar_beams[i].shortbound * ar_beams[i].L)
                    interp_ratio = -difftoBeamL - ar_beams[i].shortbound * ar_beams[i].L;

                if (interp_ratio != 0.0f)
                {
                    // Hard (normal) shock bump
                    float tspring = DEFAULT_SPRING;
                    float tdamp = DEFAULT_DAMP;

                    // Skip camera, wheels or any other shocks which are not generated in a shocks or shocks2 section
                    if (ar_beams[i].bm_type == BEAM_HYDRO)
                    {
                        tspring = ar_beams[i].shock->sbd_spring;
                        tdamp = ar_beams[i].shock->sbd_damp;
                    }

                    k += (tspring - k) * interp_ratio;
                    d += (tdamp - d) * interp_ratio;
                }
            }
            else if (ar_beams[i].bounded == TRIGGER)
            {
                this->CalcTriggers(i, difftoBeamL, trigger_hooks);
            }
            else if (ar_beams[i].bounded == SHOCK2)
            {
                this->CalcShocks2(i, difftoBeamL, k, d, v);
            }
            else if (ar_beams[i].bounded == SHOCK3)
            {
                this->CalcShocks3(i, difftoBeamL, k, d, v);
            }
            else if (ar_beams[i].bounded == SUPPORTBEAM)
            {
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
            }
            else if (ar_beams[i].bounded == ROPE)
            {
                if (difftoBeamL < 0.0f)
                {
                    k = 0.0f;
                    d *= 0.1f;
                }
            }

            if (trigger_hooks && ar_beams[i].bounded && ar_beams[i].bm_type == BEAM_HYDRO)
            {
                ar_beams[i].debug_k = k * std::abs(difftoBeamL);
                ar_beams[i].debug_d = d * std::abs(v);
                ar_beams[i].debug_v = std::abs(v);
            }

            float slen = -k * difftoBeamL - d * v;
            ar_beams[i].stress = slen;

            // Fast test for deformation
            float len = std::abs(slen);
            if (len > ar_beams[i].minmaxposnegstress)
            {
                if (ar_beams[i].bm_type == BEAM_NORMAL && ar_beams[i].bounded != SHOCK1 && k != 0.0f)
                {
                    // Actual deformation tests
                    if (slen > ar_beams[i].maxposstress && difftoBeamL < 0.0f) // compression
                    {
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

                    //Break the beam only when it is not connected to a node
                    //which is a part of a collision triangle and has 2 "live" beams or less
                    //connected to it.
                    if (!((ar_beams[i].p1->nd_cab_node && GetNumActiveConnectedBeams(ar_beams[i].p1->pos) < 3) || (ar_beams[i].p2->nd_cab_node && GetNumActiveConnectedBeams(ar_beams[i].p2->pos) < 3)))
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
                            m_buoyance->sink = true;
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

void Actor::CalcBeamsInterActor()
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
                if (ar_inter_beams[i]->bm_type == BEAM_NORMAL && ar_inter_beams[i]->bounded != SHOCK1 && k != 0.0f)
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
                    if (!((ar_inter_beams[i]->p1->nd_cab_node && GetNumActiveConnectedBeams(ar_inter_beams[i]->p1->pos) < 3) || (ar_inter_beams[i]->p2->nd_cab_node && GetNumActiveConnectedBeams(ar_inter_beams[i]->p2->pos) < 3)))
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

void Actor::CalcNodes()
{
    const auto water = App::GetSimTerrain()->getWater();
    const float gravity = App::GetSimTerrain()->getGravity();
    m_water_contact = false;

    for (NodeIdx_t i = 0; i < ar_num_nodes; i++)
    {
        // COLLISION
        if (!ar_nodes[i].nd_no_ground_contact)
        {
            Vector3 oripos = ar_nodes[i].AbsPosition;
            bool contacted = App::GetSimTerrain()->GetCollisions()->groundCollision(&ar_nodes[i], PHYSICS_DT);
            contacted = contacted | App::GetSimTerrain()->GetCollisions()->nodeCollision(&ar_nodes[i], PHYSICS_DT, false);
            ar_nodes[i].nd_has_ground_contact = contacted;
            if (ar_nodes[i].nd_has_ground_contact || ar_nodes[i].nd_has_mesh_contact)
            {
                ar_last_fuzzy_ground_model = ar_nodes[i].nd_last_collision_gm;
                // Reverts: commit/d11a88142f737528638bd357c38d717c85cebba6#diff-4003254e55aec2c60d21228f375f2a2dL1153
                // Fixes: Gavril Omega Six sliding on ground on the simple2 spawn
                // ar_nodes[i].AbsPosition - oripos is always zero ... dark floating point magic
                ar_nodes[i].RelPosition += ar_nodes[i].AbsPosition - oripos;
            }
        }

        if (i == ar_main_camera_node_pos)
        {
            // record g forces on cameras
            m_camera_gforces_accu += ar_nodes[i].Forces / ar_nodes[i].mass;
            // trigger script callbacks
            App::GetSimTerrain()->GetCollisions()->nodeCollision(&ar_nodes[i], PHYSICS_DT, true);
        }

        // integration
        if (!ar_nodes[i].nd_immovable)
        {
            ar_nodes[i].Velocity += ar_nodes[i].Forces / ar_nodes[i].mass * PHYSICS_DT;
            ar_nodes[i].RelPosition += ar_nodes[i].Velocity * PHYSICS_DT;
            ar_nodes[i].AbsPosition = ar_origin;
            ar_nodes[i].AbsPosition += ar_nodes[i].RelPosition;
        }

        // prepare next loop (optimisation)
        // we start forces from zero
        // start with gravity
        ar_nodes[i].Forces = Vector3(0, ar_nodes[i].mass * gravity, 0);

        Real approx_speed = approx_sqrt(ar_nodes[i].Velocity.squaredLength());

        // anti-explsion guard (mach 20)
        if (approx_speed > 6860 && !m_ongoing_reset)
        {
            ActorModifyRequest* rq = new ActorModifyRequest; // actor exploded, schedule reset
            rq->amr_actor = this;
            rq->amr_type = ActorModifyRequest::Type::RESET_ON_SPOT;
            App::GetGameContext()->PushMessage(Message(MSG_SIM_MODIFY_ACTOR_REQUESTED, (void*)rq));
            m_ongoing_reset = true;
        }

        if (m_fusealge_airfoil)
        {
            // aerodynamics on steroids!
            ar_nodes[i].Forces += ar_fusedrag;
        }
        else if (!ar_disable_aerodyn_turbulent_drag)
        {
            // add viscous drag (turbulent model)
            Real defdragxspeed = DEFAULT_DRAG * approx_speed;
            Vector3 drag = -defdragxspeed * ar_nodes[i].Velocity;
            // plus: turbulences
            Real maxtur = defdragxspeed * approx_speed * 0.005f;
            drag += maxtur * Vector3(frand_11(), frand_11(), frand_11());
            ar_nodes[i].Forces += drag;
        }

        if (water)
        {
            const bool is_under_water = water->IsUnderWater(ar_nodes[i].AbsPosition);
            if (is_under_water)
            {
                m_water_contact = true;
                if (ar_num_buoycabs == 0)
                {
                    // water drag (turbulent)
                    ar_nodes[i].Forces -= (DEFAULT_WATERDRAG * approx_speed) * ar_nodes[i].Velocity;
                    // basic buoyance
                    ar_nodes[i].Forces += ar_nodes[i].buoyancy * Vector3::UNIT_Y;
                }
                // engine stall
                if (i == ar_cinecam_node[0] && ar_engine)
                {
                    ar_engine->StopEngine();
                }
            }
            ar_nodes[i].nd_under_water = is_under_water;
        }
    }

    this->UpdateBoundingBoxes();
}

void Actor::CalcHooks()
{
    //locks - this is not active in network mode
    for (std::vector<hook_t>::iterator it = ar_hooks.begin(); it != ar_hooks.end(); it++)
    {
        //we need to do this here to avoid countdown speedup by triggers
        it->hk_timer = std::max(0.0f, it->hk_timer - PHYSICS_DT);

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
                if (it->hk_beam->L < it->hk_min_length)
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

void Actor::CalcRopes()
{
    for (auto r : ar_ropes)
    {
        if (r.rp_locked == LOCKED && r.rp_locked_ropable)
        {
            auto locked_node = r.rp_locked_ropable->node;
            r.rp_beam->p2->AbsPosition = locked_node->AbsPosition;
            r.rp_beam->p2->RelPosition = locked_node->AbsPosition - ar_origin;
            r.rp_beam->p2->Velocity    = locked_node->Velocity;
            locked_node->Forces       += r.rp_beam->p2->Forces;
            r.rp_beam->p2->Forces      = Vector3::ZERO;
        }
    }
}

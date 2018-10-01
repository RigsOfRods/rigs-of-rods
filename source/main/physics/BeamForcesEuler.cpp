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

void Actor::calcForcesEulerCompute(int step, int num_steps)
{
    const bool doUpdate = (step == 0);
    const float dt = static_cast<float>(PHYSICS_DT);
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

    calcBeams(doUpdate);

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

        for (hydrobeam_t& hydrobeam: ar_hydros)
        {
            beam_t* beam = &ar_beams[hydrobeam.hb_beam_index];
            if ((hydrobeam.hb_flags & (HYDRO_FLAG_DIR | HYDRO_FLAG_SPEED)) && !beam->bm_broken)
            {
                m_force_sensors.accu_hydros_forces += hydrobeam.hb_speed * beam->refL * beam->stress;
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

    this->CalcNodes();
    this->UpdateBoundingBoxes();

    // anti-explosion guard
    // rationale behind 1e9 number:
    // - while 1e6 is reachable by a fast vehicle, it will be badly deformed and shaking due to loss of precision in calculations
    // - at 1e7 any typical RoR vehicle falls apart and stops functioning
    // - 1e9 may be reachable only by a vehicle that is 1000 times bigger than a typical RoR vehicle, and it will be a loooong trip
    // to be able to travel such long distances will require switching physics calculations to higher precision numbers
    // or taking a different approach to the simulation (actor-local coordinate system?)
    if (!inRange(ar_bounding_box.getMinimum().x + ar_bounding_box.getMaximum().x +
        ar_bounding_box.getMinimum().y + ar_bounding_box.getMaximum().y +
        ar_bounding_box.getMinimum().z + ar_bounding_box.getMaximum().z, -1e9, 1e9))
    {
        m_reset_request = REQUEST_RESET_ON_INIT_POS; // actor exploded, schedule reset
        return; // return early to avoid propagating invalid values
    }

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

    float engine_torque = 0.0;
    if (ar_engine && m_num_proped_wheels > 0)
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

    m_antilockbrake = false;
    m_tractioncontrol = false;

    for (int i = 0; i < ar_num_wheels; i++)
    {
        ar_wheels[i].wh_avg_speed = ar_wheels[i].wh_avg_speed * 0.995 + ar_wheels[i].wh_speed * 0.005;

        // total torque estimation
        Real total_torque = 0.0;
        if (ar_wheels[i].wh_propulsed > 0)
        {
            total_torque = (m_num_axles == 0) ? engine_torque : intertorque[i];
        }

        float curspeed = ar_nodes[0].Velocity.length();
        float wheel_slip = fabs(ar_wheels[i].wh_speed - curspeed) / std::max(1.0f, curspeed);

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

            if ((ar_brake != 0.0 || dbrake != 0.0 || hbrake != 0.0) && m_num_braked_wheels != 0 )
            {
                float brake_coef = 1.0f;
                float antilock_coef = 1.0f;

                // anti-lock braking
                if (alb_mode && curspeed > alb_minspeed && curspeed > fabs(ar_wheels[i].wh_speed) && (ar_brake + dbrake > 0.0f) && wheel_slip > 0.25f) 
                {
                    if (alb_pulse_state)
                    {
                        ar_wheels[i].wh_antilock_coef = fabs(ar_wheels[i].wh_speed) / curspeed;
                        ar_wheels[i].wh_antilock_coef = pow(ar_wheels[i].wh_antilock_coef, alb_ratio);
                    }
                    antilock_coef = ar_wheels[i].wh_antilock_coef;
                    m_antilockbrake = std::max(m_antilockbrake, antilock_coef < 0.9);
                }

                // anti-jitter / anti-skidding
                if (fabs(ar_wheels[i].wh_avg_speed) < 0.2f)
                {
                    brake_coef *= pow(fabs(ar_wheels[i].wh_avg_speed) * 5.0f, 0.25f);
                    float speed_diff = ar_wheels[i].wh_speed - ar_wheels[i].wh_last_speed;
                    float speed_prediction = ar_wheels[i].wh_speed + speed_diff;
                    if (speed_prediction * ar_wheels[i].wh_avg_speed < 0.0f)
                        brake_coef = 0.0f;
                }

                if (ar_wheels[i].wh_avg_speed > 0)
                    total_torque -= ((ar_brake + dbrake) * antilock_coef + hbrake) * brake_coef;
                else
                    total_torque += ((ar_brake + dbrake) * antilock_coef + hbrake) * brake_coef;
            }
            else
            {
                ar_wheels[i].wh_antilock_coef = 1.0f;
            }
        }

        // traction control
        if (tc_mode && fabs(engine_torque) > 0.0f && ar_wheels[i].wh_propulsed && fabs(ar_wheels[i].wh_speed) > curspeed && wheel_slip > 0.25f)
        {
            if (tc_pulse_state)
            {
                ar_wheels[i].wh_tc_coef = curspeed / fabs(ar_wheels[i].wh_speed);
                ar_wheels[i].wh_tc_coef = pow(ar_wheels[i].wh_tc_coef, tc_ratio);
            }
            float tc_coef = pow(ar_wheels[i].wh_tc_coef, std::min(std::max(0.2f, curspeed), 1.0f));
            total_torque *= pow(tc_coef, 1.0f / std::max(1.0f, std::min(wheel_slip, 10.0f)));
            m_tractioncontrol = std::max(m_tractioncontrol, ar_wheels[i].wh_tc_coef < 0.9);
        }
        else
        {
            ar_wheels[i].wh_tc_coef = 1.0f;
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

        Real speedacc = 0.0f;
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

    if (step == num_steps)
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

    ar_wheel_speed = 0.0f;
    for (int i = 0; i < ar_num_wheels; i++)
    {
        ar_wheels[i].wh_last_speed = ar_wheels[i].wh_speed;
        ar_wheels[i].wh_speed = newspeeds[i];
        if (ar_wheels[i].wh_propulsed == 1)
        {
            ar_wheel_speed += newspeeds[i] / (float)m_num_proped_wheels;
        }
    }
    ar_avg_wheel_speed = ar_avg_wheel_speed * 0.995 + ar_wheel_speed * 0.005;

    if (ar_engine && ar_num_wheels && ar_wheels[0].wh_radius > 0.0f)
    {
        ar_engine->SetWheelSpin(ar_wheel_speed / ar_wheels[0].wh_radius * RAD_PER_SEC_TO_RPM);
    }

    // calculate driven distance
    float distance_driven = fabs(ar_wheel_speed * dt);
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
        m_stabilizer_shock_sleep -= dt * num_steps;

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

    //direction
    if (ar_hydro_dir_state != 0 || ar_hydro_dir_command != 0)
    {
        float rate = 1;
        if (ar_hydro_speed_coupling)
        {
            //new rate value (30 instead of 40) to deal with the changed cmdKeyInertia settings
            rate = 30.0 / (10.0 + fabs(ar_wheel_speed / 2.0));
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
        // Animators following, if no animator, skip all the tests...
        int flagstate = hydrobeam.hb_anim_flags;
        if (flagstate)
        {
            calcAnimators(flagstate, cstate, div, dt, 0.0f, 0.0f, hydrobeam.hb_anim_param);
        }

        if (div)
        {
            cstate /= (float)div;

            if (m_hydro_inertia)
                cstate = m_hydro_inertia->calcCmdKeyDelay(cstate, i, dt);

            if (!(hydrobeam.hb_flags & HYDRO_FLAG_SPEED) && !flagstate)
                ar_hydro_dir_wheel_display = cstate;

            float factor = 1.0 - cstate * hydrobeam.hb_speed;

            // check and apply animators limits if set
            if (flagstate)
            {
                if (factor < 1.0f - ar_beams[beam_idx].shortbound)
                    factor = 1.0f - ar_beams[beam_idx].shortbound;
                if (factor > 1.0f + ar_beams[beam_idx].longbound)
                    factor = 1.0f + ar_beams[beam_idx].longbound;
            }

            ar_beams[beam_idx].L = hydrobeam.hb_ref_length * factor;
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

                        if (m_command_inertia)
                            v = m_command_inertia->calcCmdKeyDelay(v, i, dt);

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

                        if (cmd_beam.cmb_engine_coupling > 0)
                            cf = crankfactor;

                        if (bbeam_dir > 0)
                            ar_beams[bbeam].L *= (1.0 + cmd_beam.cmb_speed * v * cf * dt / ar_beams[bbeam].L);
                        else
                            ar_beams[bbeam].L *= (1.0 - cmd_beam.cmb_speed * v * cf * dt / ar_beams[bbeam].L);

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
        if (clen > it->ti_min_length)
        {
            it->ti_beam->L *= (1.0 - it->ti_contract_speed * dt / it->ti_beam->L);
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

bool Actor::CalcForcesEulerPrepare()
{
    if (m_reset_request)
        return false;
    if (ar_sim_state != Actor::SimState::LOCAL_SIMULATED)
        return false;

    this->forwardCommands();
    this->CalcBeamsInterActor();
    return true;
}

void Actor::calcForcesEulerFinal()
{
    this->calcHooks();
    this->calcRopes();
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

void Actor::calcBeams(bool trigger_hooks)
{
    const float dt = static_cast<float>(PHYSICS_DT);

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
                    if (ar_beams[i].bm_type == BEAM_HYDRO)
                    {
                        tspring = ar_beams[i].shock->sbd_spring;
                        tdamp = ar_beams[i].shock->sbd_damp;
                    }

                    k += (tspring - k) * interp_ratio;
                    d += (tdamp - d) * interp_ratio;
                }
                break;

            case SHOCK2:
                calcShocks2(i, difftoBeamL, k, d, trigger_hooks);
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
                if (ar_beams[i].bm_type == BEAM_NORMAL && ar_beams[i].bounded != SHOCK1 && k != 0.0f)
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
                    if (!((ar_beams[i].p1->nd_contacter && GetNumActiveConnectedBeams(ar_beams[i].p1->pos) < 3) || (ar_beams[i].p2->nd_contacter && GetNumActiveConnectedBeams(ar_beams[i].p2->pos) < 3)))
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
                    if (!((ar_inter_beams[i]->p1->nd_contacter && GetNumActiveConnectedBeams(ar_inter_beams[i]->p1->pos) < 3) || (ar_inter_beams[i]->p2->nd_contacter && GetNumActiveConnectedBeams(ar_inter_beams[i]->p2->pos) < 3)))
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
    const float dt = static_cast<float>(PHYSICS_DT);
    IWater* water = App::GetSimTerrain()->getWater();
    const float gravity = App::GetSimTerrain()->getGravity();

    for (int i = 0; i < ar_num_nodes; i++)
    {
        // COLLISION
        if (!ar_nodes[i].nd_no_ground_contact)
        {
            ar_nodes[i].collTestTimer += dt;
            if (ar_nodes[i].nd_has_contact || ar_nodes[i].collTestTimer > 0.005 || ((ar_nodes[i].iswheel || ar_nodes[i].wheelid != -1) && (m_high_res_wheelnode_collisions || ar_nodes[i].collTestTimer > 0.0025)) || m_increased_accuracy)
            {
                float ns = 0;
                ground_model_t* gm = nullptr; // this is used as result storage, so we can use it later on
                bool contacted = gEnv->collisions->groundCollision(&ar_nodes[i], ar_nodes[i].collTestTimer, &gm, &ns);
                // reverted this construct to the old form, don't mess with it, the binary operator is intentionally!
                if (contacted | gEnv->collisions->nodeCollision(&ar_nodes[i], contacted, ar_nodes[i].collTestTimer, &ns, &gm))
                {
                    m_last_fuzzy_ground_model = gm;
                    ar_nodes[i].nd_collision_gm = gm;
                    ar_nodes[i].nd_collision_slip = ns;
                }
                else
                {
                    ar_nodes[i].nd_collision_gm = nullptr;
                    ar_nodes[i].nd_collision_slip = 0.f;
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
        if (!ar_nodes[i].nd_immovable)
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
            const bool is_under_water = water->IsUnderWater(ar_nodes[i].AbsPosition);
            if (is_under_water)
            {
                m_water_contact = true;
                if (ar_num_buoycabs == 0)
                {
                    // water drag (turbulent)
                    Real speed = approx_sqrt(ar_nodes[i].Velocity.squaredLength()); //we will (not) reuse this
                    ar_nodes[i].Forces -= (DEFAULT_WATERDRAG * speed) * ar_nodes[i].Velocity;
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
}

void Actor::forwardCommands()
{
    Actor* current_truck = RoR::App::GetSimController()->GetPlayerActor();
    auto bf = RoR::App::GetSimController()->GetBeamFactory();

    // forward things to trailers
    if (this == current_truck && ar_forward_commands)
    {
        for (auto actor : RoR::App::GetSimController()->GetActors())
        {
            if (actor != current_truck && actor->ar_import_commands)
            {
                // forward commands
                for (int j = 1; j <= MAX_COMMANDS; j++)
                {
                    actor->ar_command_key[j].playerInputValue = std::max(ar_command_key[j].playerInputValue, ar_command_key[j].commandValue);
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

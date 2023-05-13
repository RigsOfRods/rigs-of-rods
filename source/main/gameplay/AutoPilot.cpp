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

#include "AutoPilot.h"

#include "Application.h"
#include "GameContext.h"
#include "SimData.h"
#include "SoundScriptManager.h"
#include "Terrain.h"
#include "Water.h"

using namespace Ogre;
using namespace RoR;

Autopilot::Autopilot(ActorPtr a):
    m_actor(a)
{
    ref_span = 1.0f;
    reset();
}

void Autopilot::reset()
{
    mode_heading = HEADING_NONE;
    mode_alt = ALT_NONE;
    mode_ias = false;
    mode_gpws = true;
    heading = 0;
    alt = 1000;
    vs = 0;
    ias = 150;
    last_elevator = 0;
    last_rudder = 0;
    last_aileron = 0;
    last_gpws_height = 0;
    last_pullup_height = 0;
    m_ils_angle_vdev = -90;
    m_ils_angle_hdev = -90;
    m_ils_runway_heading = 0;
    m_ils_runway_distance = 0;
    last_closest_hdist = 0;
    wantsdisconnect = false;

    m_vertical_locator_available = false;
    m_horizontal_locator_available = false;
}

void Autopilot::disconnect()
{
    mode_heading = HEADING_NONE;
    mode_alt = ALT_NONE;
    mode_ias = false;
    wantsdisconnect = false;
    if (mode_gpws)
    {
        SOUND_PLAY_ONCE(m_actor, SS_TRIG_GPWS_APDISCONNECT);
    }
}

void Autopilot::setInertialReferences(NodeNum_t refl, NodeNum_t refr, NodeNum_t refb, NodeNum_t refc)
{
    ref_l = refl;
    ref_r = refr;
    ref_b = refb;
    ref_c = refc; // ar_camera_node_pos(0)
    ref_span = (m_actor->ar_nodes[refl].RelPosition - m_actor->ar_nodes[ref_r].RelPosition).length();
}

float Autopilot::getAilerons()
{
    float val = 0;
    if (ref_l && ref_r)
    {
        float rat = (m_actor->ar_nodes[ref_r].RelPosition.y - m_actor->ar_nodes[ref_l].RelPosition.y) / ref_span;
        float bank = 90.0;
        if (rat >= 1.0)
            bank = 90.0;
        else if (rat <= -1.0)
            bank = -90.0;
        else
            bank = 57.3 * asin(rat);
        if (mode_heading == HEADING_WLV)
        {
            val = bank / 100.0;
            if (val > 0.5)
                val = 0.5;
            if (val < -0.5)
                val = -0.5;
        }
        if (mode_heading == HEADING_FIXED)
        {
            Vector3 vel = (m_actor->ar_nodes[ref_l].Velocity + m_actor->ar_nodes[ref_r].Velocity) / 2.0;
            float curdir = atan2(vel.x, -vel.z) * 57.295779513082;
            float want_bank = curdir - (float)heading;
            if (want_bank < -180.0)
                want_bank += 360.0;
            want_bank = want_bank * 2.0;
            if (want_bank > 45.0)
                want_bank = 45.0;
            if (want_bank < -45.0)
                want_bank = -45.0;
            val = (bank - want_bank) / 100.0;
            if (val > 0.5)
                val = 0.5;
            if (val < -0.5)
                val = -0.5;
        }
        if (mode_heading == HEADING_NAV)
        {
            //compute intercept heading
            float error_heading = m_ils_angle_hdev / 10.0;
            if (error_heading > 1.0)
                error_heading = 1.0;
            if (error_heading < -1.0)
                error_heading = -1.0;
            float offcourse_tolerance = m_ils_runway_distance / 30.0;
            if (offcourse_tolerance > 60.0)
                offcourse_tolerance = 60.0;
            float intercept_heading = m_ils_runway_heading + error_heading * offcourse_tolerance;
            Vector3 vel = (m_actor->ar_nodes[ref_l].Velocity + m_actor->ar_nodes[ref_r].Velocity) / 2.0;
            float curdir = atan2(vel.x, -vel.z) * 57.295779513082;
            float want_bank = curdir - intercept_heading;
            if (want_bank < -180.0)
                want_bank += 360.0;
            want_bank = want_bank * 2.0;
            if (want_bank > 45.0)
                want_bank = 45.0;
            if (want_bank < -45.0)
                want_bank = -45.0;
            val = (bank - want_bank) / 100.0;
            if (val > 0.5)
                val = 0.5;
            if (val < -0.5)
                val = -0.5;
        }
    }
    last_aileron = (last_aileron + val) / 2.0;
    return last_aileron;
}

float Autopilot::getElevator()
{
    float val = 0;
    if (ref_l && ref_r && ref_b)
    {
        float wanted_vs = (float)vs / 196.87;
        float current_vs = (m_actor->ar_nodes[ref_l].Velocity.y + m_actor->ar_nodes[ref_r].Velocity.y) / 2.0;
        float pitch_var = current_vs - m_actor->ar_nodes[ref_b].Velocity.y;
        if (mode_alt == ALT_VS)
        {
            if (mode_heading == HEADING_NAV)
            {
                //this is cheating a bit
                float ch = m_ils_runway_distance * sin((m_ils_angle_vdev + 4.0) / 57.295779513082);
                float oh = m_ils_runway_distance * sin((4.0) / 57.295779513082);
                wanted_vs = 5000.0 / 196.87;
                float wanted_vs2 = (-ch + oh) / 5.0;
                if (wanted_vs2 < -wanted_vs)
                    wanted_vs2 = -wanted_vs;
                if (wanted_vs2 > wanted_vs)
                    wanted_vs2 = wanted_vs;
                val = (wanted_vs2 - current_vs) / 40.0 + pitch_var / 40.0;
                if (val > 0.75)
                    val = 0.75;
                if (val < -0.75)
                    val = -0.75;
            }
            else
            {
                val = (wanted_vs - current_vs) / 40.0 + pitch_var / 40.0;
                if (val > 0.5)
                    val = 0.5;
                if (val < -0.5)
                    val = -0.5;
            }
        }
        if (mode_alt == ALT_FIXED)
        {
            float wanted_alt = (float)alt * 0.3048;
            float current_alt = (m_actor->ar_nodes[ref_l].AbsPosition.y + m_actor->ar_nodes[ref_r].AbsPosition.y) / 2.0;
            if (wanted_vs < 0)
                wanted_vs = -wanted_vs; //absolute value
            float wanted_vs2 = (wanted_alt - current_alt) / 8.0;
            if (wanted_vs2 < -wanted_vs)
                wanted_vs2 = -wanted_vs;
            if (wanted_vs2 > wanted_vs)
                wanted_vs2 = wanted_vs;
            val = (wanted_vs2 - current_vs) / 40.0 + pitch_var / 40.0;
            if (val > 0.5)
                val = 0.5;
            if (val < -0.5)
                val = -0.5;
        }
    }
    return val;
}

float Autopilot::getRudder()
{
    return 0;
}

float Autopilot::getThrottle(float thrtl, float dt)
{
    if (!mode_ias) { return thrtl; };

    float val = thrtl;
    if (ref_l && ref_r)
    {
        //tropospheric model valid up to 11.000m (33.000ft)
        float altitude = m_actor->ar_nodes[ref_l].AbsPosition.y;
        //float sea_level_temperature=273.15+15.0; //in Kelvin
        float sea_level_pressure = 101325; //in Pa
        //float airtemperature=sea_level_temperature-altitude*0.0065; //in Kelvin
        float airpressure = sea_level_pressure * pow(1.0 - 0.0065 * altitude / 288.15, 5.24947); //in Pa
        float airdensity = airpressure * 0.0000120896;//1.225 at sea level

        float gspd = 1.94384449 * ((m_actor->ar_nodes[ref_l].Velocity + m_actor->ar_nodes[ref_r].Velocity) / 2.0).length();

        float spd = gspd * sqrt(airdensity / 1.225); //KIAS

        if (spd > (float)ias)
            val = val - dt / 15.0;
        if (spd < (float)ias)
            val = val + dt / 15.0;
        if (val > 1.0)
            val = 1.0;
        if (val < 0.02)
            val = 0.02;
    }
    return val;
}

int Autopilot::toggleHeading(int mode)
{
    if (mode == mode_heading)
        mode_heading = HEADING_NONE;
    else
        mode_heading = mode;
    return mode_heading;
}

int Autopilot::toggleAlt(int mode)
{
    if (mode == mode_alt)
        mode_alt = ALT_NONE;
    else
        mode_alt = mode;
    return mode_alt;
}

bool Autopilot::toggleIAS()
{
    mode_ias = !mode_ias;
    return mode_ias;
}

bool Autopilot::toggleGPWS()
{
    mode_gpws = !mode_gpws;
    return mode_gpws;
}

int Autopilot::adjHDG(int d)
{
    heading += d;
    if (heading < 0)
        heading += 360;
    if (heading > 359)
        heading -= 360;
    return heading;
}

int Autopilot::adjALT(int d)
{
    alt += d;
    return alt;
}

int Autopilot::adjVS(int d)
{
    vs += d;
    if (vs > 9900)
        vs = 9900;
    if (vs < -9900)
        vs = -9900;
    return vs;
}

int Autopilot::adjIAS(int d)
{
    ias += d;
    if (ias < 0)
        ias = 0;
    if (ias > 350)
        ias = 350;
    return ias;
}

void Autopilot::gpws_update(float spawnheight)
{
#ifdef USE_OPENAL
    if (App::GetSoundScriptManager()->isDisabled())
        return;
    if (mode_gpws && ref_b)
    {
        float groundalt = App::GetGameContext()->GetTerrain()->GetHeightAt(m_actor->ar_nodes[ref_c].AbsPosition.x, m_actor->ar_nodes[ref_c].AbsPosition.z);
        if (App::GetGameContext()->GetTerrain()->getWater() && groundalt < App::GetGameContext()->GetTerrain()->getWater()->GetStaticWaterHeight())
            groundalt = App::GetGameContext()->GetTerrain()->getWater()->GetStaticWaterHeight();
        float height = (m_actor->ar_nodes[ref_c].AbsPosition.y - groundalt - spawnheight) * 3.28083f; //in feet!
        //skip height warning sounds when the plane is slower then ~10 knots
        if ((m_actor->ar_nodes[ref_c].Velocity.length() * 1.9685f) > 10.0f)
        {
            if (height < 10 && last_gpws_height > 10)
                SOUND_PLAY_ONCE(m_actor, SS_TRIG_GPWS_10);
            if (height < 20 && last_gpws_height > 20)
                SOUND_PLAY_ONCE(m_actor, SS_TRIG_GPWS_20);
            if (height < 30 && last_gpws_height > 30)
                SOUND_PLAY_ONCE(m_actor, SS_TRIG_GPWS_30);
            if (height < 40 && last_gpws_height > 40)
                SOUND_PLAY_ONCE(m_actor, SS_TRIG_GPWS_40);
            if (height < 50 && last_gpws_height > 50)
                SOUND_PLAY_ONCE(m_actor, SS_TRIG_GPWS_50);
            if (height < 100 && last_gpws_height > 100)
                SOUND_PLAY_ONCE(m_actor, SS_TRIG_GPWS_100);
        }
        last_gpws_height = height;

        // pullup warning calculation
        // height to meters
        height *= 0.3048;
        // get the y-velocity in meters/s
        float yVel = m_actor->ar_nodes[ref_c].Velocity.y * 1.9685f;
        // will trigger the pullup sound when vvi is high (avoid pullup warning when landing normal) and groundcontact will be in less then 10 seconds
        if (yVel * 10.0f < -height && yVel < -10.0f)
            SOUND_PLAY_ONCE(m_actor, SS_TRIG_GPWS_PULLUP);
    }
#endif //OPENAL
}

void Autopilot::UpdateIls(std::vector<TerrainObjectManager::localizer_t> localizers)
{
    if (!ref_l || !ref_r)
        return;
    Vector3 position = (m_actor->ar_nodes[ref_l].AbsPosition + m_actor->ar_nodes[ref_r].AbsPosition) / 2.0;
    float closest_hdist = -1;
    float closest_hangle = -90;
    float closest_vdist = -1;
    float closest_vangle = -90;
    m_ils_runway_heading = 0;
    for (std::vector<TerrainObjectManager::localizer_t>::size_type i = 0; i < localizers.size(); i++)
    {
        Plane hplane = Plane(Vector3::UNIT_Y, 0);
        Vector3 plocd = hplane.projectVector(localizers[i].rotation * Vector3::UNIT_Z);
        float loc = atan2(plocd.z, plocd.x);
        Vector3 posd = hplane.projectVector(position - localizers[i].position);
        float dir = atan2(posd.z, posd.x);
        float diff = (dir - loc) * 57.295779513082;
        if (diff > 180.0)
            diff -= 360.0;
        if (diff < -180.0)
            diff += 360.0;
        if (diff < 80 && diff > -80)
        {
            //we are in the visibility cone, we search the closest
            float dist = (localizers[i].position - position).length();
            //horizontal
            if (localizers[i].type == LOCALIZER_HORIZONTAL)
            {
                if (closest_hdist < 0 || closest_hdist > dist)
                {
                    closest_hdist = dist;
                    closest_hangle = diff;
                    m_ils_runway_heading = loc * 57.295779513082 - 90.0; //dont ask me why
                    if (m_ils_runway_heading < 0.0)
                        m_ils_runway_heading += 360.0;
                    m_ils_runway_distance = dist;
                }
            }
            //vertical
            if (localizers[i].type == LOCALIZER_VERTICAL)
            {
                if (closest_vdist < 0 || closest_vdist > dist)
                {
                    Vector3 normv = (localizers[i].rotation * Vector3::UNIT_Z).crossProduct(Vector3::UNIT_Y);
                    Plane vplane = Plane(normv, 0);
                    float glideslope = 4; //4 degree is the norm
                    Vector3 posd2 = vplane.projectVector(position - localizers[i].position);
                    float d = posd2.length();
                    if (d < 0.01)
                        d = 0.01;
                    float dir2 = 90.0;
                    d = posd2.y / d;
                    if (d >= 1.0)
                        dir2 = 90.0;
                    else if (d <= -1.0)
                        dir2 = -90.0;
                    else
                        dir2 = asin(d) * 57.295779513082;
                    float diff2 = dir2 - glideslope;
                    closest_vdist = dist;
                    closest_vangle = diff2;
                }
            }
        }
    }
    m_ils_angle_hdev = closest_hangle;
    m_ils_angle_vdev = closest_vangle;

    m_horizontal_locator_available = (closest_hdist != -1);
    m_vertical_locator_available = (closest_vdist != -1);

    if (mode_heading == HEADING_NAV && mode_gpws && closest_hdist > 10.0 && closest_hdist < 350.0 && last_closest_hdist > 10.0 && last_closest_hdist > 350.0)
    {
        SOUND_PLAY_ONCE(m_actor, SS_TRIG_GPWS_MINIMUMS);
    }

    last_closest_hdist = closest_hdist;
    if (mode_heading == HEADING_NAV)
    {
        // disconnect if close to runway or no locators are available
        if (closest_hdist < 20.0 || closest_vdist < 20.0)
            wantsdisconnect = true;
        if (!this->IsIlsAvailable())
            wantsdisconnect = true;
    }
        
}

/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2016 Petr Ohlidal & contributors

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

#include "ForceFeedback.h"

#include "Application.h"
#include "InputEngine.h"
#include "RoRPrerequisites.h"

#include <OISForceFeedback.h>
#include <OgreString.h>

namespace RoR {

void ForceFeedback::Setup()
{
    using namespace Ogre;
    m_device = App::GetInputEngine()->getForceFeedbackDevice();
    LOG(String("ForceFeedback: ")+TOSTRING(m_device->getFFAxesNumber())+" axe(s)");
    const OIS::ForceFeedback::SupportedEffectList& supEffects = m_device->getSupportedEffects();
    if (supEffects.size() > 0)
    {
        LOG("ForceFeedback: supported effects:");
        OIS::ForceFeedback::SupportedEffectList::const_iterator efit;
#ifdef OISHEAD
        for (efit=supEffects.begin(); efit!=supEffects.end(); ++efit)
            LOG(String("ForceFeedback: ")+OIS::Effect::getEffectTypeName(efit->second));
#endif //OISHEAD
    }
    else
    LOG("ForceFeedback: no supported effect found!");
    m_device->setAutoCenterMode(false);
    m_device->setMasterGain(0.0);

    //do not load effect now, its too early
}

void ForceFeedback::SetForces(float roll, float pitch, float wspeed, float dircommand, float stress)
{
    if (!m_device) { return; }

    //LOG(String("ForceFeedback: R=")+TOSTRING(roll)+" D="+TOSTRING(dir)+" S="+TOSTRING(wspeed)+" H="+TOSTRING(stress));
    if (!m_hydro_effect)
    {
        //we create effect at the last moment, because it does not works otherwise
        m_hydro_effect = new OIS::Effect(OIS::Effect::ConstantForce, OIS::Effect::Constant);
        m_hydro_effect->direction = OIS::Effect::North;
        m_hydro_effect->trigger_button = 0;
        m_hydro_effect->trigger_interval = 0;
        m_hydro_effect->replay_length = OIS::Effect::OIS_INFINITE; // Linux/Win32: Same behaviour as 0.
        m_hydro_effect->replay_delay = 0;
        m_hydro_effect->setNumAxes(1);
        OIS::ConstantEffect* hydroConstForce = dynamic_cast<OIS::ConstantEffect*>(m_hydro_effect->getForceEffect());
        hydroConstForce->level = 0; //-10K to +10k
        hydroConstForce->envelope.attackLength = 0;
        hydroConstForce->envelope.attackLevel = (unsigned short)hydroConstForce->level;
        hydroConstForce->envelope.fadeLength = 0;
        hydroConstForce->envelope.fadeLevel = (unsigned short)hydroConstForce->level;

        m_device->upload(m_hydro_effect);
    }

    OIS::ConstantEffect* hydroConstForce = dynamic_cast<OIS::ConstantEffect*>(m_hydro_effect->getForceEffect());
    float stress_gain = App::io_ffb_stress_gain.GetActive();
    float centering_gain = App::io_ffb_center_gain.GetActive();
    float ff = -stress * stress_gain + dircommand * 100.0 * centering_gain * wspeed * wspeed;
    if (ff > 10000)
        ff = 10000;
    if (ff < -10000)
        ff = -10000;
    hydroConstForce->level = ff; //-10K to +10k
    m_device->modify(m_hydro_effect);
}

void ForceFeedback::SetEnabled(bool b)
{
    if (!m_device) { return; }

    if (b != m_enabled)
    {
        float gain = (b) ? App::io_ffb_master_gain.GetActive() : 0.f;
        m_device->setMasterGain(gain);
    }
    m_enabled = b;
}

} // namespace RoR

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
#include "Differentials.h"
#include "Language.h"

using namespace RoR;

void Differential::ToggleDifferentialMode()
{
    if (m_available_diffs.size() > 1)
    {
        std::rotate(m_available_diffs.begin(), m_available_diffs.begin() + 1, m_available_diffs.end());
    }
}

void Differential::CalcAxleTorque(DifferentialData& diff_data)
{
    if (m_available_diffs.empty())
        return;

    switch (m_available_diffs[0])
    {
    case SPLIT_DIFF:   this->CalcSeparateDiff(diff_data);  return;
    case OPEN_DIFF:    this->CalcOpenDiff(diff_data);      return;
    case VISCOUS_DIFF: this->CalcViscousDiff(diff_data);   return;
    case LOCKED_DIFF:  this->CalcLockedDiff(diff_data);    return;
    }
}

Ogre::UTFString Differential::GetDifferentialTypeName()
{
    if (m_available_diffs.empty())
        return _L("invalid");

    switch (m_available_diffs[0])
    {
    case SPLIT_DIFF:   return _L("Split");
    case OPEN_DIFF:    return _L("Open");
    case VISCOUS_DIFF: return _L("Viscous");
    case LOCKED_DIFF:  return _L("Locked");
    default:           return _L("invalid");
    }
}

void Differential::CalcSeparateDiff(DifferentialData& diff_data)
{
    diff_data.out_torque[0] = diff_data.out_torque[1] = diff_data.in_torque / 2.0f;
}

void Differential::CalcOpenDiff(DifferentialData& diff_data)
{
    /* Open differential calculations *************************
     * These calculation are surprisingly tricky
     * the power ratio is based on normalizing the speed of the
     * wheel. Normalizing is when the sum of all values equals
     * one. more detail provided below
     */

    diff_data.out_torque[0] = diff_data.out_torque[1] = diff_data.in_torque;

    // combined total velocity
    const Ogre::Real sum_of_vel = fabs(diff_data.speed[0]) + fabs(diff_data.speed[1]);

    // minimum velocity
    const Ogre::Real min_of_vel = std::min(fabs(diff_data.speed[0]), fabs(diff_data.speed[1]));

    // normalize the wheel speed, at a speed of 0 power is split evenly
    const Ogre::Real power_ratio = min_of_vel > 1.0f ? fabs(diff_data.speed[0]) / sum_of_vel : 0.5f;

    // Diff model taken from Torcs, ror needs to model reaction torque for this to work.
    //DrTq0 = DrTq*0.5f + spiderTq;
    //DrTq1 = DrTq*0.5f - spiderTq;

    // get the final ratio based on the speed of the wheels
    diff_data.out_torque[0] *= Ogre::Math::Clamp(0.0f + power_ratio, 0.1f, 0.9f);
    diff_data.out_torque[1] *= Ogre::Math::Clamp(1.0f - power_ratio, 0.1f, 0.9f);
}

void Differential::CalcViscousDiff(DifferentialData& diff_data)
{
    /* Viscous axle calculation ********************************
     * Two wheels are joined together by a rotary viscous coupling.
     * The viscous liquid counteracts speed differences in the
     * coupling.
     */

    const Ogre::Real m_torsion_damp = 10000.0f;
    const Ogre::Real delta_speed = diff_data.speed[0] - diff_data.speed[1];

    diff_data.out_torque[0] = diff_data.out_torque[1] = diff_data.in_torque / 2.0f;

    // damping
    diff_data.out_torque[0] -= delta_speed * m_torsion_damp;

    // damping
    diff_data.out_torque[1] += delta_speed * m_torsion_damp;
}

void Differential::CalcLockedDiff(DifferentialData& diff_data)
{
    /* Locked axle calculation ********************************
     * This is straight forward, two wheels are joined together
     * by a torsion spring. the torsion spring keeps the two
     * wheels in the same orientation as when they were first
     * locked.
     */

    // Torsion spring rate that holds axles together when locked
    // keep as variable for now since this value will be user configurable
    const Ogre::Real m_torsion_rate = 1000000.0f;
    const Ogre::Real m_torsion_damp = m_torsion_rate / 100.0f;
    const Ogre::Real delta_speed = diff_data.speed[0] - diff_data.speed[1];

    diff_data.out_torque[0] = diff_data.out_torque[1] = diff_data.in_torque / 2.0f;

    // derive how far wheels traveled relative to each during the last time step
    diff_data.delta_rotation += delta_speed * diff_data.dt;

    // torque cause by axle shafts
    diff_data.out_torque[0] -= diff_data.delta_rotation * m_torsion_rate;
    // damping
    diff_data.out_torque[0] -= delta_speed * m_torsion_damp;

    // torque cause by axle shafts
    diff_data.out_torque[1] += diff_data.delta_rotation * m_torsion_rate;
    // damping
    diff_data.out_torque[1] += delta_speed * m_torsion_damp;
}

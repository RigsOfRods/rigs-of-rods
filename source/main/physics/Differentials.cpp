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
#include "Differentials.h"
#include <OgreLogManager.h>
#include "Language.h"

Axle::Axle() :
	wheel_1(-1),
	wheel_2(-1),
	delta_rotation(0.0f),
	torsion_rate(1000000.0f),
	torsion_damp(torsion_rate/100),
	avg_speed(0.0f),
	gear_ratio(1.0f),
	axle_group(0),
	which_diff(0),
	free_diff(0),
	current_callback(NULL)
{
}

void Axle::addDiffType(DiffType diff)
{
	available_diff_method.push_back(diff);
	if (!current_callback) current_callback = getDiffEquation(diff);
}

const std::vector<DiffType>& Axle::availableDiffs()
{
	return available_diff_method;
}

void Axle::toggleDiff()
{
	which_diff++;
	which_diff %= available_diff_method.size();
	current_callback = getDiffEquation(available_diff_method[which_diff]);
}

void Axle::calcTorque( differential_data_t& diff_data )
{
	if (current_callback) current_callback( diff_data );
}

Ogre::UTFString Axle::getDiffTypeName()
{
	if (available_diff_method.size() <= 0) return "invalid";

	switch(available_diff_method[which_diff])
	{
		case SPLIT_DIFF: return _L("Split"); break;
		case VISCOUS_DIFF: return _L("Fluid"); break;
		case TC_DIFF: return _L("Traction Control"); break;
		case OPEN_DIFF: return _L("Open"); break;
		case LOCKED_DIFF: return _L("Locked"); break;
	}
	return _L("invalid");
}

diff_callback Axle::getDiffEquation(DiffType type)
{
	switch (type)
	{
		case SPLIT_DIFF: return calcSeperatedDiff;
		case VISCOUS_DIFF: return calcViscousDiff;
		case TC_DIFF: return calcViscousDiff;
		case OPEN_DIFF: return calcOpenDiff;
		case LOCKED_DIFF: return calcLockedDiff;
	}
	return calcSeperatedDiff;
}

void Axle::calcSeperatedDiff( differential_data_t& diff_data)
{
	diff_data.out_torque[0] = diff_data.out_torque[1] = diff_data.in_torque;
}

void Axle::calcViscousDiff( differential_data_t& diff_data )
{
	diff_data.out_torque[0] = diff_data.out_torque[1] = diff_data.in_torque;
}

static void calcTCDiff( differential_data_t& diff_data )
{
	calcTCDiff( diff_data );
	Ogre::Real tmp = diff_data.out_torque[0];
	diff_data.out_torque[0] = diff_data.out_torque[1];
	diff_data.out_torque[1] = tmp;
}

void Axle::calcOpenDiff( differential_data_t& diff_data )
{
	/* Open differential calculations *************************
	 * These calculation are surprisingly tricky
	 * the power ratio is based on normalizing the speed of the
	 * wheel. Normalizing is when the sum of all values equals
	 * one. more detail provided below
	 */

	// velocity at which open diff calculations are used 100%
	// this value is twice the speed of the actual threshold
	// the factor of 2 comes from finding the average speed and
	// is done so to remove unessecary calculations
	const Ogre::Real threshold_vel = 10.0f;

	// combined total velocity
	const Ogre::Real sum_of_vel =
		fabs(diff_data.speed[0]) + fabs(diff_data.speed[1]);

	// normalizing the wheel speeds the wheels to sputter at near 0
	// speeds, to over come this we gradually transition from even
	// power distribution to splitting power based on speed. The
	// Transition ratio defines how much of each formula we take from,
	// Until a threshold has been reached
	const Ogre::Real transition_ratio = (sum_of_vel/threshold_vel < 1) ?
			sum_of_vel/threshold_vel : 1;

	// normalize the wheel speed, at a speed of 0 power is split evenly
	const Ogre::Real power_ratio = (fabs(sum_of_vel) > 0.0) ?
			fabs(diff_data.speed[0])/sum_of_vel : 0.5;

	diff_data.out_torque[0] = diff_data.out_torque[1] = diff_data.in_torque;

	// Diff model taken from Torcs, ror needs to model reaction torque for this to work.
	//const Ogre::Real spider_acc = (diff_data.speed[0] - diff_data.speed[1])/diff_data.dt;
	//DrTq0 = DrTq*0.5f + spiderTq;
	//DrTq1 = DrTq*0.5f - spiderTq;

	// get the final ratio based on the speed of the wheels
	diff_data.out_torque[0] *= 2*(transition_ratio*power_ratio + (1-transition_ratio)*0.5f);
	diff_data.out_torque[1] *= 2*(transition_ratio*(1 - power_ratio) + (1-transition_ratio)*0.5f);

	diff_data.delta_rotation = 0.0f;
}

void Axle::calcLockedDiff( differential_data_t& diff_data )
{
	/* Locked axle calculation ********************************
	 * This is straight forward, two wheels are joined together
	 * by a torsion spring. the torsion spring keeps the two
	 * wheels in the same orientation as when they were first
	 * locked.
	 */

	// Torsion spring rate that holds axles together when locked
	// keep as variable for now since this value will be user configurable
	const Ogre::Real torsion_rate = 1000000.0f;
	const Ogre::Real torsion_damp = torsion_rate/100.0f;
	const Ogre::Real delta_speed = diff_data.speed[0] - diff_data.speed[1];

	diff_data.out_torque[0] = diff_data.out_torque[1] = diff_data.in_torque;

	// derive how far wheels traveled relative to each during the last time step
	diff_data.delta_rotation += (delta_speed)*diff_data.dt;

	// torque cause by axle shafts
	diff_data.out_torque[0] -= diff_data.delta_rotation * torsion_rate;
	// damping
	diff_data.out_torque[0] -= (delta_speed)*torsion_damp;

	// torque cause by axle shafts
	diff_data.out_torque[1] += diff_data.delta_rotation * torsion_rate;
	// damping
	diff_data.out_torque[1] -= -(delta_speed)*torsion_damp;
}

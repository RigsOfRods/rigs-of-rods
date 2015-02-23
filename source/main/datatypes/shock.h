/*
 * shock.h
 *
 *  Created on: Dec 29, 2012
 *      Author: chris
 */

#ifndef SHOCK_H_
#define SHOCK_H_

/**
* SIM-CORE; Shock.
*/
struct shock_t
{
	int beamid;
	int flags;
	float lastpos;
	float springin;
	float dampin;
	float sprogin;
	float dprogin;
	float springout;
	float dampout;
	float sprogout;
	float dprogout;
	float sbd_spring;               //!< set beam default for spring
	float sbd_damp;                 //!< set beam default for damping
	int trigger_cmdlong;            //!< F-key for trigger injection longbound-check
	int trigger_cmdshort;           //!< F-key for trigger injection shortbound-check
	bool trigger_enabled;           //!< general trigger,switch and blocker state
	float trigger_switch_state;     //!< needed to avoid doubleswitch, bool and timer in one
	float trigger_boundary_t;       //!< optional value to tune trigger_switch_state autorelease
	int last_debug_state;           //!< smart debug output
};


#endif /* SHOCK_H_ */

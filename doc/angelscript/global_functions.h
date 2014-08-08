/*
This source file is part of Rigs of Rods

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
// created on 25th May 2011.

/**
 * This is an alias for game.log(string message).
 * @see game.log
 */
void log(const string message);

/**
 * This is an alias for game.log(string message).
 * @see game.log
 */
void print(const string message);

/**
 * All the events that can be used by the script.
 * @remarks This is just a placeholder, please look in the RoR source code for the real scriptEvents enum.
 * @see game::registerForEvent
 * @see The scriptEvents enum in the Rigs of Rods source code.
 */
enum scriptEvents {}

/**
 * Different truck states registered with the script.
 */
enum truckStates {
	TS_ACTIVATED,      //!< leading truck
	TS_DESACTIVATED,   //!< not leading but active
	TS_MAYSLEEP,       //!< active but wanting to sleep
	TS_GOSLEEP,        //!< active but ordered to sleep ASAP (synchronously)
	TS_SLEEPING,       //!< not active, sleeping
	TS_NETWORKED,      //!< not calculated, gets remote data
	TS_RECYCLE,        //!< waiting for reusage
	TS_DELETED,        //!< special used when truck pointer is 0
};


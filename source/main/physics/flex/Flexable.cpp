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
#include "Flexable.h"

#include "Beam.h"

bool Flexable::flexitPrepare(Beam* b)
{
	beamid = b;

	return true;
}

void Flexable::run()
{
	flexitCompute();
}

void Flexable::onComplete()
{
	MUTEX_LOCK(&beamid->flexable_task_count_mutex);
	beamid->flexable_task_count--;
	MUTEX_UNLOCK(&beamid->flexable_task_count_mutex);
	if (!beamid->flexable_task_count)
	{
		pthread_cond_signal(&beamid->flexable_task_count_cv);
	}
}

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
#ifndef __ThreadPool_H_
#define __ThreadPool_H_

#include "RoRPrerequisites.h"

class ThreadWorker;
class IThreadTask;

#include <pthread.h>

class ThreadPool : public ZeroedMemoryAllocator
{
	friend class ThreadWorker;

public:

	ThreadPool(int size);
	~ThreadPool();

	int getSize() { return size; };

	void enqueue(IThreadTask* task);
	void enqueue(const std::list<IThreadTask*>& tasks);

protected:

	int size;
	bool stop;

	pthread_cond_t queue_cv;
	pthread_mutex_t queue_mutex;

	std::deque<IThreadTask*> tasks;

	std::vector<ThreadWorker*> workers;
};

#endif // __ThreadPool_H_

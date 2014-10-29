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
#include "ThreadPool.h"

#include "ThreadWorker.h"

ThreadPool::ThreadPool(int size) :
	  size(size)
	, stop(false)
{
	pthread_cond_init(&queue_cv, NULL);
	pthread_mutex_init(&queue_mutex, NULL);

	for (int i=0; i < size; i++)
	{
		workers.push_back(new ThreadWorker(this));
	}
}

ThreadPool::~ThreadPool()
{
	stop = true;
    pthread_cond_broadcast(&queue_cv);
    
    for (size_t i=0; i < workers.size(); i++)
	{
		pthread_join(workers[i]->thread, NULL);
	}

	pthread_cond_destroy(&queue_cv);
	pthread_mutex_destroy(&queue_mutex);
}

void ThreadPool::enqueue(IThreadTask* task)
{
	if (task)
	{
		MUTEX_LOCK(&queue_mutex);
		this->tasks.push_back(task);
		MUTEX_UNLOCK(&queue_mutex);

		pthread_cond_signal(&queue_cv);
	}
}

void ThreadPool::enqueue(const std::list<IThreadTask*>& tasks)
{
	if (!tasks.empty())
	{
		MUTEX_LOCK(&queue_mutex);
		for (std::list<IThreadTask*>::const_iterator it = tasks.begin(); it != tasks.end(); ++it)
		{
			if (*it)
			{
				this->tasks.push_back(*it);
			}
		}
		MUTEX_UNLOCK(&queue_mutex);

		pthread_cond_broadcast(&queue_cv);
	}
}

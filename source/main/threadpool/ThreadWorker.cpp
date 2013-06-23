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
#include "ThreadWorker.h"

#include "Settings.h"
#include "ThreadPool.h"
#include "IThreadTask.h"

#ifdef USE_CRASHRPT
# include "crashrpt.h"
#endif

ThreadWorker::ThreadWorker(ThreadPool* thread_pool)
{
	if (pthread_create(&thread, NULL, ThreadWorker::threadstart, thread_pool))
	{
		LOG("THREADWORKER: Can not start a thread");
		exit(1);
	}
}

ThreadWorker::~ThreadWorker()
{

}

void* ThreadWorker::threadstart(void* vid)
{
#ifdef USE_CRASHRPT
	if (SSETTING("NoCrashRpt").empty())
	{
		// add the crash handler for this thread
		CrThreadAutoInstallHelper cr_thread_install_helper;
		MYASSERT(cr_thread_install_helper.m_nInstallStatus==0);
	}
#endif // USE_CRASHRPT

	ThreadPool *thread_pool = static_cast<ThreadPool*>(vid);

	while (thread_pool)
	{
		MUTEX_LOCK(&thread_pool->queue_mutex);

		while (!thread_pool->stop && thread_pool->tasks.empty())
		{
			pthread_cond_wait(&thread_pool->queue_cv, &thread_pool->queue_mutex);
		}

		if (thread_pool->stop && thread_pool->tasks.empty())
		{
			MUTEX_UNLOCK(&thread_pool->queue_mutex);
			break;
		}

		IThreadTask *task = thread_pool->tasks.front();
		thread_pool->tasks.pop_front();

		MUTEX_UNLOCK(&thread_pool->queue_mutex);

		task->run();
		task->onComplete();
	}

	pthread_exit(NULL);
	return NULL;
}

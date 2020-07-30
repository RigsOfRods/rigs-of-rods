/*
This source file is part of Rigs of Rods
Copyright 2016 Fabian Killus

For more information, see http://www.rigsofrods.org/

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

#pragma once

#include "Application.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <stdexcept>
#include <vector>

namespace RoR {

/** /brief Handle for a task executed by ThreadPool
 *
 * Returned by ThreadPool instance when submitting a new task to run.
 * Provides a thin wrapper around the callable object which implements the actual task.
 * Allows for synchronization, i.e. to wait for the associated task to finish (see join()).
 *
 * \see ThreadPool
 */
class Task
{
    friend class ThreadPool;
    public:
    /// Block the current thread and wait for the associated task to finish.
    void join() const
    {
        // Wait until the tasks is_finished property is set to true by the ThreadPool instance.
        // Three possible scenarios:
        // 1) Execution of task has not started yet
        //    - locks task_mutex
        //    - is_finished will be false
        //    - therefore unlocks task_mutex again and waits until signaled by thread pool
        //    - then is_finished will be true (except in case of spurious wakeups for which the waiting continues)
        // 2) Task is being executed
        //    - will try to lock task_mutex, but will fail
        //    - task_mutex is locked while the task is still running
        //    - is_finished will be true after aquiring the lock
        // 3) Task has already finished execution
        //    - locks task_mutex
        //    - is_finished will be true
        std::unique_lock<std::mutex> lock(m_task_mutex);
        m_finish_cv.wait(lock, [this]{ return m_is_finished; });
    }

    private:
    // Only constructable by friend class ThreadPool
    Task(std::function<void()> task_func) : m_task_func(task_func) {}
    Task(Task &) = delete;
    Task & operator=(Task &) = delete;

    bool m_is_finished = false;                   //!< Indicates whether the task execution has finished.
    mutable std::condition_variable m_finish_cv;  //!< Used to signal the current thread when the task has finished.
    mutable std::mutex m_task_mutex;              //!< Mutex which is locked while the task is running.
    const std::function<void()> m_task_func;      //!< Callable object which implements the task to execute.
};

/** \brief Facilitates execution of (small) tasks on separate threads.
 *
 * Implements a "rent-a-thread" model where each submitted task is assigned to one of several worker threads managed by the thread pool instance.
 * This is especially useful for short running tasks as it avoids the runtime cost of creating and launching a new thread. Notice, there
 * still is a certain overhead present due to the synchronization of threads required in the internal implementation.
 *
 * Usage example 1:
 * \code
 *  ThreadPool tp;
 *  auto task_handle = tp.RunTask([]{ SomeWork() };  // Start asynchronous task
 *  SomeOtherWork();
 *  task_handle.join(); // Wait for async task to finish
 * \endcode
 *
 * Usage example 2:
 * \code
 *  ThreadPool tp;
 *  auto task1 = []{ ... };
 *  auto task2 = std::bind(my_func, arg1, arg2);
 *  tp.Parallelize({task1, task2});  // Run tasks in parallel and wait until all have finished
 * \endcode
 *
 * \see Task
 */
class ThreadPool {
public:
    static ThreadPool* DetectNumWorkersAndCreate()
    {
        // Create general-purpose thread pool
        int logical_cores = std::thread::hardware_concurrency();

        int num_threads = App::app_num_workers->GetInt();
        if (num_threads < 1 || num_threads > logical_cores)
        {
            num_threads = Ogre::Math::Clamp(logical_cores - 1, 1, 8);
            App::app_num_workers->SetVal(num_threads);
        }

        RoR::LogFormat("[RoR|ThreadPool] Found %d logical CPU cores, creating %d worker threads",
                  logical_cores, num_threads);

        return new ThreadPool(num_threads);
    }

    /** \brief Construct thread pool and launch worker threads.
     *
     * @param num_threads Number of worker threads to use
     */
    ThreadPool(int num_threads)
    {
        ROR_ASSERT(num_threads > 0);

        // Generic function (to be run on a separate thread) within which submitted tasks
        // are executed. It implements an endless loop (only returning when the ThreadPool
        // instance itself is destructed) which constantly checks the task queue, grabbing
        // and executing the frontmost task while the queue is not empty.
        auto thread_body = [this]{ 
            while (true) {
                // Get next task from queue (synchronized access via taskqueue_mutex).
                // If the queue is empty wait until either
                //   - being signaled about an available task.
                //   - the terminate flag is true (i.e. the ThreadPool instance is being destructed).
                //     In this case return from the running thread.
                std::unique_lock<std::mutex> queue_lock(m_taskqueue_mutex);
                while (m_taskqueue.empty()) {
                    if (m_terminate.load()) { return; }
                    m_task_available_cv.wait(queue_lock);
                }
                const auto current_task = m_taskqueue.front();
                m_taskqueue.pop();
                queue_lock.unlock();

                // Execute the actual task and signal the associated Task instance when finished.
                {
                    std::lock_guard<std::mutex> task_lock(current_task->m_task_mutex);
                    current_task->m_task_func();
                    current_task->m_is_finished = true;
                }
                current_task->m_finish_cv.notify_all();
            }
        };

        // Launch the specified number of threads
        for (int i = 0; i < num_threads; ++i) {
            m_threads.emplace_back(thread_body);
        }
    }

    ~ThreadPool() {
        // Indicate termination and signal potential waiting threads to wake up.
        // Then wait for all threads to finish their work and return properly.
        m_terminate = true;
        m_task_available_cv.notify_all();
        for (auto &t : m_threads) { t.join(); }
    }

    /// Submit new asynchronous task to thread pool and return Task handle to allow for synchronization.
    std::shared_ptr<Task> RunTask(const std::function<void()> &task_func) {
        // Wrap provided task callable object in task handle. Then append it to the task queue and
        // notify a waiting worker thread (if any) about the newly available task
        auto task = std::shared_ptr<Task>(new Task(task_func));
        {
            std::lock_guard<std::mutex> lock(m_taskqueue_mutex);
            m_taskqueue.push(task);
        }
        m_task_available_cv.notify_one();

        // Return task handle for later synchronization
        return task;
    }

    /// Run collection of tasks in parallel and wait until all have finished.
    void Parallelize(const std::vector<std::function<void()>> &task_funcs)
    {
        if (task_funcs.empty()) return;

        // Launch all provided tasks (except for the first) in parallel and store the associated handles
        auto it = begin(task_funcs);
        const auto first_task = it++;
        std::vector<std::shared_ptr<Task>> handles;
        for(; it != end(task_funcs); ++it)
        { 
            handles.push_back(RunTask(*it));
        }

        // Run the first task locally on the current thread
        (*first_task)();

        // Synchronize, i.e. wait for all parallelized tasks to complete
        for(const auto &h : handles) { h->join(); }
    }

    std::atomic_bool m_terminate{false};            //!< Indicates destruction of ThreadPool instance to worker threads
    std::vector<std::thread> m_threads;             //!< Collection of worker threads to run tasks
    std::queue<std::shared_ptr<Task>> m_taskqueue;  //!< Queue of submitted tasks pending for execution
    std::mutex m_taskqueue_mutex;                   //!< Protects task queue from concurrent access.
    std::condition_variable m_task_available_cv;    //!< Used to signal threads that a new task was submitted and is ready to run.
};

} // namespace RoR
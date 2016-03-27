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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 24th of August 2009

#include "BeamFactory.h"

#include "Application.h"
#include "BeamEngine.h"
#include "BeamStats.h"
#include "CacheSystem.h"
#include "Collisions.h"
#include "ErrorUtils.h"
#include "InputEngine.h"
#include "Language.h"
#include "MainThread.h"
#include "Network.h"
#include "PointColDetector.h"
#include "Settings.h"
#include "SoundScriptManager.h"
#include "ThreadPool.h"
#include "ChatSystem.h"
#include "Console.h"
#include "GUIManager.h"
#include "RigLoadingProfiler.h"
#include "RigLoadingProfilerControl.h"

#ifdef _GNU_SOURCE
#include <sys/sysinfo.h>
#endif

#ifdef USE_MYGUI
#include "GUIMp.h"
#include "GUIMenu.h"
#include "DashBoardManager.h"
#endif // USE_MYGUI

#ifdef USE_CRASHRPT
# include "crashrpt.h"
#endif

using namespace Ogre;
using namespace RoR;

template<> BeamFactory *StreamableFactory < BeamFactory, Beam >::_instance = 0;

int simulatedTruck;
void* threadstart(void* vid);

void cpuID(unsigned i, unsigned regs[4]) {
#ifdef _WIN32
	__cpuid((int *)regs, (int)i);
#elif defined(__x86_64__) || defined(__i386)
	asm volatile
		("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
		 : "a" (i), "c" (0));
#endif
}

static unsigned hardware_concurrency()
{
	#if defined(PTW32_VERSION) || defined(__hpux)
		return pthread_num_processors_np();
	#elif defined(_GNU_SOURCE)
		return get_nprocs();
	#elif defined(__APPLE__) || defined(__FreeBSD__)
		int count;
		size_t size = sizeof(count);
		return sysctlbyname("hw.ncpu", &count, &size, NULL, 0) ? 0 : count;
	#elif defined(BOOST_HAS_UNISTD_H) && defined(_SC_NPROCESSORS_ONLN)
		int const count = sysconf(_SC_NPROCESSORS_ONLN);
		return (count > 0) ? count : 0;
	#else
		return 0;
	#endif
} 

unsigned int getNumberOfCPUCores()
{
#if defined(_WIN32) || defined(__x86_64__) || defined(__i386)
	unsigned regs[4];

	// Get CPU vendor
	char vendor[12];
	cpuID(0, regs);
	((unsigned *)vendor)[0] = regs[1]; // EBX
	((unsigned *)vendor)[1] = regs[3]; // EDX
	((unsigned *)vendor)[2] = regs[2]; // ECX
	std::string cpuVendor = std::string(vendor, 12);

	// Get CPU features
	cpuID(1, regs);
	unsigned cpuFeatures = regs[3]; // EDX

	// Logical core count per CPU
	cpuID(1, regs);
	unsigned logical = (regs[1] >> 16) & 0xff; // EBX[23:16]
	unsigned cores = logical;

	if (cpuVendor == "GenuineIntel")
	{
		// Get DCP cache info
		cpuID(4, regs);
		cores = ((regs[0] >> 26) & 0x3f) + 1; // EAX[31:26] + 1
	} else if (cpuVendor == "AuthenticAMD")
	{
		// Get NC: Number of CPU cores - 1
		cpuID(0x80000008, regs);
		cores = ((unsigned)(regs[2] & 0xff)) + 1; // ECX[7:0] + 1
	}

	// Detect hyper-threads  
	bool hyperThreads = cpuFeatures & (1 << 28) && cores < logical;

	LOG("BEAMFACTORY: " + TOSTRING(logical) + " Logical CPUs" + " found");
	LOG("BEAMFACTORY: " + TOSTRING(cores) + " CPU Cores" + " found");
	LOG("BEAMFACTORY: Hyper-Threading " + TOSTRING(hyperThreads));
#else
	unsigned cores = hardware_concurrency();
	LOG("BEAMFACTORY: " + TOSTRING(cores) + " CPU Cores" + " found");
#endif
	return cores;
}

BeamFactory::BeamFactory() :
	  current_truck(-1)
	, forced_active(false)
	, free_truck(0)
	, m_dt_remainder(0.0f)
	, m_physics_frames(0)
	, m_physics_steps(2000)
	, m_simulation_speed(1.0f)
	, num_cpu_cores(0)
	, previous_truck(-1)
	, task_count(0)
	, tdr(0)
	, thread_done(true)
	, thread_mode(THREAD_SINGLE)
	, work_done(false)
{
	bool disableThreadPool = BSETTING("DisableThreadPool", false);

	for (int t=0; t < MAX_TRUCKS; t++)
		trucks[t] = 0;

	if (BSETTING("Multi-threading", true))
		thread_mode = THREAD_MULTI;

	if (BSETTING("2DReplay", false))
		tdr = new TwoDReplay();

	async_physics = BSETTING("AsynchronousPhysics", false);

	// Create worker thread (used for physics calculations)
	if (thread_mode == THREAD_MULTI)
	{
		int numThreadsInPool = ISETTING("NumThreadsInThreadPool", 0);

		if (numThreadsInPool > 1)
		{
			num_cpu_cores = numThreadsInPool;
		} else 
		{
			num_cpu_cores = getNumberOfCPUCores();
		}

		if (num_cpu_cores < 2)
		{
			disableThreadPool = true;
			LOG("BEAMFACTORY: Not enough CPU cores to enable the thread pool");
		} else if (!disableThreadPool)
		{
			gEnv->threadPool = new ThreadPool(num_cpu_cores);
			LOG("BEAMFACTORY: Creating " + TOSTRING(num_cpu_cores) + " threads");
		}

		pthread_cond_init(&thread_done_cv, NULL);
		pthread_cond_init(&work_done_cv, NULL);
		pthread_mutex_init(&thread_done_mutex, NULL);
		pthread_mutex_init(&work_done_mutex, NULL);
		pthread_cond_init(&task_count_cv, NULL);
		pthread_mutex_init(&task_count_mutex, NULL);

		if (pthread_create(&worker_thread, NULL, threadstart, this))
		{
			LOG("BEAMFACTORY: Can not start a thread");
			ErrorUtils::ShowError(UTFString("Error"), _L("Failed to start a thread."));
			exit(1);
		}
	}
}

BeamFactory::~BeamFactory()
{
	pthread_cond_destroy(&thread_done_cv);
	pthread_cond_destroy(&work_done_cv);
	pthread_mutex_destroy(&thread_done_mutex);
	pthread_mutex_destroy(&work_done_mutex);
	pthread_cond_destroy(&task_count_cv);
	pthread_mutex_destroy(&task_count_mutex);
}

bool BeamFactory::removeBeam(Beam *b)
{
	lockStreams();
	std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
	std::map < int, std::map < unsigned int, Beam *> >::iterator it1;
	std::map < unsigned int, Beam *>::iterator it2;

	for (it1=streamables.begin(); it1!=streamables.end(); it1++)
	{
		for (it2=it1->second.begin(); it2!=it1->second.end(); it2++)
		{
			if (it2->second == b)
			{
				NetworkStreamManager::getSingleton().removeStream(it1->first, it2->first);
				_deleteTruck(it2->second);
				it1->second.erase(it2);
				unlockStreams();
#ifdef USE_MYGUI
				GUI_MainMenu::getSingleton().triggerUpdateVehicleList();
#endif // USE_MYGUI
				return true;
			}
		}
	}
	unlockStreams();

	return false;
}

#define LOADRIG_PROFILER_CHECKPOINT(ENTRY_ID) rig_loading_profiler.Checkpoint(RigLoadingProfiler::ENTRY_ID);

Beam *BeamFactory::CreateLocalRigInstance(
	Ogre::Vector3 pos, 
	Ogre::Quaternion rot, 
	Ogre::String fname, 
    int cache_entry_number, // = -1, 
	collision_box_t *spawnbox /* = nullptr */, 
	bool ismachine /* = false */, 
	const std::vector<Ogre::String> *truckconfig /* = nullptr */, 
	Skin *skin /* = nullptr */, 
	bool freePosition, /* = false */
	bool preloaded_with_terrain /* = false */
)
{
    RigLoadingProfiler rig_loading_profiler;
#ifdef ROR_PROFILE_RIG_LOADING
    ::Profiler::reset();
#endif

	int truck_num = getFreeTruckSlot();
	if (truck_num == -1)
	{
		LOG("ERROR: Could not add beam to main list");
		return 0;
	}

	Beam *b = new Beam(
		truck_num,
		pos,
		rot,
		fname.c_str(),
        &rig_loading_profiler,
		false, // networked
		gEnv->network != nullptr, // networking
		spawnbox,
		ismachine,
		truckconfig,
		skin,
		freePosition,
		preloaded_with_terrain,
        cache_entry_number
		);
	trucks[truck_num] = b;

	// lock slide nodes after spawning the truck?
	if (b->getSlideNodesLockInstant())
	{
		b->toggleSlideNodeLock();
	}

	lockStreams();
	std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
	streamables[-1][10 + truck_num] = b; // 10 streams offset for beam constructions
	unlockStreams();

#ifdef USE_MYGUI
	GUI_MainMenu::getSingleton().triggerUpdateVehicleList();
#endif // USE_MYGUI

	// add own username to truck
	if (gEnv->network)
	{
		b->updateNetworkInfo();
	}
    LOADRIG_PROFILER_CHECKPOINT(ENTRY_BEAMFACTORY_CREATELOCAL_POSTPROCESS);

    LOG(rig_loading_profiler.Report());
#ifdef ROR_PROFILE_RIG_LOADING
    std::string out_path = SSETTING("Profiler output dir", "") + ROR_PROFILE_RIG_LOADING_OUTFILE;
    ::Profiler::DumpHtml(out_path.c_str());
#endif
	return b;
}

#undef LOADRIG_PROFILER_CHECKPOINT

Beam *BeamFactory::createRemoteInstance(stream_reg_t *reg)
{
	// NO LOCKS IN HERE, already locked

	stream_register_trucks_t *treg = (stream_register_trucks_t *)&reg->reg;

	LOG(" new beam truck for " + TOSTRING(reg->sourceid) + ":" + TOSTRING(reg->streamid));

#ifdef USE_SOCKETW
	// log a message about this
	if (gEnv->network)
	{
		client_t *c = gEnv->network->getClientInfo(reg->sourceid);
		if (c)
		{
			UTFString username = ChatSystem::getColouredName(*c);
			UTFString message = username + ChatSystem::commandColour + _L(" spawned a new vehicle: ") + ChatSystem::normalColour + treg->name;
#ifdef USE_MYGUI
			/*Console *console = RoR::Application::GetConsole();
			if (console) console->putMessage(Console::CONSOLE_MSGTYPE_NETWORK, Console::CONSOLE_LOGMESSAGE, message, "car_add.png");
			RoR::Application::GetGuiManager()->PushNotification("Notice:", message);*/
			RoR::Application::GetGuiManager()->pushMessageChatBox(message);
#endif // USE_MYGUI
		}
	}
#endif // USE_SOCKETW

	// check if we got this truck installed
	String filename = String(treg->name);
	String group = "";
	if (!RoR::Application::GetCacheSystem()->checkResourceLoaded(filename, group))
	{
		LOG("wont add remote stream (truck not existing): '"+filename+"'");

		// add 0 to the map so we know its stream is existing but not usable for us
		// already locked
		//lockStreams();
		std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
		streamables[reg->sourceid][reg->streamid] = 0;
		//unlockStreams();

		return 0;
	}

	// fill truckconfig
	std::vector<String> truckconfig;
	for (int t=0; t<10; t++)
	{
		if (!strnlen(treg->truckconfig[t], 60))
			break;
		truckconfig.push_back(String(treg->truckconfig[t]));
	}


	// DO NOT spawn the truck far off anywhere
	// the truck parsing will break flexbodies initialization when using huge numbers here
	Vector3 pos = Vector3::ZERO;

	int truck_num = getFreeTruckSlot();
	if (truck_num == -1)
	{
		LOG("ERROR: could not add beam to main list");
		return 0;
	}
    RigLoadingProfiler p; // TODO: Placeholder. Use it
	Beam *b = new Beam(
		truck_num,
		pos,
		Quaternion::ZERO,
		reg->reg.name,
        &p,
		true, // networked
		gEnv->network != nullptr, // networking
		nullptr, // spawnbox
		false, // ismachine
		&truckconfig,
		nullptr // skin
		);

	trucks[truck_num] = b;

	b->setSourceID(reg->sourceid);
	b->setStreamID(reg->streamid);

	// already locked
	//lockStreams();
	std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
	streamables[reg->sourceid][reg->streamid] = b;
	//unlockStreams();

	b->updateNetworkInfo();

#ifdef USE_MYGUI
	GUI_MainMenu::getSingleton().triggerUpdateVehicleList();
#endif // USE_MYGUI

	return b;
}

void BeamFactory::localUserAttributesChanged(int new_id)
{
	lockStreams();
	std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();

	if (streamables.find(-1) != streamables.end())
	{
		//Beam *b = streamables[-1][0];
		streamables[new_id][0] = streamables[-1][0]; // add alias :)
		//b->setUID(newid);
		//b->updateNetLabel();
	}
	unlockStreams();
}

void BeamFactory::netUserAttributesChanged(int source_id, int stream_id)
{
	lockStreams();
	std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
	std::map < int, std::map < unsigned int, Beam *> >::iterator it_source = streamables.find(source_id);
	std::map < unsigned int, Beam *>::iterator it_stream;

	if (it_source != streamables.end() && !it_source->second.empty())
	{
		it_stream = it_source->second.find(stream_id);
		if (it_stream != it_source->second.end() && it_stream->second)
			it_stream->second->updateNetworkInfo();
	}
	unlockStreams();
}

Beam *BeamFactory::getBeam(int source_id, int stream_id)
{
	lockStreams();
	Beam *retVal = 0;
	std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
	std::map < int, std::map < unsigned int, Beam *> >::iterator it_source = streamables.find(source_id);
	std::map < unsigned int, Beam *>::iterator it_stream;

	if (it_source != streamables.end() && !it_source->second.empty())
	{
		it_stream = it_source->second.find(stream_id);
		if (it_stream != it_source->second.end() && it_stream->second)
			retVal = it_stream->second;
	}
	unlockStreams();
	return retVal;
}

bool BeamFactory::syncRemoteStreams()
{
	// we override this here, so we know if something changed and could update the player list
	// we delete and add trucks in there, so be sure that nothing runs as we delete them ...
	bool changes = StreamableFactory <BeamFactory, Beam>::syncRemoteStreams();

	if (changes)
		updateGUI();

	return changes;
}

void BeamFactory::updateGUI()
{
#ifdef USE_MYGUI
#ifdef USE_SOCKETW
	GUI_Multiplayer::getSingleton().update();
#endif // USE_SOCKETW
#endif // USE_MYGUI	
}

bool BeamFactory::truckIntersectionAABB(int a, int b)
{
	return trucks[a]->boundingBox.intersects(trucks[b]->boundingBox);
}

bool BeamFactory::predictTruckIntersectionAABB(int a, int b)
{
	return trucks[a]->predictedBoundingBox.intersects(trucks[b]->predictedBoundingBox);
}

bool BeamFactory::truckIntersectionCollAABB(int a, int b)
{
	if (trucks[a]->collisionBoundingBoxes.empty() && trucks[b]->collisionBoundingBoxes.empty())
	{
		return truckIntersectionAABB(a, b);
	} else if (trucks[a]->collisionBoundingBoxes.empty())
	{
		for (std::vector<AxisAlignedBox>::iterator it = trucks[b]->collisionBoundingBoxes.begin(); it != trucks[b]->collisionBoundingBoxes.end(); ++it)
			if (it->intersects(trucks[a]->boundingBox))
				return true;
	} else if (trucks[b]->collisionBoundingBoxes.empty())
	{
		for (std::vector<AxisAlignedBox>::iterator it = trucks[a]->collisionBoundingBoxes.begin(); it != trucks[a]->collisionBoundingBoxes.end(); ++it)
			if (it->intersects(trucks[b]->boundingBox))
				return true;
	} else
	{
		for (std::vector<AxisAlignedBox>::iterator it_a = trucks[a]->collisionBoundingBoxes.begin(); it_a != trucks[a]->collisionBoundingBoxes.end(); ++it_a)
			for (std::vector<AxisAlignedBox>::iterator it_b = trucks[b]->collisionBoundingBoxes.begin(); it_b != trucks[b]->collisionBoundingBoxes.end(); ++it_b)
				if (it_a->intersects(*it_b))
					return true;
	}

	return false;
}

bool BeamFactory::predictTruckIntersectionCollAABB(int a, int b)
{
	if (trucks[a]->predictedCollisionBoundingBoxes.empty() && trucks[b]->predictedCollisionBoundingBoxes.empty())
	{
		return predictTruckIntersectionAABB(a, b);
	} else if (trucks[a]->predictedCollisionBoundingBoxes.empty())
	{
		for (std::vector<AxisAlignedBox>::iterator it = trucks[b]->predictedCollisionBoundingBoxes.begin(); it != trucks[b]->predictedCollisionBoundingBoxes.end(); ++it)
			if (it->intersects(trucks[a]->predictedBoundingBox))
				return true;
	} else if (trucks[b]->predictedCollisionBoundingBoxes.empty())
	{
		for (std::vector<AxisAlignedBox>::iterator it = trucks[a]->predictedCollisionBoundingBoxes.begin(); it != trucks[a]->predictedCollisionBoundingBoxes.end(); ++it)
			if (it->intersects(trucks[b]->predictedBoundingBox))
				return true;
	} else
	{
		for (std::vector<AxisAlignedBox>::iterator it_a = trucks[a]->predictedCollisionBoundingBoxes.begin(); it_a != trucks[a]->predictedCollisionBoundingBoxes.end(); ++it_a)
			for (std::vector<AxisAlignedBox>::iterator it_b = trucks[b]->predictedCollisionBoundingBoxes.begin(); it_b != trucks[b]->predictedCollisionBoundingBoxes.end(); ++it_b)
				if (it_a->intersects(*it_b))
					return true;
	}

	return false;
}

// j is the index of a MAYSLEEP truck, returns true if one active was found in the set
bool BeamFactory::checkForActive(int j, std::bitset<MAX_TRUCKS> &sleepy)
{
	sleepy.set(j, true);
	for (int t=0; t < free_truck; t++)
	{
		if (trucks[t] && !sleepy[t] && predictTruckIntersectionCollAABB(t, j))
		{
			if (trucks[t]->state == SLEEPING || trucks[t]->state == MAYSLEEP || trucks[t]->state == GOSLEEP || (trucks[t]->state == DESACTIVATED && trucks[t]->sleepcount >= 5))
				return checkForActive(t, sleepy);
			else
				return true;
		}
	}
	return false;
}

void BeamFactory::recursiveActivation(int j)
{
	if (!trucks[j] || trucks[j]->state > DESACTIVATED) return;

	for (int t=0; t < free_truck; t++)
	{
		if (t == j || !trucks[t]) continue;
		if ((trucks[t]->state == SLEEPING || trucks[t]->state == MAYSLEEP || trucks[t]->state == GOSLEEP || (trucks[t]->state == DESACTIVATED && trucks[t]->sleepcount >= 5)) &&
			predictTruckIntersectionCollAABB(t, j))
		{
			trucks[t]->desactivate(); // make the truck not leading but active

			if (getTruck(simulatedTruck))
			{
				trucks[t]->disableDrag = getTruck(simulatedTruck)->driveable==AIRPLANE;
			}

			recursiveActivation(t);
		}
	}
}

void BeamFactory::checkSleepingState()
{
	if (getTruck(simulatedTruck))
	{
		getTruck(simulatedTruck)->disableDrag = false;
	}

	for (int t=0; t < free_truck; t++)
	{
		if (trucks[t] && trucks[t]->state <= DESACTIVATED && (t == simulatedTruck || trucks[t]->sleepcount <= 7))
		{
			recursiveActivation(t);
		}
	}

	if (!forced_active)
	{
		// put to sleep
		for (int t=0; t < free_truck; t++)
		{
			if (trucks[t] && trucks[t]->state == MAYSLEEP)
			{
				std::bitset<MAX_TRUCKS> sleepy;
				if (!checkForActive(t, sleepy))
				{
					// no active truck in the set, put everybody to sleep
					for (int i=0; i < free_truck; i++)
					{
						if (trucks[i] && sleepy[i])
						{
							trucks[i]->state = GOSLEEP;
						}
					}
				}
			}
		}
	}

#if 0   // obsolete for now
	// special stuff for rollable gear
	bool rollmode = false;
	for (int t=0; t < free_truck; t++)
	{
		if (!trucks[t]) continue;
		if (trucks[t]->state != SLEEPING)
			rollmode = rollmode || trucks[t]->wheel_contact_requested;

		trucks[t]->requires_wheel_contact = rollmode;// && !trucks[t]->wheel_contact_requested;
	}
#endif
}

int BeamFactory::getFreeTruckSlot()
{
	// find a free slot for the truck
	for (int t=0; t<MAX_TRUCKS; t++)
	{
		if (!trucks[t] && t >= free_truck) // XXX: TODO: remove this hack
		{
			// reuse slots
			if (t >= free_truck)
				free_truck = t + 1;
			return t;
		}
	}
	return -1;
}

void BeamFactory::activateAllTrucks()
{
	for (int t=0; t < free_truck; t++)
	{
		if (trucks[t] && trucks[t]->state >= DESACTIVATED && trucks[t]->state <= SLEEPING)
		{
			trucks[t]->desactivate(); // make the truck not leading but active

			if (getTruck(simulatedTruck))
			{
				trucks[t]->disableDrag = getTruck(simulatedTruck)->driveable==AIRPLANE;
			}
		}
	}
}

void BeamFactory::sendAllTrucksSleeping()
{
	forced_active = false;
	for (int t=0; t < free_truck; t++)
	{
		if (trucks[t] && trucks[t]->state < SLEEPING)
		{
			trucks[t]->state = SLEEPING;
		}
	}
}

void BeamFactory::recalcGravityMasses()
{
	// update the mass of all trucks
	for (int t=0; t < free_truck; t++)
	{
		if (trucks[t])
		{
			trucks[t]->recalc_masses();
		}
	}
}

int BeamFactory::findTruckInsideBox(Collisions *collisions, const Ogre::String &inst, const Ogre::String &box)
{
	// try to find the desired truck (the one in the box)
	int id = -1;
	for (int t=0; t < free_truck; t++)
	{
		if (!trucks[t]) continue;
		if (collisions->isInside(trucks[t]->nodes[0].AbsPosition, inst, box))
		{
			if (id == -1)
				// first truck found
				id = t;
			else
				// second truck found -> unclear which vehicle was meant
				return -1;
		}
	}
	return id;
}

void BeamFactory::repairTruck(Collisions *collisions, const Ogre::String &inst, const Ogre::String &box, bool keepPosition)
{
	int rtruck = findTruckInsideBox(collisions, inst, box);
	if (rtruck >= 0)
	{
		// take a position reference
#ifdef USE_OPENAL
		SoundScriptManager::getSingleton().trigOnce(rtruck, SS_TRIG_REPAIR);
#endif // USE_OPENAL
		Vector3 ipos=trucks[rtruck]->nodes[0].AbsPosition;
		trucks[rtruck]->reset();
		trucks[rtruck]->resetPosition(ipos.x, ipos.z, false, 0);
		trucks[rtruck]->updateVisual();
	}
}

void BeamFactory::MuteAllTrucks()
{
	for (int i = 0; i < free_truck; i++)
	{
		if (trucks[i])
		{
			trucks[i]->StopAllSounds();
		}
	}
}

void BeamFactory::UnmuteAllTrucks()
{
	for (int i = 0; i < free_truck; i++)
	{
		if (trucks[i])
		{
			trucks[i]->UnmuteAllSounds();
		}
	}
}

void BeamFactory::removeTruck(Collisions *collisions, const Ogre::String &inst, const Ogre::String &box)
{
	removeTruck(findTruckInsideBox(collisions, inst, box));
}

void BeamFactory::removeTruck(int truck)
{
	if (truck < 0 || truck > free_truck)
		return;

	if (current_truck == truck)
		setCurrentTruck(-1);

	if (!removeBeam(trucks[truck]))
		// deletion over beamfactory failed, delete by hand
		// then delete the class
		_deleteTruck(trucks[truck]);
}

void BeamFactory::p_removeAllTrucks()
{
	for (int i = 0; i < free_truck; i++)
	{
		if (trucks[i])
		{
			trucks[i]->StopAllSounds();

			if (current_truck == i)
				setCurrentTruck(-1);

			if (!removeBeam(trucks[i]))
				// deletion over beamfactory failed, delete by hand
				// then delete the class
				_deleteTruck(trucks[i]);
		}
	}
}

void BeamFactory::_deleteTruck(Beam *b)
{
	if (b == 0)	return;

	_WorkerWaitForSync();

	trucks[b->trucknum] = 0;
	delete b;
	//free_truck = free_truck - 1;

#ifdef USE_MYGUI
	GUI_MainMenu::getSingleton().triggerUpdateVehicleList();
#endif // USE_MYGUI
}

void BeamFactory::removeCurrentTruck()
{
	removeTruck(current_truck);
}

void BeamFactory::removeAllTrucks()
{
	p_removeAllTrucks();
}

void BeamFactory::setCurrentTruck(int new_truck)
{
	if (current_truck >= 0 && current_truck < free_truck && trucks[current_truck])
	{
		trucks[current_truck]->desactivate();
	}

	previous_truck = current_truck;
	current_truck = new_truck;

	{
		if (previous_truck >= 0 && current_truck >= 0)
		{
			RoR::MainThread::ChangedCurrentVehicle(trucks[previous_truck], trucks[current_truck]);
		}
		else if (previous_truck >= 0)
		{
			RoR::MainThread::ChangedCurrentVehicle(trucks[previous_truck], nullptr);
		}
		else if (current_truck >= 0)
		{
			RoR::MainThread::ChangedCurrentVehicle(nullptr, trucks[current_truck]);
		}
		else
		{
			RoR::MainThread::ChangedCurrentVehicle(nullptr, nullptr);
		}
	}
}

bool BeamFactory::enterRescueTruck()
{
	// rescue!
	// search a rescue truck
	for (int t=0; t < free_truck; t++)
	{
		if (trucks[t] && trucks[t]->rescuer) {
			// go to person mode first
			setCurrentTruck(-1);
			// then to the rescue truck, this fixes overlapping interfaces
			setCurrentTruck(t);
			return true;
		}
	}
	return false;
}

void BeamFactory::updateFlexbodiesPrepare()
{
	for (int t=0; t < free_truck; t++)
	{
		if (trucks[t] && trucks[t]->state != SLEEPING && trucks[t]->loading_finished)
		{
			trucks[t]->updateFlexbodiesPrepare();
		}
	}
}

void BeamFactory::updateFlexbodiesFinal()
{
	for (int t=0; t < free_truck; t++)
	{
		if (trucks[t] && trucks[t]->state != SLEEPING && trucks[t]->loading_finished)
		{
			trucks[t]->updateFlexbodiesFinal();
		}
	}
}

void BeamFactory::updateVisual(float dt)
{
	dt *= m_simulation_speed;

	for (int t=0; t < free_truck; t++)
	{
		if (!trucks[t]) continue;

		// always update the labels
		trucks[t]->updateLabels(dt);

		if (trucks[t]->state != SLEEPING && trucks[t]->loading_finished)
		{
			trucks[t]->updateVisual(dt);
			trucks[t]->updateSkidmarks();
			trucks[t]->updateFlares(dt, (t==current_truck));
		}
	}
}

void BeamFactory::calcPhysics(float dt)
{
	m_physics_frames++;

	// do not allow dt > 1/20
	dt = std::min(dt, 1.0f / 20.0f);

	dt *= m_simulation_speed;

	dt += m_dt_remainder;
	m_physics_steps = dt / PHYSICS_DT;
	m_dt_remainder = dt - (m_physics_steps * PHYSICS_DT);
	dt = PHYSICS_DT * m_physics_steps;

	gEnv->mrTime += dt;

	simulatedTruck = current_truck;

	if (simulatedTruck == -1)
	{
		for (int t=0; t < free_truck; t++)
		{
			if (!trucks[t]) continue;

			if (trucks[t]->state <= DESACTIVATED)
			{
				simulatedTruck = t;
				break;
			}
		}
	}

	if (simulatedTruck >= 0 && simulatedTruck < free_truck)
	{
		trucks[simulatedTruck]->frameStep(m_physics_steps);
	}

	// update 2D replay if activated
	if (tdr) tdr->update(dt);

	// things always on
	for (int t=0; t < free_truck; t++)
	{
		if (!trucks[t]) continue;

		// networked trucks must be taken care of
		switch(trucks[t]->state)
		{
		case NETWORKED:
			trucks[t]->calcNetwork();
			break;

		case RECYCLE:
			break;

		case NETWORKED_INVALID:
			break;

		default:
			if (trucks[t]->state > DESACTIVATED && trucks[t]->engine)
				trucks[t]->engine->update(dt, 1);
			if (trucks[t]->networking)
				trucks[t]->sendStreamData();
			break;
		}
	}
}

void BeamFactory::removeInstance(Beam *b)
{
	if (b == 0) return;
	// hide the truck
	b->deleteNetTruck();
	//_deleteTruck(b);
}

void BeamFactory::removeInstance(stream_del_t *del)
{
	// we override this here so we can also delete the truck array content
	// already locked
	// lockStreams();
	std::map < int, std::map < unsigned int, Beam *> > &streamables = getStreams();
	std::map < int, std::map < unsigned int, Beam *> >::iterator it_stream = streamables.find(del->sourceid);;
	std::map < unsigned int, Beam *>::iterator it_beam;

	if (it_stream == streamables.end() || it_stream->second.empty())
		// no stream for this source id
		return;

	if (del->streamid == -1)
	{
		// delete all streams
		for (it_beam=it_stream->second.begin(); it_beam != it_stream->second.end(); it_beam++)
			removeInstance(it_beam->second);
	} else
	{
		// find the stream matching the streamid
		it_beam = it_stream->second.find(del->streamid);
		if (it_beam != it_stream->second.end())
			removeInstance(it_beam->second);
	}
	// unlockStreams();
}

void BeamFactory::windowResized()
{
#ifdef USE_MYGUI
	for (int t=0; t < free_truck; t++)
	{
		if (trucks[t])
		{
			trucks[t]->dash->windowResized();
		}
	}
#endif // USE_MYGUI
}

void BeamFactory::_WorkerWaitForSync()
{
	if (thread_mode == THREAD_MULTI)
	{
		MUTEX_LOCK(&thread_done_mutex);
		while (!thread_done)
		{
			pthread_cond_wait(&thread_done_cv, &thread_done_mutex);
		}
		MUTEX_UNLOCK(&thread_done_mutex);
	}
}

void BeamFactory::_WorkerPrepareStart()
{
	if (thread_mode == THREAD_MULTI)
	{
		MUTEX_LOCK(&thread_done_mutex);
		thread_done = false;
		MUTEX_UNLOCK(&thread_done_mutex);
	}
}

void BeamFactory::_WorkerSignalStart()
{
	if (thread_mode == THREAD_MULTI)
	{
		MUTEX_LOCK(&work_done_mutex);
		work_done = true;
		MUTEX_UNLOCK(&work_done_mutex);
		pthread_cond_signal(&work_done_cv);
	}
}

void BeamFactory::prepareShutdown()
{
	_WorkerWaitForSync();
}

Beam* BeamFactory::getCurrentTruck()
{
	return getTruck(current_truck);
}

Beam* BeamFactory::getTruck(int number)
{
	if (number >= 0 && number < free_truck)
	{
		return trucks[number];
	}
	return 0;
}

void BeamFactory::onTaskComplete()
{
	MUTEX_LOCK(&task_count_mutex);
	task_count--;
	MUTEX_UNLOCK(&task_count_mutex);
	if (!task_count)
	{
		pthread_cond_signal(&task_count_cv);
	}
}

void BeamFactory::runThreadTask(Beam::ThreadTask task)
{
	if (gEnv->threadPool)
	{
		std::list<IThreadTask*> tasks;

		// Push tasks into thread pool
		for (int t=0; t<free_truck; t++)
		{
			if (trucks[t] && trucks[t]->simulated)
			{
				trucks[t]->thread_task = task;
				tasks.emplace_back(trucks[t]);
			}
		}

		task_count = tasks.size();

		gEnv->threadPool->enqueue(tasks);

		// Wait for all tasks to complete
		MUTEX_LOCK(&task_count_mutex);
		while (task_count > 0)
		{
			pthread_cond_wait(&task_count_cv, &task_count_mutex);
		}
		MUTEX_UNLOCK(&task_count_mutex);
	} else
	{
		for (int t=0; t<free_truck; t++)
		{
			if (trucks[t] && trucks[t]->simulated)
			{
				trucks[t]->thread_task = task;
				trucks[t]->run();
			}
		}
	}
}

void BeamFactory::threadentry()
{
	for (int i=0; i<m_physics_steps; i++)
	{
		int num_simulated_trucks = 0;

		for (int t=0; t<free_truck; t++)
		{
			if (trucks[t] && (trucks[t]->simulated = trucks[t]->calcForcesEulerPrepare(i==0, PHYSICS_DT, i, m_physics_steps)))
			{
				num_simulated_trucks++;
				trucks[t]->curtstep = i;
				trucks[t]->tsteps   = m_physics_steps;
			}
		}
		if (num_simulated_trucks < 2 || !gEnv->threadPool)
		{
			for (int t=0; t<free_truck; t++)
			{
				if (trucks[t] && trucks[t]->simulated)
				{
					trucks[t]->calcForcesEulerCompute(i==0, PHYSICS_DT, i, m_physics_steps);
					if (!trucks[t]->disableTruckTruckSelfCollisions)
					{
						trucks[t]->IntraPointCD()->update(trucks[t]);
						intraTruckCollisions(PHYSICS_DT,
								*(trucks[t]->IntraPointCD()),
								trucks[t]->free_collcab,
								trucks[t]->collcabs,
								trucks[t]->cabs,
								trucks[t]->intra_collcabrate,
								trucks[t]->nodes,
								trucks[t]->collrange,
								*(trucks[t]->submesh_ground_model));
					}
				}
			}
		} else
		{
			runThreadTask(Beam::THREAD_BEAMFORCESEULER);
		}
		for (int t=0; t<free_truck; t++)
		{
			if (trucks[t] && trucks[t]->simulated)
				trucks[t]->calcForcesEulerFinal(i==0, PHYSICS_DT, i, m_physics_steps);
		}

		if (num_simulated_trucks > 1)
		{
			BES_START(BES_CORE_Contacters);
			runThreadTask(Beam::THREAD_INTER_TRUCK_COLLISIONS);
			BES_STOP(BES_CORE_Contacters);
		}
	}
}

void* threadstart(void* vid)
{
#ifdef USE_CRASHRPT
	if (SSETTING("NoCrashRpt").empty())
	{
		// add the crash handler for this thread
		CrThreadAutoInstallHelper cr_thread_install_helper;
		MYASSERT(cr_thread_install_helper.m_nInstallStatus==0);
	}
#endif // USE_CRASHRPT

	BeamFactory *bf = static_cast<BeamFactory*>(vid);

	while (1)
	{
		MUTEX_LOCK(&bf->work_done_mutex);

		MUTEX_LOCK(&bf->thread_done_mutex);
		bf->thread_done = true;
		MUTEX_UNLOCK(&bf->thread_done_mutex);
		pthread_cond_signal(&bf->thread_done_cv);

		while (!bf->work_done)
		{
			pthread_cond_wait(&bf->work_done_cv, &bf->work_done_mutex);
		}
		bf->work_done = false;
		MUTEX_UNLOCK(&bf->work_done_mutex);

		if (bf->getTruck(simulatedTruck))
		{
			bf->threadentry();
		}
	}

	pthread_exit(NULL);
	return NULL;
}

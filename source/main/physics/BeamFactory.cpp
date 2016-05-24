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
#include "ChatSystem.h"
#include "Collisions.h"
#include "DynamicCollisions.h"
#include "GUIManager.h"
#include "Language.h"
#include "MainThread.h"
#include "Network.h"
#include "PointColDetector.h"
#include "RigLoadingProfiler.h"
#include "RigLoadingProfilerControl.h"
#include "Settings.h"
#include "SoundScriptManager.h"
#include "ThreadPool.h"

#ifdef _GNU_SOURCE
#include <sys/sysinfo.h>
#endif

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#ifdef USE_MYGUI
#include "GUIMenu.h"
#include "DashBoardManager.h"
#endif // USE_MYGUI

using namespace Ogre;

template<> BeamFactory *StreamableFactory < BeamFactory, Beam >::_instance = 0;

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
	#if defined(_GNU_SOURCE)
		return get_nprocs();
	#elif defined(__APPLE__) || defined(__FreeBSD__)
		int count;
		size_t size = sizeof(count);
		return sysctlbyname("hw.ncpu", &count, &size, NULL, 0) ? 0 : count;
	#elif defined(BOOST_HAS_UNISTD_H) && defined(_SC_NPROCESSORS_ONLN)
		int const count = sysconf(_SC_NPROCESSORS_ONLN);
		return (count > 0) ? count : 0;
	#else
		return std::thread::hardware_concurrency();
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
	  m_current_truck(-1)
	, m_dt_remainder(0.0f)
	, m_forced_active(false)
	, m_free_truck(0)
	, m_num_cpu_cores(0)
	, m_physics_frames(0)
	, m_physics_steps(2000)
	, m_previous_truck(-1)
	, m_simulation_speed(1.0f)
{
	bool disableThreadPool = BSETTING("DisableThreadPool", false);

	for (int t=0; t < MAX_TRUCKS; t++)
		m_trucks[t] = 0;

	if (BSETTING("Multi-threading", true))
	{
		// Create thread pool
		int numThreadsInPool = ISETTING("NumThreadsInThreadPool", 0);

		if (numThreadsInPool > 1)
		{
			m_num_cpu_cores = numThreadsInPool;
		} else 
		{
			int logical_cpus = hardware_concurrency();
			int physical_cpus = getNumberOfCPUCores();
			m_num_cpu_cores = std::max(physical_cpus, logical_cpus - 1);
		}

		if (m_num_cpu_cores < 2)
		{
			disableThreadPool = true;
			LOG("BEAMFACTORY: Not enough CPU cores to enable the thread pool");
		} else if (!disableThreadPool)
		{
			gEnv->threadPool = new ThreadPool(m_num_cpu_cores);
			LOG("BEAMFACTORY: Creating " + TOSTRING(m_num_cpu_cores) + " threads");
		}

		// Create worker thread (used for physics calculations)
		m_sim_thread_pool = std::unique_ptr<ThreadPool>(new ThreadPool(1));
	}
}

BeamFactory::~BeamFactory()
{
	delete gEnv->threadPool;
}

#define LOADRIG_PROFILER_CHECKPOINT(ENTRY_ID) rig_loading_profiler.Checkpoint(RoR::RigLoadingProfiler::ENTRY_ID);

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
	RoR::RigLoadingProfiler rig_loading_profiler;
#ifdef ROR_PROFILE_RIG_LOADING
    ::Profiler::reset();
#endif

	int truck_num = this->GetFreeTruckSlot();
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
	m_trucks[truck_num] = b;

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

	int truck_num = this->GetFreeTruckSlot();
	if (truck_num == -1)
	{
		LOG("ERROR: could not add beam to main list");
		return 0;
	}
	RoR::RigLoadingProfiler p; // TODO: Placeholder. Use it
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

	m_trucks[truck_num] = b;

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

bool BeamFactory::truckIntersectionAABB(int a, int b)
{
	return m_trucks[a]->boundingBox.intersects(m_trucks[b]->boundingBox);
}

bool BeamFactory::predictTruckIntersectionAABB(int a, int b)
{
	return m_trucks[a]->predictedBoundingBox.intersects(m_trucks[b]->predictedBoundingBox);
}

bool BeamFactory::truckIntersectionCollAABB(int a, int b)
{
	if (m_trucks[a]->collisionBoundingBoxes.empty() && m_trucks[b]->collisionBoundingBoxes.empty())
	{
		return truckIntersectionAABB(a, b);
	} else if (m_trucks[a]->collisionBoundingBoxes.empty())
	{
		for (std::vector<AxisAlignedBox>::iterator it = m_trucks[b]->collisionBoundingBoxes.begin(); it != m_trucks[b]->collisionBoundingBoxes.end(); ++it)
			if (it->intersects(m_trucks[a]->boundingBox))
				return true;
	} else if (m_trucks[b]->collisionBoundingBoxes.empty())
	{
		for (std::vector<AxisAlignedBox>::iterator it = m_trucks[a]->collisionBoundingBoxes.begin(); it != m_trucks[a]->collisionBoundingBoxes.end(); ++it)
			if (it->intersects(m_trucks[b]->boundingBox))
				return true;
	} else
	{
		for (std::vector<AxisAlignedBox>::iterator it_a = m_trucks[a]->collisionBoundingBoxes.begin(); it_a != m_trucks[a]->collisionBoundingBoxes.end(); ++it_a)
			for (std::vector<AxisAlignedBox>::iterator it_b = m_trucks[b]->collisionBoundingBoxes.begin(); it_b != m_trucks[b]->collisionBoundingBoxes.end(); ++it_b)
				if (it_a->intersects(*it_b))
					return true;
	}

	return false;
}

bool BeamFactory::predictTruckIntersectionCollAABB(int a, int b)
{
	if (m_trucks[a]->predictedCollisionBoundingBoxes.empty() && m_trucks[b]->predictedCollisionBoundingBoxes.empty())
	{
		return predictTruckIntersectionAABB(a, b);
	} else if (m_trucks[a]->predictedCollisionBoundingBoxes.empty())
	{
		for (std::vector<AxisAlignedBox>::iterator it = m_trucks[b]->predictedCollisionBoundingBoxes.begin(); it != m_trucks[b]->predictedCollisionBoundingBoxes.end(); ++it)
			if (it->intersects(m_trucks[a]->predictedBoundingBox))
				return true;
	} else if (m_trucks[b]->predictedCollisionBoundingBoxes.empty())
	{
		for (std::vector<AxisAlignedBox>::iterator it = m_trucks[a]->predictedCollisionBoundingBoxes.begin(); it != m_trucks[a]->predictedCollisionBoundingBoxes.end(); ++it)
			if (it->intersects(m_trucks[b]->predictedBoundingBox))
				return true;
	} else
	{
		for (std::vector<AxisAlignedBox>::iterator it_a = m_trucks[a]->predictedCollisionBoundingBoxes.begin(); it_a != m_trucks[a]->predictedCollisionBoundingBoxes.end(); ++it_a)
			for (std::vector<AxisAlignedBox>::iterator it_b = m_trucks[b]->predictedCollisionBoundingBoxes.begin(); it_b != m_trucks[b]->predictedCollisionBoundingBoxes.end(); ++it_b)
				if (it_a->intersects(*it_b))
					return true;
	}

	return false;
}

// j is the index of a MAYSLEEP truck, returns true if one active was found in the set
bool BeamFactory::CheckForActive(int j, std::bitset<MAX_TRUCKS> &sleepy)
{
	sleepy.set(j, true);
	for (int t=0; t < m_free_truck; t++)
	{
		if (m_trucks[t] && !sleepy[t] && predictTruckIntersectionCollAABB(t, j))
		{
			if (m_trucks[t]->state == SLEEPING || m_trucks[t]->state == MAYSLEEP || m_trucks[t]->state == GOSLEEP || (m_trucks[t]->state == DESACTIVATED && m_trucks[t]->sleepcount >= 5))
				return this->CheckForActive(t, sleepy);
			else
				return true;
		}
	}
	return false;
}

void BeamFactory::RecursiveActivation(int j)
{
	if (!m_trucks[j] || m_trucks[j]->state > DESACTIVATED) return;

	for (int t=0; t < m_free_truck; t++)
	{
		if (t == j || !m_trucks[t]) continue;
		if ((m_trucks[t]->state == SLEEPING || m_trucks[t]->state == MAYSLEEP || m_trucks[t]->state == GOSLEEP || (m_trucks[t]->state == DESACTIVATED && m_trucks[t]->sleepcount >= 5)) &&
			predictTruckIntersectionCollAABB(t, j))
		{
			m_trucks[t]->desactivate(); // make the truck not leading but active
			this->RecursiveActivation(t);
		}
	}
}

void BeamFactory::UpdateSleepingState(float dt)
{
	for (int t=0; t<m_free_truck; t++)
	{
		if (!m_trucks[t]) continue;

		if (m_trucks[t]->state <= DESACTIVATED && (t == m_simulated_truck || m_trucks[t]->sleepcount <= 7))
			this->RecursiveActivation(t);

		// synchronous sleep
		if (m_trucks[t]->state == GOSLEEP) m_trucks[t]->state = SLEEPING;

		if (!m_forced_active && m_trucks[t]->state == DESACTIVATED)
		{
			m_trucks[t]->sleepcount++;
			if (m_trucks[t]->getVelocity().squaredLength() > 0.01f)
			{
				m_trucks[t]->sleepcount = 7;
			} else if (m_trucks[t]->sleepcount > 10)
			{
				m_trucks[t]->state = MAYSLEEP;
				m_trucks[t]->sleepcount = 0;
			}
		}
	}

	if (!m_forced_active)
	{
		// put to sleep
		for (int t=0; t < m_free_truck; t++)
		{
			if (m_trucks[t] && m_trucks[t]->state == MAYSLEEP)
			{
				std::bitset<MAX_TRUCKS> sleepy;
				if (!this-CheckForActive(t, sleepy))
				{
					// no active truck in the set, put everybody to sleep
					for (int i=0; i < m_free_truck; i++)
					{
						if (m_trucks[i] && sleepy[i])
						{
							m_trucks[i]->state = GOSLEEP;
						}
					}
				}
			}
		}
	}
}

int BeamFactory::GetFreeTruckSlot()
{
	// find a free slot for the truck
	for (int t=0; t<MAX_TRUCKS; t++)
	{
		if (!m_trucks[t] && t >= m_free_truck) // XXX: TODO: remove this hack
		{
			// reuse slots
			if (t >= m_free_truck)
				m_free_truck = t + 1;
			return t;
		}
	}
	return -1;
}

void BeamFactory::activateAllTrucks()
{
	for (int t=0; t < m_free_truck; t++)
	{
		if (m_trucks[t] && m_trucks[t]->state >= DESACTIVATED && m_trucks[t]->state <= SLEEPING)
		{
			m_trucks[t]->desactivate(); // make the truck not leading but active

			if (this->getTruck(m_simulated_truck))
			{
				m_trucks[t]->disableDrag = this->getTruck(m_simulated_truck)->driveable==AIRPLANE;
			}
		}
	}
}

void BeamFactory::sendAllTrucksSleeping()
{
	m_forced_active = false;
	for (int t=0; t < m_free_truck; t++)
	{
		if (m_trucks[t] && m_trucks[t]->state < SLEEPING)
		{
			m_trucks[t]->state = SLEEPING;
		}
	}
}

void BeamFactory::recalcGravityMasses()
{
	// update the mass of all trucks
	for (int t=0; t < m_free_truck; t++)
	{
		if (m_trucks[t])
		{
			m_trucks[t]->recalc_masses();
		}
	}
}

int BeamFactory::FindTruckInsideBox(Collisions *collisions, const Ogre::String &inst, const Ogre::String &box)
{
	// try to find the desired truck (the one in the box)
	int id = -1;
	for (int t=0; t < m_free_truck; t++)
	{
		if (!m_trucks[t]) continue;
		if (collisions->isInside(m_trucks[t]->nodes[0].AbsPosition, inst, box))
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
	int rtruck = this->FindTruckInsideBox(collisions, inst, box);
	if (rtruck >= 0)
	{
		// take a position reference
#ifdef USE_OPENAL
		SoundScriptManager::getSingleton().trigOnce(rtruck, SS_TRIG_REPAIR);
#endif // USE_OPENAL
		Vector3 ipos = m_trucks[rtruck]->nodes[0].AbsPosition;
		m_trucks[rtruck]->reset();
		m_trucks[rtruck]->resetPosition(ipos.x, ipos.z, false, 0);
		m_trucks[rtruck]->updateVisual();
	}
}

void BeamFactory::MuteAllTrucks()
{
	for (int i = 0; i < m_free_truck; i++)
	{
		if (m_trucks[i])
		{
			m_trucks[i]->StopAllSounds();
		}
	}
}

void BeamFactory::UnmuteAllTrucks()
{
	for (int i = 0; i < m_free_truck; i++)
	{
		if (m_trucks[i])
		{
			m_trucks[i]->UnmuteAllSounds();
		}
	}
}

void BeamFactory::removeTruck(Collisions *collisions, const Ogre::String &inst, const Ogre::String &box)
{
	removeTruck(this->FindTruckInsideBox(collisions, inst, box));
}

void BeamFactory::removeTruck(int truck)
{
	if (truck < 0 || truck > m_free_truck)
		return;

	if (!m_trucks[truck])
		return;

	if (m_trucks[truck]->state == NETWORKED)
		return;

	this->DeleteTruck(m_trucks[truck]);
}

void BeamFactory::removeAllTrucks()
{
	for (int i = 0; i < m_free_truck; i++)
	{
		removeTruck(i);
	}
}

void BeamFactory::DeleteTruck(Beam *b)
{
	if (b == 0)	return;

	this->SyncWithSimThread();

	if (b->networking && b->state < NETWORKED)
	{
		NetworkStreamManager::getSingleton().removeLocalStream(b);
	}

	if (m_current_truck == b->trucknum)
		setCurrentTruck(-1);

	m_trucks[b->trucknum] = 0;
	delete b;

#ifdef USE_MYGUI
	GUI_MainMenu::getSingleton().triggerUpdateVehicleList();
#endif // USE_MYGUI
}

void BeamFactory::removeCurrentTruck()
{
	removeTruck(m_current_truck);
}

int BeamFactory::GetMostRecentTruckSlot()
{
	if (getTruck(m_current_truck))
	{
		return m_current_truck;
	} else if (getTruck(m_previous_truck))
	{
		return m_previous_truck;
	}

	return -1;
}

void BeamFactory::enterNextTruck()
{
	int pivot_index = this->GetMostRecentTruckSlot();

	for (int i = pivot_index + 1; i < m_free_truck; i++)
	{
		if (m_trucks[i] && !m_trucks[i]->isPreloadedWithTerrain())
		{
			setCurrentTruck(i);
			return;
		}
	}

	for (int i = 0; i < pivot_index; i++)
	{
		if (m_trucks[i] && !m_trucks[i]->isPreloadedWithTerrain())
		{
			setCurrentTruck(i);
			return;
		}
	}
}

void BeamFactory::enterPreviousTruck()
{
	int pivot_index = this->GetMostRecentTruckSlot();

	for (int i = pivot_index - 1; i >= 0; i--)
	{
		if (m_trucks[i] && !m_trucks[i]->isPreloadedWithTerrain())
		{
			setCurrentTruck(i);
			return;
		}
	}

	for (int i = m_free_truck - 1; i > pivot_index; i--)
	{
		if (m_trucks[i] && !m_trucks[i]->isPreloadedWithTerrain())
		{
			setCurrentTruck(i);
			return;
		}
	}
}

void BeamFactory::setCurrentTruck(int new_truck)
{
	if (m_current_truck >= 0 && m_current_truck < m_free_truck && m_trucks[m_current_truck])
	{
		m_trucks[m_current_truck]->desactivate();
	}

	m_previous_truck = m_current_truck;
	m_current_truck = new_truck;

	{
		if (m_previous_truck >= 0 && m_current_truck >= 0)
		{
			RoR::MainThread::ChangedCurrentVehicle(m_trucks[m_previous_truck], m_trucks[m_current_truck]);
		}
		else if (m_previous_truck >= 0)
		{
			RoR::MainThread::ChangedCurrentVehicle(m_trucks[m_previous_truck], nullptr);
		}
		else if (m_current_truck >= 0)
		{
			RoR::MainThread::ChangedCurrentVehicle(nullptr, m_trucks[m_current_truck]);
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
	for (int t=0; t < m_free_truck; t++)
	{
		if (m_trucks[t] && m_trucks[t]->rescuer) {
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
	for (int t=0; t < m_free_truck; t++)
	{
		if (m_trucks[t] && (m_trucks[t]->state < SLEEPING || m_trucks[t]->state == NETWORKED))
		{
			m_trucks[t]->updateFlexbodiesPrepare();
		}
	}
}

void BeamFactory::joinFlexbodyTasks()
{
	for (int t=0; t < m_free_truck; t++)
	{
		if (m_trucks[t] && (m_trucks[t]->state < SLEEPING || m_trucks[t]->state == NETWORKED))
		{
			m_trucks[t]->joinFlexbodyTasks();
		}
	}
}

void BeamFactory::updateFlexbodiesFinal()
{
	for (int t=0; t < m_free_truck; t++)
	{
		if (m_trucks[t] && (m_trucks[t]->state < SLEEPING || m_trucks[t]->state == NETWORKED))
		{
			m_trucks[t]->updateFlexbodiesFinal();
		}
	}
}

void BeamFactory::updateVisual(float dt)
{
	dt *= m_simulation_speed;

	for (int t=0; t < m_free_truck; t++)
	{
		if (!m_trucks[t]) continue;

		// always update the labels
		m_trucks[t]->updateLabels(dt);

		if (m_trucks[t]->state < SLEEPING || m_trucks[t]->state == NETWORKED)
		{
			m_trucks[t]->updateVisual(dt);
			m_trucks[t]->updateSkidmarks();
			m_trucks[t]->updateFlares(dt, (t==m_current_truck));
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

	this->SyncWithSimThread();

	this->UpdateSleepingState(dt);

	for (int t=0; t < m_free_truck; t++)
	{
		if (!m_trucks[t]) continue;

		m_trucks[t]->handleResetRequests(dt);
		m_trucks[t]->handleTruckPosition(dt);
		m_trucks[t]->updateAngelScriptEvents(dt);

		switch(m_trucks[t]->state)
		{
		case NETWORKED:
			m_trucks[t]->calcNetwork();
			break;

		case NETWORKED_INVALID:
			break;

		default:
			if (m_trucks[t]->state > DESACTIVATED && m_trucks[t]->engine)
				m_trucks[t]->engine->update(dt, 1);
			if (m_trucks[t]->networking)
				m_trucks[t]->sendStreamData();
			break;
		}
	}

	m_simulated_truck = m_current_truck;

	if (m_simulated_truck == -1)
	{
		for (int t=0; t < m_free_truck; t++)
		{
			if (m_trucks[t] && m_trucks[t]->state <= DESACTIVATED)
			{
				m_simulated_truck = t;
				break;
			}
		}
	}

	if (m_simulated_truck >= 0 && m_simulated_truck < m_free_truck)
	{
		if (m_simulated_truck == m_current_truck)
		{
#ifdef USE_MYGUI
		m_trucks[m_simulated_truck]->updateDashBoards(dt);
#endif // USE_MYGUI
		m_trucks[m_simulated_truck]->updateTruckMirrors(dt);
#ifdef FEAT_TIMING
			if (m_trucks[m_simulated_truck]->statistics)     m_trucks[m_simulated_truck]->statistics->frameStep(dt);
			if (m_trucks[m_simulated_truck]->statistics_gfx) m_trucks[m_simulated_truck]->statistics_gfx->frameStep(dt);
#endif // FEAT_TIMING
		}
		if (!m_trucks[m_simulated_truck]->replayStep())
		{
			m_trucks[m_simulated_truck]->updateFrameTimeInformation(dt);
			m_trucks[m_simulated_truck]->updateForceFeedback(m_physics_steps);
			if (m_sim_thread_pool)
			{
				auto func = std::function<void(int)>([this](float dt) {
					this->UpdatePhysicsSimulation();
				});
				m_sim_task = m_sim_thread_pool->RunTask(std::bind(func, dt));
			} else
			{
				this->UpdatePhysicsSimulation();
			}
		}
	}
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
		{
			this->DeleteTruck(it_beam->second);
			it_beam->second = 0;
		}
	} else
	{
		// find the stream matching the streamid
		it_beam = it_stream->second.find(del->streamid);
		if (it_beam != it_stream->second.end())
		{
			this->DeleteTruck(it_beam->second);
			it_beam->second = 0;
		}
	}
	// unlockStreams();
}

void BeamFactory::windowResized()
{
#ifdef USE_MYGUI
	for (int t=0; t < m_free_truck; t++)
	{
		if (m_trucks[t])
		{
			m_trucks[t]->dash->windowResized();
		}
	}
#endif // USE_MYGUI
}

void BeamFactory::prepareShutdown()
{
	this->SyncWithSimThread();
}

Beam* BeamFactory::getCurrentTruck()
{
	return this->getTruck(m_current_truck);
}

Beam* BeamFactory::getTruck(int number)
{
	if (number >= 0 && number < m_free_truck)
	{
		return m_trucks[number];
	}
	return 0;
}

void BeamFactory::UpdatePhysicsSimulation()
{
	if (gEnv->threadPool)
	{
		for (int i=0; i<m_physics_steps; i++)
		{
			int num_simulated_trucks = 0;
			{
				std::vector<std::function<void()>> tasks;
				for (int t=0; t<m_free_truck; t++)
				{
					if (m_trucks[t] && (m_trucks[t]->simulated = m_trucks[t]->calcForcesEulerPrepare(i==0, PHYSICS_DT, i, m_physics_steps)))
					{
						num_simulated_trucks++;
						auto func = std::function<void(int)>([this, i](int t){
							m_trucks[t]->calcForcesEulerCompute(i==0, PHYSICS_DT, i, m_physics_steps);
							if (!m_trucks[t]->disableTruckTruckSelfCollisions)
							{
								m_trucks[t]->IntraPointCD()->update(m_trucks[t]);
								intraTruckCollisions(PHYSICS_DT,
										*(m_trucks[t]->IntraPointCD()),
										m_trucks[t]->free_collcab,
										m_trucks[t]->collcabs,
										m_trucks[t]->cabs,
										m_trucks[t]->intra_collcabrate,
										m_trucks[t]->nodes,
										m_trucks[t]->collrange,
										*(m_trucks[t]->submesh_ground_model));
							}
						});
						tasks.push_back(std::bind(func, t));
					}
				}
				gEnv->threadPool->Parallelize(tasks);
			}

			for (int t=0; t<m_free_truck; t++)
			{
				if (m_trucks[t] && m_trucks[t]->simulated)
					m_trucks[t]->calcForcesEulerFinal(i==0, PHYSICS_DT, i, m_physics_steps);
			}

			if (num_simulated_trucks > 1)
			{
				std::vector<std::function<void()>> tasks;
				for (int t=0; t<m_free_truck; t++)
				{
					if (m_trucks[t] && m_trucks[t]->simulated && !m_trucks[t]->disableTruckTruckCollisions)
					{
						auto func = std::function<void(int)>([this](int t){
							m_trucks[t]->InterPointCD()->update(m_trucks[t], m_trucks, m_free_truck);
							if (m_trucks[t]->collisionRelevant)
							{
								interTruckCollisions(PHYSICS_DT,
										*(m_trucks[t]->InterPointCD()),
										m_trucks[t]->free_collcab,
										m_trucks[t]->collcabs,
										m_trucks[t]->cabs,
										m_trucks[t]->inter_collcabrate,
										m_trucks[t]->nodes,
										m_trucks[t]->collrange,
										m_trucks, m_free_truck,
										*(m_trucks[t]->submesh_ground_model));
							}
						});
						tasks.push_back(std::bind(func, t));
					}
				}
				gEnv->threadPool->Parallelize(tasks);
			}
		}
	} else
	{
		for (int i=0; i<m_physics_steps; i++)
		{
			int num_simulated_trucks = 0;

			for (int t=0; t<m_free_truck; t++)
			{
				if (m_trucks[t] && (m_trucks[t]->simulated = m_trucks[t]->calcForcesEulerPrepare(i==0, PHYSICS_DT, i, m_physics_steps)))
				{
					num_simulated_trucks++;
					m_trucks[t]->calcForcesEulerCompute(i==0, PHYSICS_DT, i, m_physics_steps);
					m_trucks[t]->calcForcesEulerFinal(i==0, PHYSICS_DT, i, m_physics_steps);
					if (!m_trucks[t]->disableTruckTruckSelfCollisions)
					{
						m_trucks[t]->IntraPointCD()->update(m_trucks[t]);
						intraTruckCollisions(PHYSICS_DT,
								*(m_trucks[t]->IntraPointCD()),
								m_trucks[t]->free_collcab,
								m_trucks[t]->collcabs,
								m_trucks[t]->cabs,
								m_trucks[t]->intra_collcabrate,
								m_trucks[t]->nodes,
								m_trucks[t]->collrange,
								*(m_trucks[t]->submesh_ground_model));
					}
				}
			}

			if (num_simulated_trucks > 1)
			{
				BES_START(BES_CORE_Contacters);
				for (int t=0; t<m_free_truck; t++)
				{
					if (m_trucks[t] && m_trucks[t]->simulated && !m_trucks[t]->disableTruckTruckCollisions)
					{
						m_trucks[t]->InterPointCD()->update(m_trucks[t], m_trucks, m_free_truck);
						if (m_trucks[t]->collisionRelevant) {
							interTruckCollisions(
									PHYSICS_DT,
									*(m_trucks[t]->InterPointCD()),
									m_trucks[t]->free_collcab,
									m_trucks[t]->collcabs,
									m_trucks[t]->cabs,
									m_trucks[t]->inter_collcabrate,
									m_trucks[t]->nodes,
									m_trucks[t]->collrange,
									m_trucks, m_free_truck,
									*(m_trucks[t]->submesh_ground_model));
						}
					}
				}
				BES_STOP(BES_CORE_Contacters);
			}
		}
	}
}

void BeamFactory::SyncWithSimThread()
{
	if (m_sim_task)
		m_sim_task->join();
}

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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 7th of August 2009

#ifndef __NetworkStreamManager_H_
#define __NetworkStreamManager_H_

#include "RoRPrerequisites.h"

#include <pthread.h>

#include "rornet.h"
#include "Singleton.h"

#ifdef USE_SOCKETW
#include "SocketW.h"
#endif // USE_SOCKETW

class Streamable;
class StreamableFactoryInterface;

class NetworkStreamManager : public RoRSingleton< NetworkStreamManager >, public ZeroedMemoryAllocator
{
	friend class Network;

public:

	NetworkStreamManager();
	~NetworkStreamManager();
	
	void addLocalStream(Streamable *stream, stream_register_t *reg, unsigned int size=0);
	void addRemoteStream(Streamable *stream, int source=-1, int streamid=-1);
	void removeStream(int sourceid, int streamid);

	void pauseStream(Streamable *stream);
	void resumeStream(Streamable *stream);
	
	void triggerSend();

#ifdef USE_SOCKETW
	void sendStreams(Network *net, SWInetSocket *socket);
#else
	void sendStreams(Network *net, void *socket);
#endif //SOCKETW

	void update();

	void removeUser(int sourceID);

	void addFactory(StreamableFactoryInterface *factory);

protected:

	pthread_mutex_t stream_mutex;
	pthread_mutex_t send_work_mutex;
	pthread_cond_t send_work_cv;

	bool send_start;

	std::map < int, std::map < unsigned int, Streamable *> > streams;
	std::vector < StreamableFactoryInterface * > factories;

	unsigned int streamid;

	void pushReceivedStreamMessage(header_t header, char *buffer);

	void syncRemoteStreams();
	void receiveStreams();
};

#endif // __NetworkStreamManager_H_

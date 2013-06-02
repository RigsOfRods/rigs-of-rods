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

#include "NetworkStreamManager.h"

#include "Network.h"
#include "Language.h"
#include "Streamable.h"
#include "StreamableFactoryInterface.h"

using namespace Ogre;

NetworkStreamManager::NetworkStreamManager() : 
	  send_start(false)
	, streamid(10)
{
	pthread_cond_init(&send_work_cv, NULL);
	pthread_mutex_init(&send_work_mutex, NULL);
	pthread_mutex_init(&stream_mutex, NULL);
}

NetworkStreamManager::~NetworkStreamManager()
{
	pthread_cond_destroy(&send_work_cv);
	pthread_mutex_destroy(&send_work_mutex);
	pthread_mutex_destroy(&stream_mutex);
}

void NetworkStreamManager::addLocalStream(Streamable *stream, stream_register_t *reg, unsigned int size)
{
#ifdef USE_SOCKETW
	MUTEX_LOCK(&stream_mutex);
	// for own streams: count stream id up ...
	int mysourceid = gEnv->network->getUserID();

	// use counting streamid
	stream->setSourceID(mysourceid);
	stream->setStreamID(streamid);

	// add IDs to registration
	reg->origin_sourceid = mysourceid;
	reg->origin_streamid = streamid;
	reg->status = 0;

	// tell the stream that its a local stream (an origin)
	stream->isOrigin = true;

	// add new stream map to the streams map
	if (streams.find(mysourceid) == streams.end())
		streams[mysourceid] = std::map < unsigned int, Streamable *>();
	// map the stream
	streams[mysourceid][streamid] = stream;
	LOG("adding local stream: " + TOSTRING(mysourceid) + ":"+ TOSTRING(streamid) + ", type: " + TOSTRING(reg->type));
	// send stream setup notice to server
	if (size == 0) size = sizeof(stream_register_t);
	stream->addPacket(MSG2_STREAM_REGISTER, size, (char*)reg);

	// increase stream counter
	streamid++;
	MUTEX_UNLOCK(&stream_mutex);
#endif // USE_SOCKETW
}

void NetworkStreamManager::addRemoteStream(Streamable *stream, int rsource, int rstreamid)
{
	//MUTEX_LOCK(&stream_mutex);
	streams[rsource][rstreamid] = stream;
	LOG("adding remote stream: " + TOSTRING(rsource) + ":"+ TOSTRING(rstreamid));
	//MUTEX_UNLOCK(&stream_mutex);
}

void NetworkStreamManager::removeStream(int sourceid, int streamid)
{
#ifdef USE_SOCKETW
	int mysourceid = gEnv->network->getUserID();

	if (sourceid == -1)
	{
		sourceid = mysourceid;
	}

	MUTEX_LOCK(&stream_mutex);

	std::map < int, std::map < unsigned int, Streamable *> >::iterator it_source = streams.find(sourceid);
	std::map < unsigned int, Streamable *>::iterator it_stream;

	if (it_source != streams.end() && !it_source->second.empty())
	{
		it_stream = it_source->second.find(streamid);
		if (it_stream != it_source->second.end())
			streams[sourceid].erase(it_stream);
	}

	if (sourceid != mysourceid)
	{
		// now iterate over all factories and remove their instances (only triggers)
		std::vector < StreamableFactoryInterface * >::iterator it;
		for (it=factories.begin(); it!=factories.end(); it++)
		{
			(*it)->deleteRemote(sourceid, streamid);
		}
	}

	MUTEX_UNLOCK(&stream_mutex);
#endif // USE_SOCKETW
}


void NetworkStreamManager::pauseStream(Streamable *stream)
{
}

void NetworkStreamManager::resumeStream(Streamable *stream)
{
}

#ifdef USE_SOCKETW
void NetworkStreamManager::removeUser(int sourceID)
{
	MUTEX_LOCK(&stream_mutex);
	if (streams.find(sourceID) == streams.end())
	{
		// no such stream?!
		MUTEX_UNLOCK(&stream_mutex);
		return;
	}
	// found and deleted
	streams.erase(streams.find(sourceID));

	// now iterate over all factories and remove their instances (only triggers)
	std::vector < StreamableFactoryInterface * >::iterator it;
	for (it=factories.begin(); it!=factories.end(); it++)
	{
		(*it)->deleteRemote(sourceID, -1); // -1 = all streams
	}
	MUTEX_UNLOCK(&stream_mutex);
}
#endif // USE_SOCKETW

void NetworkStreamManager::pushReceivedStreamMessage(header_t header, char *buffer)
{
	MUTEX_LOCK(&stream_mutex);
	if (streams.find(header.source) == streams.end())
	{
		// no such stream?!
		LOG("EEE Source not found: "+TOSTRING(header.source)+":"+TOSTRING(header.streamid));
		MUTEX_UNLOCK(&stream_mutex);
		return;
	}
	if (streams.find(header.source)->second.find(header.streamid) == streams.find(header.source)->second.end())
	{
		// no such stream?!
		
		// removed: too verbose
		//if (header.streamid != 0)
		//	LOG("EEE Stream not found: "+TOSTRING(header.source)+":"+TOSTRING(header.streamid));
		
		MUTEX_UNLOCK(&stream_mutex);
		return;
	}
	streams[header.source][header.streamid]->addReceivedPacket(header, buffer);
	MUTEX_UNLOCK(&stream_mutex);
}

void NetworkStreamManager::triggerSend()
{
	MUTEX_LOCK(&send_work_mutex);
	send_start = true;
	pthread_cond_broadcast(&send_work_cv);
	MUTEX_UNLOCK(&send_work_mutex);
}

#ifdef USE_SOCKETW
void NetworkStreamManager::sendStreams(Network *net, SWInetSocket *socket)
{
	MUTEX_LOCK(&send_work_mutex);
	while (!send_start)
	{
		pthread_cond_wait(&send_work_cv, &send_work_mutex);
	}
	send_start = false;
	MUTEX_UNLOCK(&send_work_mutex);

	MUTEX_LOCK(&stream_mutex);
	//char *buffer = 0;
	//int bufferSize=0;

	std::map < int, std::map < unsigned int, Streamable *> >::iterator it;
	for (it=streams.begin(); it!=streams.end(); it++)
	{
		std::map<unsigned int,Streamable *>::iterator it2;
		for (it2=it->second.begin(); it2!=it->second.end(); it2++)
		{
			if (!it2->second) continue;
			std::deque <Streamable::bufferedPacket_t> *packets = it2->second->getPacketQueue();

			while (!packets->empty())
			{
				// remove oldest packet in queue
				Streamable::bufferedPacket_t packet = packets->front();

				int etype = net->sendMessageRaw(socket, packet.packetBuffer, packet.size);
				if (etype)
				{
					wchar_t emsg[256];
					UTFString tmp = _L("Error %i while sending data packet");
					swprintf(emsg, 256, tmp.asWStr_c_str(), etype);
					net->netFatalError(UTFString(emsg));
					MUTEX_UNLOCK(&stream_mutex);
					return;
				}

				packets->pop_front();
			}

		}
	}
	MUTEX_UNLOCK(&stream_mutex);
}
#else
void NetworkStreamManager::sendStreams(Network *net, void *socket)
{
}
#endif // USE_SOCKETW

#ifdef USE_SOCKETW
void NetworkStreamManager::update()
{
	syncRemoteStreams();
	receiveStreams();
}
#endif // USE_SOCKETW

void NetworkStreamManager::syncRemoteStreams()
{
	MUTEX_LOCK(&stream_mutex);
	// iterate over all factories
	std::vector < StreamableFactoryInterface * >::iterator it;
	for (it=factories.begin(); it!=factories.end(); it++)
	{
		(*it)->syncRemoteStreams();
	}
	MUTEX_UNLOCK(&stream_mutex);
}

void NetworkStreamManager::receiveStreams()
{
	MUTEX_LOCK(&stream_mutex);
	//char *buffer = 0;
	//int bufferSize=0;
	std::map < int, std::map < unsigned int, Streamable *> >::iterator it;
	for (it=streams.begin(); it!=streams.end(); it++)
	{
		std::map<unsigned int,Streamable *>::iterator it2;
		for (it2=it->second.begin(); it2!=it->second.end(); it2++)
		{
			if (!it2->second) continue;
			it2->second->lockReceiveQueue();
			std::deque <recvPacket_t> *packets = it2->second->getReceivePacketQueue();

			while (!packets->empty())
			{
				// remove oldest packet in queue
				recvPacket_t packet = packets->front();

				//Network::debugPacket("receive-2", &packet.header, (char *)packet.buffer);

				if (it2->second) it2->second->receiveStreamData(packet.header.command, packet.header.source, packet.header.streamid, (char*)packet.buffer, packet.header.size);

				packets->pop_front();
			}
			it2->second->unlockReceiveQueue();
		}
	}
	MUTEX_UNLOCK(&stream_mutex);
}

void NetworkStreamManager::addFactory(StreamableFactoryInterface *factory)
{
	MUTEX_LOCK(&stream_mutex);
	this->factories.push_back(factory);
	MUTEX_UNLOCK(&stream_mutex);
}


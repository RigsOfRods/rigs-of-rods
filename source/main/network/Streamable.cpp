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

#include "Streamable.h"

#include "Network.h"
#include "NetworkStreamManager.h"

using namespace Ogre;

Streamable::Streamable() : isOrigin(false), streamResultsChanged(false)
{
	//NetworkStreamManager::getSingleton().addStream(this);
	pthread_mutex_init(&recv_work_mutex, NULL);
}

Streamable::~Streamable()
{
}

std::deque < Streamable::bufferedPacket_t > *Streamable::getPacketQueue()
{
	return &packets;
}

std::deque < recvPacket_t > *Streamable::getReceivePacketQueue()
{
	return &receivedPackets;
}

void Streamable::addPacket(int type, unsigned int len, char* content)
{
#ifdef USE_SOCKETW
	if (packets.size() > packetBufferSizeDiscardData && type == MSG2_STREAM_DATA)
		// discard unimportant data packets for some while
		return;

	if (packets.size() > packetBufferSize)
		// buffer full, packet discarded
		return;
	if (len > maxPacketLen)
		// packet too big, discarded
		return;

	int uid = Network::getUID();
	unsigned int streamid = this->streamid; //we stored the streamid upon stream registration in this class

	bufferedPacket_t packet;
	memset(&packet, 0, sizeof(bufferedPacket_t));

	// allocate buffer
	//packet.packetBuffer = (char*)calloc(len, sizeof(char));

	char *buffer = (char*)(packet.packetBuffer);

	// write header in buffer
	header_t *head = (header_t *)buffer;
	head->command  = type;
	head->source   = uid;
	head->size     = len;
	head->streamid = streamid;

	// then copy the contents
	char *bufferContent = (char *)(buffer + sizeof(header_t));
	memcpy(bufferContent, content, len);

	// record the packet size
	packet.size = len + sizeof(header_t);

	/*
	String header_hex = hexdump(buffer, sizeof(header_t));
	String content_hex = hexdump((buffer + sizeof(header_t)), len);

	LOG("header:  " + header_hex);
	LOG("content: " + content_hex);
	*/

	/*
	char hash[256] = "";
	RoR::CSHA1 sha1;
	sha1.UpdateHash((uint8_t *)bufferContent, len);
	sha1.Final();
	sha1.ReportHash(hash, RoR::CSHA1::REPORT_HEX_SHORT);
	LOG("S|HASH: " + String(hash));
	*/

	packets.push_back(packet);

	// trigger buffer clearing
	NetworkStreamManager::getSingleton().triggerSend();
#endif //SOCKETW
}

void Streamable::addReceivedPacket(header_t header, char *buffer)
{
	if (packets.size() > packetBufferSizeDiscardData && header.command == MSG2_STREAM_DATA)
		// discard unimportant data packets for some while
		return;

	if (receivedPackets.size() > packetBufferSize)
		// buffer full, packet discarded
		return;
	MUTEX_LOCK(&recv_work_mutex);

	// construct the data holding struct
	recvPacket_t packet;
	memset(&packet, 0, sizeof(packet)); // we need to do this, since we use String(buffer) at some point which will crash otherwise
	packet.header = header;
	memcpy(packet.buffer, buffer, header.size);

	receivedPackets.push_back(packet);
	MUTEX_UNLOCK(&recv_work_mutex);
}

void Streamable::lockReceiveQueue()
{
	MUTEX_LOCK(&recv_work_mutex);
}

void Streamable::unlockReceiveQueue()
{
	MUTEX_UNLOCK(&recv_work_mutex);
}

void Streamable::addStreamRegistrationResult(int sourceid, stream_register_t reg)
{
	mStreamableResults[sourceid] = reg;
	streamResultsChanged=true;
}

int Streamable::getStreamRegisterResultForSource(int sourceid, stream_register_t *reg)
{
	if (mStreamableResults.find(sourceid) == mStreamableResults.end())
		return 1;
	*reg = mStreamableResults[sourceid];
	return 0;
}

bool Streamable::getStreamResultsChanged()
{
	if (streamResultsChanged)
	{
		streamResultsChanged = false;
		return true;
	}
	return false;
}